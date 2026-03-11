//=========================================================================
// CGListener.h 
// 定义SOCKET监听类
// 柯达昭
// 2007-12-02
//=========================================================================
#ifndef __GAMMA_LISTENER_H__
#define __GAMMA_LISTENER_H__

#include "GammaCommon/TGammaRBTree.h"
#include "GammaNetworkHelp.h"
#include "CGSocket.h"
#include <map>

namespace Gamma
{
	//========================================================
	// TCP SOCKET监听类
	//========================================================
	class CGListener : public IListener
	{
	protected:
        CGNetwork*				m_pNetwork;
		CGSocket*				m_pSocket;
		IListenHandler*			m_pHandler;
		CAddress				m_LocalAddress;

	public:
		CGListener( CGNetwork* pNetwork, CGSocket* pSocket );
		virtual ~CGListener();

		bool					IsValid();
		virtual void			SetHandler( IListenHandler* pHandler ) { m_pHandler = pHandler; }
		virtual IListenHandler*	GetHandler() { return m_pHandler; }
		virtual const CAddress& GetLocalAddress() const;
		virtual EConnecterType	GetConnectType() const;

		virtual void			Start( const char* szAddres, uint16_t nPort );
		virtual void			Release();
	};
}

#endif
