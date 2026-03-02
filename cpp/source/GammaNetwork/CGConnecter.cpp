
#include "CGNetBuffer.h"
#include "GammaNetworkHelp.h"
#include "CAddrResolution.h"
#include "CGNetwork.h"
#include "CGConnecter.h"
#include "CGListener.h"
#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/GammaProfile.h"
#include "GammaCommon/CGammaRand.h"
#include "GammaCommon/GammaTime.h"
#include <assert.h>
#include <sstream>

#ifndef _WIN32
#include <alloca.h>
#endif

using namespace std;

namespace Gamma
{
	//========================================================
	// SOCKET连接类构造函数
	//========================================================
	CGConnecter::CGConnecter( CGNetwork* pNetwork, CGSocket* pSocket )
		: m_pNetwork( pNetwork )
		, m_pSocket( pSocket )
		, m_pHandler( NULL )
		, m_eCloseType( eCE_Null )
		, m_eState( eCS_Connecting )
		, m_bEverConnected( false )
		, m_nTotalSend( 0 )
		, m_nTotalRecv( 0 )
		, m_nCreateTime( GetNatureTime() )
		, m_nConnectTime( 0 )
		, m_nFirstEvenTime( 0 )
		, m_nConnectOKTime( 0 )
	{
		GammaAst( m_pNetwork && m_pSocket );
		m_pSocket->Bind( this );
		if( m_pSocket->GetLocalAddress().empty() ||
			m_pSocket->GetRemoteAddress().empty() )
			return;
		// 运行到这里表示这是Accept产生的连接
		SetState( eCS_Connecting );
	}

	CGConnecter::~CGConnecter()
	{
		GammaAst( m_pNetwork && m_pSocket );
		SAFE_RELEASE( m_pSocket );
		m_pSocket = nullptr;
		m_pNetwork = nullptr;
	}

	bool CGConnecter::IsValid()
	{
		return m_pNetwork != nullptr;
	}

	void CGConnecter::Connect( CAddrResolutionDelegate* pAddrResolution )
	{
		m_nConnectTime = GetNatureTime();
		GammaAst( m_eState == eCS_Connecting );
		if( !pAddrResolution->IsResolveSuceeded() )
		{
			Close( eCE_Unreach );
			return;
		}

		uint16_t nPort = m_RemoteAddress.GetPort();
		bool bUdp = GetConnectType() == eConnecterType_UDP;
		const UAddressInfo* pInfo = pAddrResolution->GetAddressBuff( bUdp );
		if( !pInfo )
		{
			Close( eCE_Unreach );
			return;
		}

		UAddressInfo Info = *pInfo;
		if( Info.IPv4.sin_family == AF_INET )
			( (sockaddr_in*)&Info.IPv4 )->sin_port = htons( (u_short)nPort );
		else if( Info.IPv6.sin6_family == AF_INET6 )
			( (sockaddr_in6*)&Info.IPv6 )->sin6_port = htons( (u_short)nPort );
		uint32_t nBufferSize = Info.IPv4.sin_family == AF_INET ?
			sizeof( sockaddr_in ) : sizeof( sockaddr_in6 );

		m_RemoteAddress = MakeAddress( (sockaddr*)&Info, nBufferSize );
		auto eError = m_pSocket->Connect( (sockaddr*)&Info, nBufferSize );
		if( eError == eCE_Null )
			return;
		Close( eError );
	}

	void CGConnecter::CmdClose( bool bGrace )
	{
		Close( bGrace ? eCE_GraceClose : eCE_ForceClose );
	}

	void CGConnecter::Close( ECloseType eCloseType )
	{ 
		if( m_eState > eCS_Disconnecting ||
			m_eState == eCS_Disconnecting && eCloseType != eCE_ShutdownOnCheck )
			return;

		m_eCloseType = eCloseType; 
		if( m_eCloseType != eCE_ShutdownOnCheck )
		{
			SetState( eCS_Disconnecting );
			return m_pNetwork->AddDisConnSocket( this );
		}

		GammaAst( m_eState == eCS_Disconnecting );
		SetState( eCS_Disconnected );
		if( m_pHandler )
		{
			IConnectHandler* pHandler = static_cast<IConnectHandler*>(m_pHandler);
			m_pHandler = NULL;
			pHandler->OnDisConnect( m_eCloseType );
		}
		m_pNetwork->ReleaseConnect( this );
	}

	void CGConnecter::SetHandler( IConnectHandler* pHandler )
	{
		m_pHandler = pHandler;
	}

	const Gamma::CAddress& CGConnecter::GetLocalAddress() const
	{
		if( m_LocalAddress.GetPort() )
			return m_LocalAddress;
		GammaAst( m_pSocket );
		auto& strLocalAddressBuffer = m_pSocket->GetLocalAddress();
		auto pAddr = (const sockaddr*)strLocalAddressBuffer.c_str();
		auto& Address = const_cast<CAddress&>(m_LocalAddress);
		Address = MakeAddress( pAddr, strLocalAddressBuffer.size() );
		return m_LocalAddress;
	}

	const Gamma::CAddress& CGConnecter::GetRemoteAddress() const
	{
		if (m_RemoteAddress.GetPort())
			return m_RemoteAddress;
		GammaAst(m_pSocket);
		auto& strLocalAddressBuffer = m_pSocket->GetRemoteAddress();
		auto pAddr = (const sockaddr*)strLocalAddressBuffer.c_str();
		auto& Address = const_cast<CAddress&>(m_RemoteAddress);
		Address = MakeAddress(pAddr, strLocalAddressBuffer.size());
		return m_RemoteAddress;
	}

	void CGConnecter::SetRemoteAddress( const CAddress& Address )
	{
		m_RemoteAddress = Address;
	}

	void CGConnecter::OnEvent( bool bError, const tbyte* pData, uint32_t nSize )
	{
		if( IsConnecting() )
		{
			int64_t nCurTime = 0;
			if( m_nFirstEvenTime == 0 )
				m_nFirstEvenTime = nCurTime = GetNatureTime();

			if( bError )
			{
				Close( eCE_ConnectRefuse );
				return;
			}

			m_bEverConnected = true;
			SetState( eCS_Connected );
			if( nCurTime == 0 )
				nCurTime = GetNatureTime();
			m_nConnectOKTime = nCurTime;
			if( m_pHandler )
				static_cast<IConnectHandler*>(m_pHandler)->OnConnected();
			if( !IsValid() )
				return;
		}

		if( pData == nullptr )
			return;

		if( nSize == 0 )
			Close( eCE_NormalClose );
		else
			RecvData( pData, nSize );
	}

	void CGConnecter::Send( const void* pBuf, size_t nSize )
	{
		if( m_eState >= eCS_Disconnecting )
			return;
		GammaAst( m_pSocket );
		m_pSocket->Send( pBuf, nSize );
		m_nTotalSend += nSize;
	}

	//========================================================
    // SOCKET TCP连接类构造函数
    //========================================================
    CGConnecterTCP::CGConnecterTCP( CGNetwork* pNetwork, CGSocket* pSocket )
        : CGConnecter( pNetwork, pSocket )
    {
    }

    CGConnecterTCP::~CGConnecterTCP()
	{
	}

	void CGConnecterTCP::CheckRecvBuff()
	{
		if( !m_pHandler )
			return;
		auto pBuffer = m_RecvBuf.GetBuffer();
		size_t nTotalSize = m_RecvBuf.GetBufferSize();
		auto nPopSize = m_pHandler->OnRecv( pBuffer, nTotalSize );
		if( !IsValid() )
			return;
		m_RecvBuf.Pop( nPopSize );
	}

	void CGConnecterTCP::RecvData( const tbyte* pData, uint32_t nSize )
	{
		GammaAst( pData && nSize );
		size_t nLeftSize = m_RecvBuf.GetLeftSize();
		if( nLeftSize < nSize )
			m_RecvBuf.Extend( m_RecvBuf.GetBufferSize() + nSize );
		char* pRecvBuf = m_RecvBuf.GetLeftBuffer();
		memcpy( pRecvBuf, pData, nSize );
		m_RecvBuf.Push( nSize );
		m_nTotalRecv += nSize;
		CheckRecvBuff();
	}

	//========================================================
	// SOCKET UDP连接类构造函数
	//========================================================
	CGConnecterUDP::CGConnecterUDP( CGNetwork* pNetwork, CGSocket* pSocket )
		: CGConnecter( pNetwork, pSocket )
	{
	}

	CGConnecterUDP::~CGConnecterUDP()
	{
	}

	void CGConnecterUDP::RecvData( const tbyte* pData, uint32_t nSize )
	{
		if( !m_pHandler )
			return;
		m_pHandler->OnRecv( (const char*)pData, nSize );
		m_nTotalRecv += nSize;
	}
}
