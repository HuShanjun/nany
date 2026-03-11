#pragma once
#include "GammaCommon/IGammaUnknow.h"
#include "GammaNetwork/CAddress.h"
#include "GammaConnects/ConnectDef.h"

#if ( defined( _WIN32 ) && defined( GAMMA_DLL ) )
	#if defined( GAMMA_CONNECTS_EXPORTS )
		#define GAMMA_CONNECTS_API __declspec(dllexport)
	#else
		#define GAMMA_CONNECTS_API __declspec(dllimport)
	#endif
#else
	#define GAMMA_CONNECTS_API 
#endif

namespace Gamma
{
	class INetwork;
	class CBaseConn;

	class IConnectionMgr : public IGammaUnknown
	{
	public:
		virtual INetwork*	GetNetwork() = 0;
		virtual void		PreResolveDomain( const char* szAddress ) = 0;
		virtual bool		Check( uint32_t nWaitTimes ) = 0;
		virtual void		StopService( const char* szAddres, uint16_t nPort, uint32_t nConnectClassID ) = 0;
		virtual void		StopAllService() = 0;
		virtual bool		StopConnect( uint32_t nConnectClassID ) = 0;
		virtual bool		StopAllConnect() = 0;

		virtual void		StartService( const char* szAddres, uint16_t nPort, 
								uint32_t nConnectClassID, EConnType eType = eConnType_TCP_Prt,
								const char* pCertificatePath = NULL, const char* pPrivateKeyPath = NULL ) = 0;
		virtual CBaseConn*	Connect( const char* szAddres, uint16_t nPort, 
								uint32_t nConnectClassID, EConnType eType = eConnType_TCP_Prt ) = 0;
		virtual uint32_t		GetAllConn( uint32_t nConnectClassID, CBaseConn* aryConn[], uint32_t nCount ) = 0;

		virtual SKcpConfig	GetKcpConfig() = 0;
		virtual void		SetKcpConfig( SKcpConfig config ) = 0;
	};

	// 严格模式下，连接没有回调OnConnected，就不会回调OnDisconnect
	GAMMA_CONNECTS_API IConnectionMgr* CreateConnMgr( uint32_t nAutoDisconnectTime, bool bStrictMode = false );
}
