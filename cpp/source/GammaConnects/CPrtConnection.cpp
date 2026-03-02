
#include "GammaCommon/TGammaStrStream.h"
#include "GammaCommon/CPkgFile.h"
#include "CConnectionMgr.h"
#include "CPrtConnection.h"
#include "GammaPrt.h"
#include "ikcp.h"

namespace Gamma
{
	CPrtConnection::CPrtConnection( CConnectionMgr* pConnMgr, 
		IConnecter* pConnect, CBaseConn* pBase, 
		const char* szConnectAddress, bool bTcp )
		: CConnection( pConnMgr, pConnect, pBase, szConnectAddress, bTcp )
		, m_pKCP( NULL )
		, m_nKCPBufferSize( 0 )
		, m_nNextKcpUpdateTime( 0 )
		, m_nRecvCount( 0 )
		, m_nSendCount( 0 )
		, m_nPingDelay( 0 )
		, m_nHeartBeatInterval( 1 )
		, m_nPreSendTime( 0 )
		, m_nPreUpdateTime( 0 )
	{
		m_nHeartBeatInterval = Limit<uint32>( pConnMgr->GetAutoDisconnectTime()/10, 1, 10 );
		if( bTcp || !GetConn() )
			return;

		struct KCP
		{
			static int KcpCallback( const char *buf, int len, ikcpcb* kcp, void* user )
			{
				GammaAst( len < MAX_RAW_UPD_SIZE );
				char szBuffer[sizeof(CGC_ShellMsg8) + MAX_RAW_UPD_SIZE];
				uint8 nId = (uint8)( len >> 8 ) + MAX_RAW_UPD_SIZE/256;
				CGC_ShellMsg8& NetData = *( new( szBuffer )CGC_ShellMsg8( nId ) );
				NetData.nMsgLen = (uint8)( len );
				memcpy( szBuffer + sizeof( CGC_ShellMsg8 ), buf, len );
				len = len + sizeof(CGC_ShellMsg8);
				( (CPrtConnection*)user )->SendBuffer( false, szBuffer, len );
				return len;
			}
		};
		const SKcpConfig & config = pConnMgr->KcpConfig();
		m_pKCP = ikcp_create( config.KCPCONFIG_CONV, this );
		ikcp_wndsize( m_pKCP, config.KCPCONFIG_SENDWND, config.KCPCONFIG_RECVWND );
		ikcp_setoutput( m_pKCP, &KCP::KcpCallback );
		ikcp_nodelay( m_pKCP, 1, config.KCPCONFIG_TICK, config.KCPCONFIG_RESEND, 1 );
		ikcp_setmtu( m_pKCP, MAX_RAW_UPD_SIZE - 1 );
		m_pKCP->rx_minrto = config.KCPCONFIG_RTO;
		pConnMgr->AddUpdateConn( *this );
	}

	CPrtConnection::~CPrtConnection(void)
	{
		if( m_pKCP != NULL && m_pKCP )
			ikcp_release( m_pKCP );
		m_pKCP = NULL;		
	}	

	//===========================================================================
	// MsgHandler
	//===========================================================================
	template<> 
	void CPrtConnection::OnNetMsg( const CGC_ShellMsg8* pCmd, size_t nSize )
	{
		if( !pCmd->nMsgLen && !pCmd->GetId() )
			return;

		uint32 nLen = pCmd->nMsgLen;
		const char* szBuffer = (const char*)( pCmd + 1 );

		if( !m_pKCP )
		{
			nLen += pCmd->GetId() << 8;
			szBuffer = (const char*)AligenBuffer( szBuffer, nLen );
			AddRecvSize( *(const uint16*)szBuffer, nLen );
			GetHandler()->OnShellMsg( szBuffer, nLen, false );
			return;
		}

		bool bInKcpBuffer = szBuffer >= &m_strKCPRecvBuffer[0] && 
			szBuffer < &m_strKCPRecvBuffer[m_nKCPBufferSize];

		if( pCmd->GetId() >= MAX_RAW_UPD_SIZE/256 && !bInKcpBuffer )
		{
			nLen += ( pCmd->GetId() - MAX_RAW_UPD_SIZE/256 ) << 8;
			ikcp_input( m_pKCP, szBuffer, nLen );
			return;
		}

		nLen += pCmd->GetId() << 8;
		const void* pBuffer = (const char*)AligenBuffer( szBuffer, nLen );
		AddRecvSize( *(const uint16*)pBuffer, nLen );
		GetHandler()->OnShellMsg( pBuffer, nLen, !bInKcpBuffer );
	}

	template<> 
	void CPrtConnection::OnNetMsg( const CGC_ShellMsg32* pCmd, size_t nSize )
	{
		uint32 nLen;
		memcpy( &nLen, &pCmd->nMsgLen, sizeof(uint32) );
		if( !nLen )
			return;

		const void* pBuffer = AligenBuffer( pCmd + 1, nLen );
		AddRecvSize( *(const uint16*)pBuffer, nLen );
		GetHandler()->OnShellMsg( pBuffer, nLen, false );
	}

	template<> 
	void CPrtConnection::OnNetMsg( const CGC_HeartbeatMsg* pCmd, size_t nSize )
	{
		if( !IsConnected() )
			return;
		CGC_HeartbeatReply NetMsg;
		SendBuffer( true, &NetMsg, sizeof(NetMsg) );
	}

	template<> 
	void CPrtConnection::OnNetMsg( const CGC_HeartbeatReply* pCmd, size_t nSize )
	{
		m_nPingDelay = (uint32)( GetGammaTime() - m_nPreSendTime );
		m_nPreSendTime = 0;
	}

	CPrtConnection::MsgCheckFun_t CPrtConnection::GetCheckFun( size_t nIndex )
	{
		struct __{ static size_t Check( CPrtConnection*, const void*, size_t ){ return eCR_Again; } };
		if( !IsMsgDispatchEnable() )
			return &__::Check;
		return TDispatch<CPrtConnection, uint8>::GetCheckFun( nIndex );
	}

	//===========================================================================
	// MsgHandler Register
	//===========================================================================
	void CPrtConnection::RegisterMsgHandler()
	{
		std::vector<CMsgFunction>& vecMsgFun = TDispatch<CPrtConnection, uint8>::GetMsgFunction();
		if( !vecMsgFun.empty() )
			return;

		struct SCheckMsg8
		{
			static size_t CheckMsg( CPrtConnection* pClass, const void* pData, size_t nSize )
			{
				if( nSize < CGC_ShellMsg8::GetHeaderSize() )
					return (size_t)eCR_Again;

				const CGC_ShellMsg8* pMsg = static_cast<const CGC_ShellMsg8*>(pData);
				size_t nExtraSize = pMsg->GetExtraSize( nSize );
				if( pClass->m_pKCP )
				{
					const char* szBuffer = (const char*)pData;
					bool bInKcpBuffer = szBuffer >= &pClass->m_strKCPRecvBuffer[0] && 
						szBuffer < &pClass->m_strKCPRecvBuffer[pClass->m_nKCPBufferSize];
					if( !bInKcpBuffer && pMsg->GetId() >= MAX_RAW_UPD_SIZE/256 )
						nExtraSize -= MAX_RAW_UPD_SIZE;
				}

				if( nExtraSize >= CPrtConnection::eMaxRecivesize )
					return (size_t)eCR_Error;

				size_t nCmdSize = nExtraSize + CGC_ShellMsg8::GetHeaderSize();
				if( nCmdSize > nSize )
					return (size_t)eCR_Again;
				return nCmdSize;
			}
		};

		vecMsgFun.resize( 256 );
		typedef void (CPrtConnection::*CGC_ShellMsg8Fun)( const CGC_ShellMsg8*, size_t );
		CGC_ShellMsg8Fun funShellMsg = &CPrtConnection::OnNetMsg<CGC_ShellMsg8>;
		for( uint8 i = 0; i < eGC_ShellMsg32; i++ )
		{
			vecMsgFun[i].first = &SCheckMsg8::CheckMsg;
			vecMsgFun[i].second = (MsgProcessFun_t)funShellMsg;
			vecMsgFun[i].szName = CGC_ShellMsg8::GetName();
			vecMsgFun[i].nHeadSize = sizeof( CGC_ShellMsg8 );
		}

		TDispatch<CPrtConnection, uint8>::RegistProcessFun( &CPrtConnection::OnNetMsg<CGC_ShellMsg32> );
		TDispatch<CPrtConnection, uint8>::RegistProcessFun( &CPrtConnection::OnNetMsg<CGC_HeartbeatMsg> );
		TDispatch<CPrtConnection, uint8>::RegistProcessFun( &CPrtConnection::OnNetMsg<CGC_HeartbeatReply> );
	}	

	size_t CPrtConnection::Dispatch( const char* pBuf, size_t nSize )
	{
		// 重置接收时间    
		m_nRecvCount = 0;
		if( !IsMsgDispatchEnable() )
			return 0;
		return TDispatch<CPrtConnection, uint8>::Dispatch( pBuf, nSize );
	}

	void CPrtConnection::OnCheckTimeOut()
	{
		if( !IsConnected() )
			return;

		m_nRecvCount++;
		m_nSendCount++;

		// 在这里检查是否需要发心跳消息   
		if( m_nSendCount > m_nHeartBeatInterval && !m_nPreSendTime )
			SendHeartBeatMsg();

		// 在这里检查空闲是否太久没接收到远端消息，超过一定时间将远端断掉   
		if( GetConnMgr()->GetAutoDisconnectTime() < m_nRecvCount )
			OnHeartBeatStop();
	}

	void CPrtConnection::OnHeartBeatStop()
	{
		GetHandler()->OnHeartBeatStop();
		GammaLog << "HeartBeatStop ( " << GetRemoteAddress().GetAddress()
			<< ":" << GetRemoteAddress().GetPort() << endl;
		// 直接调用基类的ShutDown，那么连接断最终由底层触发  
		CPrtConnection::ShutDown( false, "CPrtConnection::OnHeartBeatStop" );
	}

	uint32 CPrtConnection::GetPingDelay() const
	{
		if( m_nPreSendTime == 0 )
			return m_nPingDelay;
		uint32 nDelay = (uint32)( GetGammaTime() - m_nPreSendTime );
		return Max( m_nPingDelay, nDelay );
	}

	void CPrtConnection::SetHeartBeatInterval( uint32 nSeconds )
	{
		m_nHeartBeatInterval = nSeconds;
	}
	
	void CPrtConnection::SendHeartBeatMsg()
	{
		if( !IsConnected() )
			return;
		int64 nCurTime = GetGammaTime();
		m_nSendCount = 0;
		m_nPreSendTime = nCurTime;
		CGC_HeartbeatMsg NetMsg;
		SendBuffer( true, &NetMsg, sizeof(NetMsg) );
	}
	
	void CPrtConnection::SendShellMsg( bool bReliable, const SSendBuf aryBuffer[], uint32 nBufferCount )
	{
		if( !IsEnableSendShellMsg() || !IsConnected() )
			return;

		size_t nTotalSize = 0;
		for( uint32 i = 0; i < nBufferCount; i++ )
			nTotalSize += aryBuffer[i].second;

		AddSendSize( *(const uint16*)aryBuffer[0].first, nTotalSize );

		if( !m_pKCP || bReliable )
		{
			if( nTotalSize < eGC_ShellMsg32*256 )
			{
				uint8 nId = (uint8)( nTotalSize >> 8 );
				CGC_ShellMsg8 NetData( nId );
				NetData.nMsgLen = (uint8)( nTotalSize );
				SendBuffer( true, &NetData, sizeof(NetData) );
			}
			else
			{
				GammaAst( nTotalSize <= INVALID_32BITID );
				CGC_ShellMsg32 NetData;
				NetData.nMsgLen = (uint32)nTotalSize;
				SendBuffer( true, &NetData, sizeof(NetData) );
			}

			for( uint32 i = 0; i < nBufferCount; i++ )
				SendBuffer( true, aryBuffer[i].first, aryBuffer[i].second );
		}
		else
		{
			GammaAst( nTotalSize < MAX_RAW_UPD_SIZE );
			char szBuffer[sizeof(CGC_ShellMsg8) + MAX_RAW_UPD_SIZE];
			uint8 nId = (uint8)( nTotalSize >> 8 );
			CGC_ShellMsg8& NetData = *( new( szBuffer )CGC_ShellMsg8( nId ) );
			NetData.nMsgLen = (uint8)( nTotalSize );
			for( uint32 i = 0, n = sizeof( CGC_ShellMsg8 ); i < nBufferCount; i++ )
			{
				memcpy( szBuffer + n, aryBuffer[i].first, aryBuffer[i].second );
				n += (uint32)aryBuffer[i].second;
			}

			nTotalSize += sizeof(CGC_ShellMsg8);
			SendBuffer( false, szBuffer, nTotalSize );
		}
	}

	bool CPrtConnection::OnUpdate()
	{
		if( m_pKCP )
		{
			uint32 nCurTime = (uint32)GetGammaTime();
			uint32 nDataSize = (uint32)m_strKCPSendBuffer.size();
			uint32 nInterval = GetConnMgr()->KcpConfig().KCPCONFIG_UPDATE;
			if( ( nDataSize && nCurTime - m_nPreUpdateTime > nInterval ) ||
				nDataSize >= MAX_RAW_UPD_SIZE )
			{
				ikcp_send( m_pKCP, m_strKCPSendBuffer.c_str(), nDataSize );
				m_strKCPSendBuffer.clear();
				m_nPreUpdateTime = nCurTime;
			}

			if( m_nNextKcpUpdateTime == 0 )
				m_nNextKcpUpdateTime = ikcp_check( m_pKCP, nCurTime );

			if( nCurTime >= m_nNextKcpUpdateTime )
			{
				m_nNextKcpUpdateTime = 0;
				ikcp_update( m_pKCP, nCurTime );
			}

			while( true )
			{
				uint32 nLeftSize = (uint32)m_strKCPRecvBuffer.size() - m_nKCPBufferSize;
				int32 nPeekSize = ikcp_peeksize( m_pKCP );
				if( nPeekSize < 0 )
					break;
				if( (int32)nLeftSize < nPeekSize )
					m_strKCPRecvBuffer.resize( m_nKCPBufferSize + nPeekSize + 1024 );
				char* szBuffer = &m_strKCPRecvBuffer[m_nKCPBufferSize];
				int32 nRecvSize = ikcp_recv( m_pKCP, szBuffer, nPeekSize + 1024 );
				if( nRecvSize <= 0 )
					break;
				m_nKCPBufferSize += nRecvSize;
			}

			if( m_nKCPBufferSize )
			{
				try
				{
					size_t nSize = Dispatch( &m_strKCPRecvBuffer[0], m_nKCPBufferSize );				
					if( nSize != m_nKCPBufferSize )
						memcpy( &m_strKCPRecvBuffer[0], &m_strKCPRecvBuffer[nSize], m_nKCPBufferSize - nSize );
					m_nKCPBufferSize -= (uint32)nSize;
				}
				catch( ... )
				{
					char szBuf[256];
					gammasstream( szBuf, ELEM_COUNT(szBuf) ) << "Dispatch Msg Error ( "
						<< GetRemoteAddress().GetAddress() << ":"
						<< GetRemoteAddress().GetPort() << ", " << m_nKCPBufferSize;

					string szLog = szBuf;
					for( size_t i = 0; i < 32 && i < m_nKCPBufferSize; i++ )
					{
						char c1 = (char)( ( (uint8)m_strKCPRecvBuffer[i] ) & 0xf );
						char c2 = (char)( ( (uint8)m_strKCPRecvBuffer[i] ) >> 4 );
						szLog.push_back( c2 + ( c2 > 9 ? ( 'a' - 10 ) : '0' ) );
						szLog.push_back( c1 + ( c1 > 9 ? ( 'a' - 10 ) : '0' ) );
						szLog.push_back( ' ' );
					}
					szLog.push_back( ']' );
					GammaErr << szLog.c_str() << endl;

					ShutDown( false, "CPrtConnection::OnUpdate" );
					return false;
				}
			}
		}

		return CConnection::OnUpdate() || m_pKCP;
	}


}
