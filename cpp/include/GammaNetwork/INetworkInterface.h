//=========================================================================
// IGraphic.h 
// 定义基本图形接口
// 柯达昭
// 2007-09-29
//=========================================================================
#ifndef __INETWORK_INTERFACE_H__
#define __INETWORK_INTERFACE_H__

#include "GammaNetwork/GammaNetworkBase.h"
#include "GammaNetwork/INetHandler.h"
#include "GammaNetwork/CAddress.h"

namespace Gamma
{
	class ISocket;
	class IConnecter;
	class INetwork;

	enum EConnecterType
	{
		eConnecterType_UDP,
		eConnecterType_TCP,
		eConnecterType_TCPS,
	};

	class IListener
	{
	public:
		virtual void				SetHandler( IListenHandler* pHandler ) = 0;
		virtual IListenHandler*		GetHandler() = 0;
		virtual const CAddress&		GetLocalAddress() const = 0;
		virtual EConnecterType		GetConnectType() const = 0;
		virtual void				Release() = 0;
	};

	class IConnecter
	{
	public:
		virtual void				CmdClose( bool bGrace = true ) = 0;
		virtual void				Send( const void* pBuf, size_t nSize ) = 0;
		virtual void				CheckRecvBuff() = 0;

		virtual void				SetHandler( IConnectHandler* pHandler ) = 0;
		virtual IConnectHandler*	GetHandler() const = 0;

		virtual const CAddress&		GetLocalAddress() const = 0;
		virtual const CAddress&		GetRemoteAddress() const = 0;
		virtual EConnecterType		GetConnectType() const = 0;

		virtual bool				IsConnecting() const = 0;
		virtual bool				IsConnected() const = 0;
		virtual bool				IsDisconnected() const = 0;
		virtual bool				IsDisconnecting() const = 0;
		virtual bool				IsEverConnected() const = 0;

		virtual ECloseType			GetCloseType() const = 0;
		virtual size_t				GetTotalSendSize() const = 0;
		virtual size_t				GetTotalRecvSize() const = 0;
		virtual int64_t				GetCreateTime() const = 0;
		virtual int64_t				GetConnectTime() const = 0;
		virtual int64_t				GetConnectOKTime() const = 0;
		virtual int64_t				GetFirstEvenTime() const = 0;
	};

	class INetwork
	{
	public:
		virtual void				Release() = 0;
		virtual void				EnableLog( bool bLog ) = 0;
		virtual bool				PreResolveDomain( const char* szAddress, uint32_t nValidSeconds = INVALID_32BITID ) = 0;

		virtual IListener*			StartListener( const char* szAddres, uint16_t nPort, EConnecterType eType = eConnecterType_TCP,
										const char* pCertificatePath = NULL, const char* pPrivateKeyPath = NULL ) = 0;
		virtual IConnecter*			Connect( const char* szAddres, uint16_t nPort, EConnecterType eType = eConnecterType_TCP ) = 0;
		virtual bool				Check( uint32_t nTimeOut ) = 0;
	};

	GAMMA_NETWORK_API INetwork* CreateNetWork( uint32_t nMaxConnect = -1, uint32_t nNetworkThread = -1 );
}

#endif
