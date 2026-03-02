//=========================================================================
// CGNetwork.h 
// 定义NetWork系统的主框架，Reactor模式
// 柯达昭
// 2007-12-02
//=========================================================================
#ifndef __GAMMA_NETWORK_H__
#define __GAMMA_NETWORK_H__

#include "GammaCommon/CThread.h"
#include "GammaCommon/TGammaList.h"
#include "GammaCommon/TGammaRBTree.h"
#include "GammaNetwork/INetworkInterface.h"
#include "GammaNetworkHelp.h"
#include <vector>

namespace Gamma
{
	class CGSocket;
	class CGNetThread;
	class CGConnecter;
	class CGListener;
	class CGListenerTCP;
	class CGConnecterTCP;
	class CGNetSendBuffer;
	class CConnectingNode;
	class CDisconnectNode;
	class CTempNode;
	class CAddrResolutionDelegate;
	struct SSendBuffer;
	typedef std::map<std::string, SSL_CTX*> CServerSSLContextMap;
	typedef TGammaList<CDisconnectNode> CDisconnectList;
	typedef TGammaList<CAddrResolutionDelegate> CResolutionDelegateList;
	typedef TGammaRBTree<CAddrResolutionDelegate> CResolutionDelegateTree;

	class CGNetwork : public INetwork
    {
		bool						m_bLog;

		const SSL_METHOD*			m_sslServerMethod;
		const SSL_METHOD*			m_sslClientMethod;
		SSL_CTX*					m_sslClientContext;
		CServerSSLContextMap		m_mapServerContexts;

		std::vector<CGNetThread*>	m_vecNetThread;

		HLOCK						m_hLockResolution;
		CResolutionDelegateTree		m_mapAddressReslv;
		CResolutionDelegateList		m_listFinished;
		CDisconnectList				m_listDisConnSocket;

		SSendBuffer*				m_pFreeBuffer;

    public:
        CGNetwork( uint32 nMaxConnect = -1, uint32 nNetworkThread = -1 );
		~CGNetwork(void);

		const SSL_METHOD*			GetSSLServerMethod() const { return m_sslServerMethod; }
		const SSL_METHOD*			GetSSLClientMethod() const { return m_sslClientMethod; }

		void						Release(){ delete this; }
		bool						PreResolveDomain( const char* szAddress, uint32 nValidSeconds );
		void						EnableLog( bool bLog ) { m_bLog = bLog; }
		bool						IsLogEnable() { return m_bLog; }

		/// 上层调用
		IListener*					StartListener( const char* szAddres, uint16 nPort, EConnecterType eType,
											const char* pCertificatePath,	const char* pPrivateKeyPath );
		IConnecter*					Connect( const char* szAddres, uint16 nPort, EConnecterType eType );
		CAddrResolutionDelegate*	GetAddressReslv( const char* szAddres );
		CGNetThread*				GetMinSocketThread();

		CGConnecter*				CreateConnecter( CGSocket* pSocket );
		void						ReleaseConnect( CGConnecter* pConnect );
		void						ReleaseListener( CGListener* pListener );
		void						AddDisConnSocket( CDisconnectNode* pDisConnSocket );

		bool		                Check( uint32 nTimeOut );
		SSendBuffer*				Alloc();
		void						Free( SSendBuffer* pData );
    };
}

#endif
