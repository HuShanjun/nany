
#include "GammaCommon/TGammaStrStream.h"
#include "GammaCommon/GammaHttp.h"
#include "GammaCommon/GammaMd5.h"
#include "GammaCommon/CPkgFile.h"
#include "GammaCommon/CThread.h"
#include "GammaCommon/SHA1.h"
#include "CConnectionMgr.h"
#include "CWebConnection.h"
#include "WebSocketPrt.h"

namespace Gamma
{
	CWebConnection::CWebConnection( CConnectionMgr* pConnMgr, 
		IConnecter* pConnect, CBaseConn* pBase, const char* szConnectAddress )
		: CConnection( pConnMgr, NULL, pBase, szConnectAddress, true )
		, m_nRecvCount( 0 )
		, m_nSendCount( 0 )
		, m_nPingDelay( 0 )
		, m_nHeartBeatInterval( 1 )
		, m_nPreSendTime( 0 )
		, m_nPreUpdateTime( 0 )
		, m_pConnectPending( pConnect )
		, m_nBufferID( INVALID_32BITID )
		, m_strHostName( szConnectAddress ? szConnectAddress :  "" )
	{
		m_nHeartBeatInterval = Limit<uint32_t>( pConnMgr->GetAutoDisconnectTime()/10, 1, 10 );
		m_pConnectPending->SetHandler( this );
	}

	CWebConnection::~CWebConnection(void)
	{
	}	

	template<> 
	void CWebConnection::OnNetMsg( const CWS_Empty* pCmd )
	{
	}

	template<> 
	void CWebConnection::OnNetMsg( const CWS_Text* pCmd )
	{
		const void* pBuffer = m_szAligenBuf.c_str();
		uint32_t nLen = (uint32_t)m_szAligenBuf.size();
		AddRecvSize( *(const uint16_t*)pBuffer, nLen );
		GetHandler()->OnShellMsg( pBuffer, nLen, false );
	}

	template<> 
	void CWebConnection::OnNetMsg( const CWS_Binary* pCmd )
	{
		const void* pBuffer = m_szAligenBuf.c_str();
		uint32_t nLen = (uint32_t)m_szAligenBuf.size();
		AddRecvSize( *(const uint16_t*)pBuffer, nLen );
		GetHandler()->OnShellMsg( pBuffer, nLen, false );
	}

	template<> 
	void CWebConnection::OnNetMsg( const CWS_Close* pCmd )
	{
		ShutDown( false, "websocket closs" );;
	}

	template<> 
	void CWebConnection::OnNetMsg( const CWS_Ping* pCmd )
	{
		if( !IsConnected() )
			return;

		SSendBuf NetData( m_szAligenBuf.c_str(), m_szAligenBuf.size() );
		SendFrameData( CWS_Pong(), false, &NetData, 1 );
	}

	template<> 
	void CWebConnection::OnNetMsg( const CWS_Pong* pCmd )
	{
		m_nPingDelay = (uint32_t)( GetGammaTime() - m_nPreSendTime );
		m_nPreSendTime = 0;
	}

	template<class MsgType>
	void CWebConnection::OnProcessMsg( const MsgType* pCmd, size_t nSize )
	{
		if( m_nBufferID != INVALID_32BITID && m_nBufferID != pCmd->GetId() )
			return ShutDown( false, "invalid append frame" );

		if( m_nBufferID == INVALID_32BITID )
			m_szAligenBuf.clear();
		m_szAligenBuf.reserve( m_szAligenBuf.size() + nSize );
		m_nBufferID = pCmd->GetId();

		char* pExtraBuffer = (char*)(pCmd + 1);
		uint64_t nExtraSize = nSize - sizeof(MsgType);
		uint64_t nDecode = DecodeWebSocketProtocol( pCmd, pExtraBuffer, nExtraSize );
		GammaAst( nDecode != INVALID_64BITID );
		m_szAligenBuf.append( pExtraBuffer, (uint32_t)nExtraSize );
		if( pCmd->m_bFinished == 0 )
			return;
		m_nBufferID = INVALID_32BITID;
		OnNetMsg( pCmd );
	}
	
	CWebConnection::MsgCheckFun_t CWebConnection::GetCheckFun( size_t nIndex )
	{
		struct __{ static size_t Check( CWebConnection*, const void*, size_t ){ return eCR_Again; } };
		if( !IsMsgDispatchEnable() )
			return &__::Check;
		return CWebConnBase::GetCheckFun( nIndex );
	}

	void CWebConnection::RegisterMsgHandler()
	{
		std::vector<CMsgFunction>& vecMsgFun = CWebConnBase::GetMsgFunction();
		if( !vecMsgFun.empty() )
			return;

		vecMsgFun.resize( 256 );
		CWebConnBase::RegistProcessFun( &CWebConnection::OnProcessMsg<CWS_Empty> );
		CWebConnBase::RegistProcessFun( &CWebConnection::OnProcessMsg<CWS_Text> );
		CWebConnBase::RegistProcessFun( &CWebConnection::OnProcessMsg<CWS_Binary> );
		CWebConnBase::RegistProcessFun( &CWebConnection::OnProcessMsg<CWS_Close> );
		CWebConnBase::RegistProcessFun( &CWebConnection::OnProcessMsg<CWS_Ping> );
		CWebConnBase::RegistProcessFun( &CWebConnection::OnProcessMsg<CWS_Pong> );
	}	

	//===========================================================================
	// MsgHandler Register
	//===========================================================================
	void CWebConnection::OnConnected()
	{
		if( IsServer() )
			return;

		char strID[256];
		gammasstream ssID(strID);
		ssID << m_strHostName
			<< "_" << GetLocalAddress().GetAddress() 
			<< "_" << GammaGetCurrentProcessID() 
			<< "_" << GetNatureTime() 
			<< "_" << this;
		uint8_t szResult[16];
		MD5( (uint8_t*)szResult, strID, ssID.GetCurPos() );

		char szBuffer[512];
		char szUrl[256];
		gammasstream(szUrl)
			<< "http://" << m_strHostName << ":"
			<< m_pConnectPending->GetRemoteAddress().GetPort()
			<< "/chat";
		uint32_t nSize = MakeWebSocketShakeHand( szBuffer, 
			ELEM_COUNT(szBuffer), szResult, szUrl );
		m_pConnectPending->Send( szBuffer, nSize );
	}

	size_t CWebConnection::Dispatch( const char* pBuf, size_t nSize )
	{
		// 重置接收时间    
		m_nRecvCount = 0;
		if( !IsMsgDispatchEnable() )
			return 0;

		// 握手阶段
		if( m_pConnectPending )
			return CheckShakeHand( nSize, pBuf );
		// 消息处理阶段
		return CWebConnBase::Dispatch( pBuf, nSize );
	}

	void CWebConnection::OnCheckTimeOut()
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

	void CWebConnection::OnHeartBeatStop()
	{
		GetHandler()->OnHeartBeatStop();
		GammaLog << "HeartBeatStop ( " << GetRemoteAddress().GetAddress()
			<< ":" << GetRemoteAddress().GetPort() << endl;
		// 直接调用基类的ShutDown，那么连接断最终由底层触发  
		CWebConnection::ShutDown( false, "CWebConnection::OnHeartBeatStop" );
	}

	uint32_t CWebConnection::GetPingDelay() const
	{
		if( m_nPreSendTime == 0 )
			return m_nPingDelay;
		uint32_t nDelay = (uint32_t)( GetGammaTime() - m_nPreSendTime );
		return Max( m_nPingDelay, nDelay );
	}

	void CWebConnection::SetHeartBeatInterval( uint32_t nSeconds )
	{
		m_nHeartBeatInterval = nSeconds;
	}
	
	void CWebConnection::SendHeartBeatMsg()
	{
		if( !IsConnected() )
			return;
		int64_t nCurTime = GetGammaTime();
		m_nSendCount = 0;
		m_nPreSendTime = nCurTime;
		SendFrameData( CWS_Ping(), false, NULL, 0 );
	}
	
	void CWebConnection::SendShellMsg( bool bReliable, 
		const SSendBuf aryBuffer[], uint32_t nBufferCount )
	{
		if( !IsEnableSendShellMsg() || !IsConnected() )
			return;
		SendFrameData( CWS_Binary(), true, aryBuffer, nBufferCount );
	}

	void CWebConnection::SendFrameData( SWebSocketMsg MsgHead, 
		bool bShell, const SSendBuf* u8, uint32_t nBufferCount )
	{
		m_szSendBuff.clear();
		for( uint32_t i = 0; i < nBufferCount; i++ )
			m_szSendBuff.append( (const char*)u8[i].first, u8[i].second );

		union { uint64_t u64; uint8_t u8[sizeof(uint64_t)]; } Size;
		Size.u64 = (uint32_t)m_szSendBuff.size();
		if( !m_szSendBuff.empty() && bShell )
			AddSendSize( *(const uint16_t*)u8[0].first, (uint32_t)Size.u64 );

		MsgHead.m_bMask = !IsServer();
		uint32_t nMask = EncodeWebSocketProtocol( MsgHead, &m_szSendBuff[0], Size.u64 );

		SendBuffer( &MsgHead, sizeof(MsgHead) );
		if( Size.u64 >= 65536 )
		{
			uint8_t aryBigEnd64[] = { Size.u8[7], Size.u8[6], Size.u8[5], 
				Size.u8[4], Size.u8[3], Size.u8[2], Size.u8[1], Size.u8[0] };
			SendBuffer( aryBigEnd64, sizeof(aryBigEnd64) );
		}
		else if( Size.u64 >= 126 )
		{
			uint8_t aryBigEnd16[] = { Size.u8[1], Size.u8[0] };
			SendBuffer( aryBigEnd16, sizeof(aryBigEnd16) );
		}

		if( MsgHead.m_bMask )
			SendBuffer( &nMask, sizeof(nMask) );
		if( !Size.u64 )
			return;
		SendBuffer( m_szSendBuff.c_str(), Size.u64 );
	}

	size_t CWebConnection::CheckShakeHand( const size_t nSize, const char* pBuf )
	{
		const char* szKey;
		uint32_t nKeyLen;
		uint32_t nReadCount = WebSocketShakeHandCheck( 
			pBuf, nSize, IsServer(), szKey, nKeyLen );

		if( nReadCount == INVALID_32BITID )
		{
			std::swap( m_pConnecter, m_pConnectPending );
			ShutDown( false, szKey );
			return nSize;
		}

		if( nReadCount == 0 )
			return 0;

		std::swap( m_pConnecter, m_pConnectPending );
		if( IsServer() )
		{
			char szBuffer[256];
			uint32_t nSendSize = MakeWebSocketServerShakeHandResponese( 
				szBuffer, ELEM_COUNT(szBuffer), szKey, nKeyLen );			
			SendBuffer( szBuffer, nSendSize );
		}

		try
		{
			uint32_t nCurInterval = GetHeartBeatInterval();
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

		return nReadCount;
	}
}
