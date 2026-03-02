#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/TTinyObjectPool.h"
#include "CGNetBuffer.h"
#include "GammaNetworkHelp.h"
#include "CGNetwork.h"
#include "CGListener.h"
#include "CGConnecter.h"
#include "CAddrResolution.h"
#include "CGNetThread.h"
#include <sstream>

#ifdef _WIN32
#pragma comment( linker, "/defaultlib:ws2_32.lib" )
#endif

using namespace std;

namespace Gamma
{
	typedef TTinyObjectPool<CGConnecterUDP> CGConnecterUDPPool;
	typedef TTinyObjectPool<CGConnecterTCP> CGConnecterTCPPool;
	typedef TTinyObjectPool<CGListener> CGListenerPool;

	CGNetwork::CGNetwork( uint32 nMaxConnect, uint32 nNetworkThread )
		: m_hLockResolution( GammaCreateLock() )
		, m_sslServerMethod( NULL )
		, m_sslClientMethod( NULL )
		, m_bLog( false )
		, m_pFreeBuffer( nullptr )
	{
#ifdef _WIN32
		WORD wVersion;
		WSADATA wsaData;

		wVersion = MAKEWORD( 2, 2 );
		int32 nResult = WSAStartup( wVersion, &wsaData );
		if( nResult )
		{
			stringstream strm;
			strm << "WSAStartup failed with error code:" << nResult << ends;
			GammaThrow( strm.str() );
		}
#endif
		if( nNetworkThread == INVALID_32BITID )
			nNetworkThread = 2;
		m_vecNetThread.resize( nNetworkThread );
		for( size_t i = 0; i < nNetworkThread; i++ )
			m_vecNetThread[i] = new CGNetThread( this, nMaxConnect );

		SSL_library_init();
		SSL_load_error_strings();
		SSLeay_add_ssl_algorithms();

		m_sslServerMethod = SSLv23_server_method();
		m_sslClientMethod = SSLv23_client_method();
		m_sslClientContext = SSL_CTX_new( m_sslClientMethod );
	}

	CGNetwork::~CGNetwork(void)
	{
		Check( 0 );
		for( size_t i = 0; i < m_vecNetThread.size(); i++ )
			SAFE_DELETE( m_vecNetThread[i] );
		m_vecNetThread.clear();

		while( m_mapAddressReslv.GetFirst() )
			delete m_mapAddressReslv.GetFirst();
		GammaDestroyLock( m_hLockResolution );
		m_hLockResolution = NULL;

#ifdef _WIN32
		int32 nResult = WSACleanup();
		if( nResult )
		{
			stringstream strm;
			strm << "WSACleanup failed with error code:" << nResult << ends;
			GammaThrow( strm.str() );
		}
#endif
		for( auto& Context : m_mapServerContexts )
			SSL_CTX_free( Context.second );
		SSL_CTX_free( m_sslClientContext );
	}

	void CGNetwork::AddDisConnSocket( CDisconnectNode* pDisConnSocket )
	{
		if( pDisConnSocket->IsInList() )
			return;
		m_listDisConnSocket.PushBack( *pDisConnSocket );
	}

	bool CGNetwork::Check( uint32 nTimeOut )
	{
		STATCK_LOG( this );
		while( m_listFinished.GetFirst() )
		{
			GammaLock( m_hLockResolution );
			CAddrResolutionDelegate* pResolution = m_listFinished.GetFirst();
			pResolution->TGammaList<CAddrResolutionDelegate>::CGammaListNode::Remove();
			GammaUnlock( m_hLockResolution );

			while( pResolution->GetFirst() )
			{
				CConnectingNode* pNode = pResolution->GetFirst();
				CGConnecter* pConnect = static_cast<CGConnecter*>( pNode );
				pNode->Remove();
				pConnect->Connect( pResolution );
			}
		}

		/* 断开注册的所有连接 */
		while( m_listDisConnSocket.GetFirst() )
		{
			CDisconnectNode* pNode = m_listDisConnSocket.GetFirst();
			CGConnecter* pConnect = static_cast<CGConnecter*>( pNode );
			pNode->Remove();
			pConnect->Close( eCE_ShutdownOnCheck );
		}

		for( size_t i = 0; i < m_vecNetThread.size(); i++ )
			m_vecNetThread[i]->MainThreadCheck();

		return true;
	}

	bool CGNetwork::PreResolveDomain( const char* szAddress, uint32 nValidSeconds )
	{
		CAddrResolutionDelegate* pResolustion = GetAddressReslv( szAddress );
		pResolustion->SetIPValidTime( nValidSeconds );
		return pResolustion->IsResolveSuceeded();
	}

	IListener* CGNetwork::StartListener( 
		const char* szAddres, uint16 nPort, EConnecterType eType,
		const char* pCertificatePath, const char* pPrivateKeyPath )
	{
		CGSocket* pSocket = nullptr;
		if( eType == eConnecterType_UDP )
			pSocket = new CGSocketUDP( this );
		else if( eType == eConnecterType_TCP )
			pSocket = new CGSocketTCP( this );
		else
		{
			GammaAst( pCertificatePath && pPrivateKeyPath );
			std::string strKey = pCertificatePath;
			strKey += ":";
			strKey += pPrivateKeyPath;
			auto& pContext = m_mapServerContexts[strKey];
			if( pContext == nullptr )
			{
				pContext = SSL_CTX_new( GetSSLServerMethod() );
				GammaAst( pContext );
				if( !SSL_CTX_use_certificate_file( pContext, 
						pCertificatePath, SSL_FILETYPE_PEM ) ||
					!SSL_CTX_use_PrivateKey_file( pContext, 
						pPrivateKeyPath, SSL_FILETYPE_PEM ) ||
					!SSL_CTX_check_private_key( pContext ) )
				{
					ostringstream strm;
					strm << "invalid certificate or key file :"
						<< pCertificatePath << "/" << pPrivateKeyPath << ends;
					GammaThrow( strm.str() );
				}
			}

			pSocket = new CGSocketTCPS( this, pContext );
		}

		auto pListener = CGListenerPool::Inst().Alloc( this, pSocket );
		pListener->Start( szAddres, nPort );
		return pListener;
	}

	IConnecter* CGNetwork::Connect( 
		const char* szAddres, uint16 nPort, EConnecterType eType )
	{
		CGSocket* pSocket = nullptr;
		if( eType == eConnecterType_UDP )
			pSocket = new CGSocketUDP( this );
		else if( eType == eConnecterType_TCP )
			pSocket = new CGSocketTCP( this );
		else
			pSocket = new CGSocketTCPS( this, m_sslClientContext );

		CGConnecter* pConnect = CreateConnecter( pSocket );
		pConnect->SetRemoteAddress( CAddress( "", nPort ) );
		GetAddressReslv( szAddres )->PushBack( *pConnect );
		return pConnect;
	}

	CAddrResolutionDelegate* CGNetwork::GetAddressReslv( const char* szAddres )
	{
		gammacstring strKey( szAddres, true );
		CAddrResolutionDelegate* pResolution = m_mapAddressReslv.Find( strKey );
		if( pResolution == NULL )
		{
			pResolution = new CAddrResolutionDelegate( 
				szAddres, m_hLockResolution, m_listFinished );
			m_mapAddressReslv.Insert( *pResolution );
		}

		pResolution->Resolve();
		return pResolution;
	}

	CGNetThread* CGNetwork::GetMinSocketThread()
	{
		uint32 nSocketCount = INVALID_32BITID;
		CGNetThread* pMinSocketThread = nullptr;
		for( uint32 i = 0; i < m_vecNetThread.size(); i++ )
		{
			if( m_vecNetThread[i]->GetSocketCount() >= nSocketCount )
				continue;
			pMinSocketThread = m_vecNetThread[i];
			nSocketCount = pMinSocketThread->GetSocketCount();
		}
		return pMinSocketThread;
	}

	CGConnecter* CGNetwork::CreateConnecter( CGSocket* pSocket )
	{
		auto eType = pSocket->GetConnectType();
		return eType == eConnecterType_UDP ?
			(CGConnecter*)(CGConnecterUDPPool::Inst().Alloc( this, pSocket )) :
			(CGConnecter*)(CGConnecterTCPPool::Inst().Alloc( this, pSocket ));
	}

	void CGNetwork::ReleaseConnect( CGConnecter* pConnect )
	{
		auto eType = pConnect->GetConnectType();
		if( eType == eConnecterType_UDP )
		{
			auto pConnectUDP = static_cast<CGConnecterUDP*>(pConnect);
			TTinyObjectPool<CGConnecterUDP>::Inst().Free( pConnectUDP );
		}
		else
		{
			auto pConnectTCP = static_cast<CGConnecterTCP*>(pConnect);
			TTinyObjectPool<CGConnecterTCP>::Inst().Free( pConnectTCP );
		}
	}

	void CGNetwork::ReleaseListener( CGListener* pListener )
	{
		CGListenerPool::Inst().Free( pListener );
	}

	SSendBuffer* CGNetwork::Alloc()
	{
		if( m_pFreeBuffer == nullptr )
			return (SSendBuffer*)TFixedPageAlloc<sizeof( SSendBuffer )>::Alloc();
		SSendBuffer* pFreeBuffer = m_pFreeBuffer;
		m_pFreeBuffer = m_pFreeBuffer->pNext;
		return pFreeBuffer;
	}

	void CGNetwork::Free( SSendBuffer* pData )
	{
		pData->pNext = m_pFreeBuffer;
		m_pFreeBuffer = pData;
	}

	INetwork* CreateNetWork( uint32 nMaxConnect, uint32 nNetworkThread )
	{
		return new CGNetwork( nMaxConnect, nNetworkThread );
	}
}
