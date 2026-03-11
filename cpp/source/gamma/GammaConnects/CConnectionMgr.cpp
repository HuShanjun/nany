
#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/GammaTime.h"
#include "CConnectionMgr.h"
#include "CListenHandler.h"
#include "CPrtConnection.h"
#include "CWebConnection.h"

#define CONNECTING_CHECK_TIME	1000
#define DISCONNECT_TIME			5000

namespace Gamma
{
	IConnectionMgr* CreateConnMgr(
		uint32_t nAutoDisconnectTime, bool bStrictMode )
	{
		return new CConnectionMgr( nAutoDisconnectTime, bStrictMode );
	}

	CConnectionMgr::CConnectionMgr( 
		uint32_t nAutoDisconnectTime,	bool bStrictMode )
		: m_pNetWork( CreateNetWork() )
		, m_nAutoDisconnectTime( nAutoDisconnectTime )
		, m_bStrictMode( bStrictMode )
		, m_nCurCheckTime( 0 )
		, m_nRef( 1 )
	{
		m_pNetWork->EnableLog( true );
		m_kcpConfig.KCPCONFIG_CONV = 0xd14d4926;
		m_kcpConfig.KCPCONFIG_UPDATE = 33;
		m_kcpConfig.KCPCONFIG_TICK = 10;
		m_kcpConfig.KCPCONFIG_RESEND = 2;
		m_kcpConfig.KCPCONFIG_SENDWND = 128;
		m_kcpConfig.KCPCONFIG_RECVWND = 128;
		m_kcpConfig.KCPCONFIG_RTO = 10;
		CPrtConnection::RegisterMsgHandler();
		CWebConnection::RegisterMsgHandler();
	}

	CConnectionMgr::~CConnectionMgr()
	{
		for( uint32_t i = 0; i < eCT_Count; i++ )
		{
			for( CConnListMap::iterator it = m_mapConnList[i].begin(); 
				it != m_mapConnList[i].end(); it++ )
				SAFE_DELETE( it->second );
			m_mapConnList[i].clear();
		}

		while( m_listListener.GetFirst() )
			delete m_listListener.GetFirst();
		SAFE_RELEASE( m_pNetWork );
	}

	void CConnectionMgr::PreResolveDomain( const char* szAddress )
	{
		m_pNetWork->PreResolveDomain( szAddress );
	}

	void CConnectionMgr::AddUpdateConn( CConnection& Conn )
	{
		m_listUpdateConn.PushFront( Conn );
	}

	bool CConnectionMgr::Check( uint32_t nWaitTimes )
	{
		STATCK_LOG( this );
		int64_t nCurTime = GetGammaTime();
		if( nCurTime - m_nCurCheckTime > CONNECTING_CHECK_TIME )
		{
			m_nCurCheckTime = nCurTime;
			OnCheckConnecting();
		}

		CConnection* pConn = m_listUpdateConn.GetFirst();
		while( pConn )
		{
			CConnection* pCurConn = pConn;
			pConn = pConn->CConnUpdateList::CTinyListNode::GetNext();
			if( pCurConn->OnUpdate() )
				continue;
			pCurConn->CConnUpdateList::CTinyListNode::Remove();
		}

		return m_pNetWork->Check( nWaitTimes );
	}

	void CConnectionMgr::OnCheckConnecting()
	{
		int64_t nMinCreateTime = m_nCurCheckTime - DISCONNECT_TIME;
		CConnListMap& mapClientConn = m_mapConnList[eCT_Client];
		for( CConnListMap::iterator it = mapClientConn.begin(); 
			it != mapClientConn.end(); it++ )
		{
			CConnList& listConn = *it->second;
			CConnection* pConn = listConn.GetFirst();
			while( pConn )
			{
				CConnection* pCurConn = pConn;
				pConn = pConn->ConnNode::GetNext();
				if( pCurConn->IsDisconnected() )
					pCurConn->OnCheckTimeOut();
				else if( pCurConn->IsDisconnecting() && 
					nMinCreateTime > pCurConn->GetCreateTime() )
					pCurConn->ShutDown( false, "CConnectionMgr::OnCheckConnecting" );
			}
		}

		for( int32_t i = 0; i < eCT_Count; i++ )
		{	
			for( CConnListMap::iterator it = m_mapConnList[i].begin(); 
				it != m_mapConnList[i].end(); it++ )
			{
				CConnList& listConn = *it->second;
				CConnection* pConn = listConn.GetFirst();
				while( pConn )
				{
					CConnection* pCurConn = pConn;
					pConn = pConn->ConnNode::GetNext();
					pCurConn->OnCheckTimeOut();
				}
			}
		}
	}

	void CConnectionMgr::TryShutDownConn( CConnList& listConn )
	{
		CConnection* pConnecting = listConn.GetFirst();
		while( pConnecting )
		{
			CConnection* pCurConnecting = pConnecting;
			pConnecting = pConnecting->ConnNode::GetNext();
			pCurConnecting->TGammaList<CConnection>::CGammaListNode::Remove();

			if( pCurConnecting->IsDisconnected() )
				continue;
			if( pCurConnecting->IsConnected() )
				pCurConnecting->ShutDown( true, "CConnectionMgr::TryShutDownConn" );
			else if( pCurConnecting->IsDisconnecting() || pCurConnecting->IsConnecting() )
				pCurConnecting->ShutDown( false, "CConnectionMgr::TryShutDownConn" );
		}
	}

	void CConnectionMgr::StartService( const char* szAddres, uint16_t nPort, uint32_t nConnectClassID, 
		EConnType eType, const char* pCertificatePath, const char* pPrivateKeyPath )
	{
		if( m_mapConnList[eCT_Server].find( nConnectClassID ) == m_mapConnList[eCT_Server].end() )
			m_mapConnList[eCT_Server][nConnectClassID] = new CConnList;
		EConnecterType eConnType = eConnecterType_TCP;
		if( eType == eConnType_UDP_Raw || eType == eConnType_UDP_Prt )
			eConnType = eConnecterType_UDP;
		else if( eType == eConnType_TCP_Prts || eType == eConnType_TCP_Webs )
			eConnType = eConnecterType_TCPS;
		IListener* pListener = m_pNetWork->StartListener( 
			szAddres, nPort, eConnType, pCertificatePath, pPrivateKeyPath );
		CListenHandler* pHandler = new CListenHandler( this, pListener, nConnectClassID, eType );
		m_listListener.PushFront( *pHandler );
		GammaLog << "[Net] StartService," << szAddres << ":" << nPort << endl;
	}

	CConnection * CConnectionMgr::CreateConnect( IConnecter* pConnecter, 
		uint32_t nConnClassID, const char* szConnectAddress, EConnType eType )
	{
		CBaseConn* pHandler = static_cast<CBaseConn*>( CDynamicObject::CreateInstance( nConnClassID ) );
		if( eType == eConnType_UDP_Raw )
			return new CConnection( this, pConnecter, pHandler, szConnectAddress, false );
		if( eType == eConnType_UDP_Prt )
			return new CPrtConnection( this, pConnecter, pHandler, szConnectAddress, false );
		if( eType == eConnType_TCP_Raw )
			return new CConnection( this, pConnecter, pHandler, szConnectAddress, true );
		if( eType == eConnType_TCP_Prt || eType == eConnType_TCP_Prts )
			return new CPrtConnection( this, pConnecter, pHandler, szConnectAddress, true );
		if( eType == eConnType_TCP_Web || eType == eConnType_TCP_Webs )
			return new CWebConnection( this, pConnecter, pHandler, szConnectAddress );
		return NULL;
	}

	void CConnectionMgr::OnAccept( Gamma::IConnecter& Connect, 
		uint32_t nConnClassID, EConnType eType )
	{
		CConnection* pConnect = CreateConnect( &Connect, nConnClassID, NULL, eType );
		CConnList* pConnList = m_mapConnList[eCT_Server][nConnClassID];
		GammaAst( pConnList );
		pConnList->PushBack( *pConnect );
	}

	CBaseConn* CConnectionMgr::Connect( const char* szAddress, 
		uint16_t nPort, uint32_t nConnectClassID, EConnType eType )
	{
		IConnecter* pConn = NULL;
		EConnecterType eConnType = eConnecterType_TCP;
		if( eType == eConnType_UDP_Raw || eType == eConnType_UDP_Prt )
			eConnType = eConnecterType_UDP;
		else if( eType == eConnType_TCP_Prts || eType == eConnType_TCP_Webs )
			eConnType = eConnecterType_TCPS;
		if( szAddress && szAddress[0] && nPort )
			pConn = m_pNetWork->Connect( szAddress, nPort, eConnType );
		if( szAddress && szAddress[0] && nPort && !pConn )
			return NULL;
		if( m_mapConnList[eCT_Client].find( nConnectClassID ) == m_mapConnList[eCT_Client].end() )
			m_mapConnList[eCT_Client][nConnectClassID] = new CConnList;
		CConnection* pConnect = CreateConnect( pConn, nConnectClassID, szAddress, eType );
		m_mapConnList[eCT_Client][nConnectClassID]->PushBack( *pConnect );
		return pConnect->GetHandler();
	}

	uint32_t CConnectionMgr::GetAllConn( CConnList& listConn, CBaseConn** pConnArray, uint32_t nMaxCount )
	{
		uint32_t nCount = 0;
		CConnection* pConnecting = listConn.GetFirst();
		while( pConnecting && nCount < nMaxCount )
		{
			if( pConnArray )
				pConnArray[nCount] = pConnecting->GetHandler();
			nCount++;
			pConnecting = pConnecting->ConnNode::GetNext();
		}
		return nCount;
	}

	uint32_t CConnectionMgr::GetAllConn( uint32_t nConnectClassID, CBaseConn* aryConn[], uint32_t nCount )
	{
		CConnListMap::iterator it;
		it = m_mapConnList[eCT_Server].find( nConnectClassID );
		if( it != m_mapConnList[eCT_Server].end() )
			return GetAllConn( *it->second, aryConn, nCount );
		it = m_mapConnList[eCT_Client].find( nConnectClassID );
		if( it == m_mapConnList[eCT_Client].end() )
			return 0;
		return GetAllConn( *it->second, aryConn, nCount );
	}

	void CConnectionMgr::StopService( const char* szAddres, uint16_t nPort, uint32_t nConnectClassID )
	{
		CAddress Address = CAddress( szAddres, nPort );
		CListenHandler* pHandler = m_listListener.GetFirst();
		while( pHandler && !pHandler->Match( nConnectClassID, Address ) )
			pHandler = pHandler->GetNext();
		delete pHandler;
		CConnListMap::iterator it = m_mapConnList[eCT_Server].find( nConnectClassID );
		if( it == m_mapConnList[eCT_Server].end() )
			return;
		TryShutDownConn( *it->second );
		m_mapConnList[eCT_Server].erase( it );
	}

	void CConnectionMgr::StopAllService()
	{
		while( m_listListener.GetFirst() )
		{
			CListenHandler* pHandler = m_listListener.GetFirst();
			const CAddress& addrListen = pHandler->GetAddress();
			uint32_t nClassID = pHandler->GetConnClassID();
			StopService( addrListen.GetAddress(), addrListen.GetPort(), nClassID );
		}
	}

	bool CConnectionMgr::StopConnect( uint32_t nConnectClassID )
	{
		CConnListMap::iterator it = m_mapConnList[eCT_Client].find( nConnectClassID );
		if( it == m_mapConnList[eCT_Client].end() )
			return true;
		TryShutDownConn( *it->second );
		m_mapConnList[eCT_Client].erase( it );
		return false;
	}

	bool CConnectionMgr::StopAllConnect()
	{
		bool bFull = true;
		while( !m_mapConnList[eCT_Client].empty() )
		{
			CConnListMap::iterator it = m_mapConnList[eCT_Client].begin();
			if ( StopConnect( it->first ) )
				continue;
			bFull = false;
		}
		return bFull;
	}

}
