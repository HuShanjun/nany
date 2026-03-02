/************************************************************************/
/* TConnectionMgr 连接管理                                              */
/* 功能:																*/
/* 1:调用网络底层方法,基类为check,close.实现类根据不同实现				*/
/* 2:管理和维护所有连接,以及连接断开的缓冲处理							*/
/* 3:实现CTick的时钟响应												*/
/* 4:包含本层提供给上层使用的接口,触发该接口							*/
/************************************************************************/
#pragma once

#include "GammaCommon/GammaTime.h"
#include "GammaCommon/TTinyList.h"
#include "GammaCommon/TGammaList.h"
#include "GammaNetwork/INetworkInterface.h"
#include "GammaConnects/IConnectionMgr.h"
#include <map>

namespace Gamma
{
	using namespace std;
	class CConnection;
	class CListenHandler; 
	typedef TGammaList<CConnection> CConnList;
	typedef TTinyList<CConnection> CConnUpdateList;
	typedef map<uint32_t, CConnList*> CConnListMap;
	typedef TTinyList<CListenHandler> CListenerList;
	typedef TGammaList<CConnection>::CGammaListNode ConnNode;

	class CConnectionMgr : public IConnectionMgr
	{
		DEFAULT_METHOD( CConnectionMgr );
		enum { eCT_Client, eCT_Server, eCT_Count };
	protected:
		INetwork*			m_pNetWork;
		bool				m_bStrictMode;
		uint32_t				m_nAutoDisconnectTime;
		int64_t				m_nCurCheckTime;
		CListenerList		m_listListener;
		CConnUpdateList		m_listUpdateConn;
		CConnListMap		m_mapConnList[eCT_Count];
		SKcpConfig			m_kcpConfig;

		void				OnCheckConnecting();
		void				TryShutDownConn( CConnList& listConn );
		uint32_t				GetAllConn( CConnList& listConn, CBaseConn** pConnArray, uint32_t nMaxCount );
		void				OnAccept( Gamma::IConnecter& Connect, uint32_t nConnClassID, EConnType eType );
		CConnection*		CreateConnect( IConnecter* pConnecter, uint32_t nConnClassID, 
								const char* szConnectAddress, EConnType eType );
	public:
		CConnectionMgr( uint32_t nAutoDisconnectTime, bool bStrictMode );
		virtual ~CConnectionMgr();
		friend class CListenHandler;

		/// 框架或shell循环调用
		/// 检查系统和网络事件,nWaitTimes为事件等待时间.若为网络事件,回由底层回调到响应的连接处理
		bool				Check( uint32_t nWaitTimes );
		INetwork*			GetNetwork() { return m_pNetWork; }
		void				PreResolveDomain( const char* szAddress );
		int64_t				GetCurCheckTime() const { return m_nCurCheckTime; }
		bool				IsStrictMode() const { return m_bStrictMode; }
		uint32_t				GetAutoDisconnectTime() const { return m_nAutoDisconnectTime; }
		void				AddUpdateConn( CConnection& Conn );
		void				StopService( const char* szAddres, uint16_t nPort, uint32_t nConnectClassID );
		void				StopAllService();
		bool				StopConnect( uint32_t nConnectClassID );
		bool				StopAllConnect();

		virtual void		StartService( const char* szAddres, uint16_t nPort, uint32_t nConnectClassID, 
								EConnType eType, const char* pCertificatePath, const char* pPrivateKeyPath );
		virtual CBaseConn*	Connect( const char* szAddress, uint16_t nPort, uint32_t nConnectClassID, EConnType eType );
		virtual uint32_t		GetAllConn( uint32_t nConnectClassID, CBaseConn* aryConn[], uint32_t nCount );

		const SKcpConfig&	KcpConfig() { return m_kcpConfig; }
		SKcpConfig			GetKcpConfig() { return m_kcpConfig; }
		void				SetKcpConfig( SKcpConfig config ) { m_kcpConfig = config; }
	};
}

