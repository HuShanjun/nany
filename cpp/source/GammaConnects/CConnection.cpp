
#include "GammaCommon/TGammaStrStream.h"
#include "GammaCommon/CPkgFile.h"
#include "CConnectionMgr.h"
#include "CConnection.h"

namespace Gamma
{
	static const CAddress s_Empty;

	CConnection::CConnection( CConnectionMgr* pConnMgr, 
		IConnecter* pConnect, CBaseConn* pBase, 
		const char* szConnectAddress, bool bTcp )
		: m_pConnMgr( pConnMgr )
		, m_pConnecter( pConnect )
		, m_bEnableSendShellMsg( true )
		, m_nMinDelay(0)
		, m_nMaxDelay(0)
		, m_nEnableDispatchState( eES_Enable )
		, m_nCreateTime( pConnMgr->GetCurCheckTime() )
		, m_eConnectionType( 0 )
		, m_eCloseType( eCT_None )
	{
		if( bTcp )
			m_eConnectionType |= eTcp;
		if( szConnectAddress == NULL )
			m_eConnectionType |= eServer;

		m_pConnHandler = pBase;
		m_pConnHandler->m_pConnection = this;
		if( !m_pConnecter )
			return;
		m_pConnecter->SetHandler( this );
	}

	CConnection::~CConnection(void)
	{
		GammaAst( m_pConnecter == NULL || m_pConnecter->GetHandler() == NULL );
		m_pConnecter = NULL;
	}	

	void CConnection::ShutDown( bool bGrace, const char* szLogContext )
	{
		if( !m_pConnecter )
			return;
		if( szLogContext )
			m_strCloseLog.assign( szLogContext );
		m_eCloseType = (uint8)( bGrace ? eCT_Grace : eCT_Force );
		m_pConnecter->CmdClose( bGrace );
	}

	void CConnection::OnConnected()
	{ 
		try
		{
			uint32 nCurInterval = GetHeartBeatInterval();
			SetHeartBeatInterval( 0 );
			OnCheckTimeOut();
			SetHeartBeatInterval( nCurInterval );
			if( m_pConnHandler )
				m_pConnHandler->OnConnected();
		}
		catch( ... )
		{
			GammaErr << "OnConnected Error" << endl;
		}
	}

	void CConnection::OnDisConnect( ECloseType eTypeClose )
	{
		if( m_pConnHandler )
		{
			try
			{
				if( !m_pConnMgr->IsStrictMode() ||
					( m_pConnecter && m_pConnecter->IsEverConnected() ) )
					m_pConnHandler->OnDisConnect();
			}
			catch( ... )
			{
				GammaErr << "OnDisConnect Error" << endl;
			}

			if( m_pConnHandler )
			{
				m_pConnHandler->m_pConnection = nullptr;
				CDynamicObject::DestroyInstance( m_pConnHandler );
				m_pConnHandler = NULL;
			}
		}
		delete this;
	}

	size_t CConnection::OnRecv( const char* pBuf, size_t nSize )
	{
		// 重置接收时间    
		if( m_nEnableDispatchState == eES_Disable )
			return 0;

		// 如果有网络延迟模拟，则先缓存
		// 如果没有网络延迟模拟，但有之前先缓存的数据，
		// 也要缓存到所有数据处理完毕
		if( m_nMaxDelay || !m_szRecvBuf.empty() )
		{
			uint32 nDelayTime = CGammaRand::Rand( m_nMinDelay, m_nMaxDelay );
			int64 nRecvTime  = GetGammaTime() + nDelayTime;
			m_szRecvBuf.append( (const char*)&nRecvTime, sizeof(int64) );
			m_szRecvBuf.append( (const char*)&nSize, sizeof(size_t) );
			m_szRecvBuf.append( (const char*)pBuf, nSize );
			return nSize;
		}

		return Process( pBuf, nSize );
	}

	size_t CConnection::Process( const char* pBuf, size_t nSize )
	{
		size_t nDispatchSize = 0;

		try
		{
			nDispatchSize = Dispatch( pBuf, nSize );
		}
		catch( ... )
		{
			char szBuf[256];
			gammasstream( szBuf, ELEM_COUNT(szBuf) ) << "Dispatch Msg Error ( "
				<< GetRemoteAddress().GetAddress() << ":"
				<< GetRemoteAddress().GetPort() << ", " << nSize;

			string szLog = szBuf;
			for( size_t i = 0; i < 32 && i < nSize; i++ )
			{
				char c1 = (char)( ( (uint8)pBuf[i] ) & 0xf );
				char c2 = (char)( ( (uint8)pBuf[i] ) >> 4 );
				szLog.push_back( c2 + ( c2 > 9 ? ( 'a' - 10 ) : '0' ) );
				szLog.push_back( c1 + ( c1 > 9 ? ( 'a' - 10 ) : '0' ) );
				szLog.push_back( ' ' );
			}
			szLog.push_back( ']' );
			GammaErr << szLog.c_str() << endl;

			ShutDown( false, "CConnection::Process" );
		}

		return nDispatchSize;
	}

	size_t CConnection::Dispatch( const char* pBuf, size_t nSize )
	{
		char nShellID[2] = { pBuf[0], pBuf[1] };
		nSize = GetHandler()->OnShellMsg( pBuf, nSize, !( m_eConnectionType&eTcp ) );
		AddRecvSize( *(uint16*)nShellID, (uint32)nSize );
		return nSize;
	}

	CConnectionMgr* CConnection::GetConnMgr() const
	{
		return m_pConnMgr;
	}

	bool CConnection::IsServer() const
	{
		return ( m_eConnectionType&eServer ) != 0;
	}


	bool CConnection::IsConnecting() const
	{
		return m_pConnecter && m_pConnecter->IsConnecting();
	}

	bool CConnection::IsConnected() const
	{ 
		return m_pConnecter && m_pConnecter->IsConnected();
	}

	bool CConnection::IsDisconnected() const
	{
		return !m_pConnecter || m_pConnecter->IsDisconnected();
	}

	const CAddress&	CConnection::GetLocalAddress()const		
	{ 
		return m_pConnecter ? m_pConnecter->GetLocalAddress() : s_Empty;
	}

	const CAddress&	CConnection::GetRemoteAddress()const
	{ 
		return m_pConnecter ? m_pConnecter->GetRemoteAddress() : s_Empty;
	}

	CBaseConn* CConnection::GetHandler()
	{
		return m_pConnHandler;
	}

	IConnecter*	CConnection::GetConn()
	{ 
		return m_pConnecter;
	}

	bool CConnection::IsDisconnecting() const
	{
		return m_pConnecter && m_pConnecter->IsDisconnecting();
	}

	int64 CConnection::GetCreateTime() const
	{
		return m_nCreateTime;
	}

	void CConnection::SendShellMsg( bool bReliable, const SSendBuf aryBuffer[], uint32 nBufferCount )
	{
		if( !m_bEnableSendShellMsg || !IsConnected() )
			return;

		size_t nTotalSize = 0;
		for( uint32 i = 0; i < nBufferCount; i++ )
			nTotalSize += aryBuffer[i].second;

		AddSendSize( *(const uint16*)aryBuffer[0].first, nTotalSize );

		if( ( m_eConnectionType&eTcp ) || nBufferCount == 1 )
		{
			SendBuffer( aryBuffer[0].first, aryBuffer[0].second );
			for( uint32 i = 1; i < nBufferCount; i++ )
				SendBuffer( aryBuffer[i].first, aryBuffer[i].second );
		}
		else
		{
			GammaAst( !bReliable );
			size_t nTotalSize = 0;
			char szBuffer[MAX_RAW_UPD_SIZE];
			for( uint32 i = 0; i < nBufferCount; i++ )
			{
				size_t nDataEnd = nTotalSize + aryBuffer[i].second;
				GammaAst( nDataEnd < MAX_RAW_UPD_SIZE );
				memcpy( szBuffer + nTotalSize, aryBuffer[i].first, aryBuffer[i].second );
				nTotalSize = nDataEnd;
			}
			SendBuffer( szBuffer, nTotalSize );
		}
	}

	void CConnection::EnableSendShellMsg( bool bValue )
	{
		m_bEnableSendShellMsg =  bValue;
	}

	bool CConnection::OnUpdate()
	{
		if( !IsConnected() )
			return true;

		int64 nCurTime = GetGammaTime();
		if( m_nEnableDispatchState == eES_Pending )
		{
			m_nEnableDispatchState = eES_Enable;
			m_pConnecter->CheckRecvBuff();
		}

		// 处理延时发送  
		CBufFile SendBufFile( (const tbyte*)m_szSendBuf.c_str(), (uint32)m_szSendBuf.size() );
		for( size_t nProcessSize = 0; SendBufFile.GetBufSize(); )
		{			
			int64 nSendTime;
			SendBufFile.Read( &nSendTime, sizeof(int64) );
			bool bSend = nCurTime >= nSendTime;
			if( bSend )
			{
				size_t nSize = *SendBufFile.PopData<size_t>();
				const tbyte* pBuffer = SendBufFile.GetBufByCur();
				m_pConnecter->Send( pBuffer, nSize );
				SendBufFile.SeekFromCur( (uint32)nSize );
				nProcessSize = SendBufFile.GetPos();
			}

			if( !bSend || nProcessSize == SendBufFile.GetBufSize() )
			{
				m_szSendBuf.erase( 0, nProcessSize );
				break;
			}
		}

		// 处理延时接收  
		while( !m_szRecvBuf.empty() && IsMsgDispatchEnable() )
		{
			int64 nRecvTime;
			uint32 nTotalSize = (uint32)m_szRecvBuf.size();
			CBufFile RecvBufFile( (const tbyte*)m_szRecvBuf.c_str(), nTotalSize );
			RecvBufFile.Read( &nRecvTime, sizeof(int64) );
			if( nCurTime < nRecvTime && m_nMaxDelay )
				break;

			size_t nSize;
			RecvBufFile.Read( &nSize, sizeof(size_t) );
			uint32 nCurPos = RecvBufFile.GetPos();
			const void* pCmd = RecvBufFile.GetBuf() + nCurPos;
			size_t nProcessSize = Process( (const char *)pCmd, nSize );

			// 本数据段全部接收完毕，可以删除
			if( nProcessSize == nSize )
			{
				m_szRecvBuf.erase( 0, nCurPos + nProcessSize );
				continue;
			}

			// 如果没有后续的数据
			if( nCurPos + nSize == nTotalSize )
			{
				// 移除已经处理的数据，并且调整nSize的值
				if( nProcessSize )
				{
					m_szRecvBuf.erase( nCurPos, nProcessSize );
					*(size_t*)&m_szRecvBuf[sizeof(int64)] = nSize - nProcessSize;
				}
				// 跳出循环，下次处理
				break;
			}
			
			// 如果有后续的数据，合并剩余数据到后续的数据段
			size_t nHeadSize = sizeof(int64) + sizeof(size_t);
			size_t nEraseBegin = nCurPos + (uint32)nSize;
			size_t nEraseCount = nHeadSize;
			size_t nRemainCount = (uint32)nSize - (uint32)nProcessSize;
			memcpy( &m_szRecvBuf[0], &m_szRecvBuf[nEraseBegin], nHeadSize );
			if( nProcessSize )
			{
				size_t nRemainBegin = nHeadSize + (uint32)nProcessSize;
				memmove( &m_szRecvBuf[nHeadSize], &m_szRecvBuf[nRemainBegin], nRemainCount );
				nEraseCount += nProcessSize;
				nEraseBegin -= nProcessSize;
			}
			m_szRecvBuf.erase( nEraseBegin, nEraseCount );
			*(size_t*)&m_szRecvBuf[sizeof(int64)] += nRemainCount;
		}

		return !m_szSendBuf.empty() || !m_szRecvBuf.empty() || m_nMaxDelay;	
	}

	void CConnection::EnableMsgDispatch( bool bEnable )
	{
		if( ( m_nEnableDispatchState != eES_Disable ) == bEnable )
			return;
		m_nEnableDispatchState = (uint8)( bEnable ? eES_Pending : eES_Disable );
		if( m_nEnableDispatchState == eES_Disable )
			return;
		if( TTinyList<CConnection>::CTinyListNode::IsInList() )
			return;
		m_pConnMgr->AddUpdateConn( *this );
	}

	void CConnection::SetNetDelay( uint32 nMinDelay, uint32 nMaxDelay )
	{
		m_nMinDelay = nMinDelay;
		m_nMaxDelay = nMaxDelay;
		if( m_nMaxDelay < m_nMinDelay )
			std::swap( m_nMinDelay, m_nMaxDelay );
		if( TTinyList<CConnection>::CTinyListNode::IsInList() )
			return;
		m_pConnMgr->AddUpdateConn( *this );
	}

	void CConnection::EnableProfile( uint8 nSendIDBits, uint8 nRecvIDBits )
	{
		GammaAst( nSendIDBits <= 16 && nRecvIDBits <= 16 );
		m_vecTotalSendSize.resize( nSendIDBits ? 1 << AligenUp( nSendIDBits, 8 ) : 0 );
		m_vecTotalRecvSize.resize( nRecvIDBits ? 1 << AligenUp( nRecvIDBits, 8 ) : 0 );
	}

	size_t CConnection::GetSendBufferSize( uint16 nShellID )
	{
		return nShellID >= m_vecTotalSendSize.size() ? 0 : m_vecTotalSendSize[nShellID];
	}

	size_t CConnection::GetRecvBufferSize( uint16 nShellID )
	{
		return nShellID >= m_vecTotalRecvSize.size() ? 0 : m_vecTotalRecvSize[nShellID];
	}

	size_t CConnection::GetTotalSendSize()
	{
		return m_pConnecter ? m_pConnecter->GetTotalSendSize() : 0;
	}

	size_t CConnection::GetTotalRecvSize()
	{
		return m_pConnecter ? m_pConnecter->GetTotalRecvSize() : 0;
	}

	bool CConnection::IsGraceClose() const
	{
		return m_eCloseType == eCT_Grace;
	}

	bool CConnection::IsForceClose() const
	{
		return m_eCloseType == eCT_Force;
	}

	const char* CConnection::GetCloseLog() const
	{
		return m_strCloseLog.c_str();
	}

}
