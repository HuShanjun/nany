//=========================================================================
// CGListener.h 
// 定义SOCKET监听类
// 柯达昭
// 2007-12-02
//=========================================================================
#ifndef __GAMMA_NETTHREAD_H__
#define __GAMMA_NETTHREAD_H__

#include "GammaCommon/TGammaRBTree.h"
#include "GammaCommon/TGammaList.h"
#include "GammaCommon/TTinyList.h"
#include "GammaCommon/TCircleBuffer.h"
#include "GammaNetworkHelp.h"
#include "CGSocket.h"
#include <map>
#include <string>

namespace Gamma
{
	class CGNetwork;

	enum ENetCmd
	{
		eNC_AddSocket,
		eNC_RemoveSocket,
		eNC_Start,
		eNC_DataArrived,
		eNC_Accept,
		eNC_Connected,
	};

	typedef std::pair<ENetCmd, CGSocket*> SNetCmd;

	//========================================================
	// 网络线程类
	//========================================================
	class CGNetThread
	{
		CGNetwork*						m_pNetwork;
		HTHREAD							m_hThread;
		bool							m_bQuit;

		std::string						m_strThreadName;

#ifndef FORCE_SELECT_MODE
#ifndef _WIN32
		int                             m_nEpoll;
		std::vector<epoll_event>		m_vecEvent;
#endif
#endif

		CSocketList						m_listSockets;
		uint32							m_nSocketCount;

		CCircleBuffer					m_SendBuffer;
		CCircleBuffer					m_RecvBuffer;

		static uint32					NT_RunThread( void* pParam );

		void							NT_ThreadCheck();
		bool							NT_Loop();

		// 网络线程回调
		void							NT_OnAdd( CGSocket* pSocket );
		void							NT_OnRemove( CGSocket* pSocket );
		void							NT_OnStart( CGSocket* pSocket );

		// 主线程回调
		void							OnRemoved( CGSocket* pSocket );
		void							OnAccept( CGSocket* pSocket, CGListener* pListener );
		void							OnConnected( CGSocket* pSocket );
		void							OnDataRecieve( CGSocket* pSocket, const void* pData, size_t nSize );
	public:
		CGNetThread( CGNetwork* pNetwork, uint32 nMaxConnect );
		virtual ~CGNetThread();

		// 主线程回调
		uint32							GetSocketCount() { return m_nSocketCount; }
		void							AddSocket( CGSocket* pSocket );
		void							CloseSocket( CGSocket* pSocket );
		void							StartSocket( CGSocket* pSocket );

		//flush 缓冲  监听事件  并派发到相应的事件处理函数中去
		bool		                    MainThreadCheck();

		// 网络线程回调
		void							NT_OnAccept( CGSocket* pSocket, void* pContext );
		void							NT_OnConnected( CGSocket* pSocket );
		void							NT_RecieveData( CGSocket* pSocket, const void* pData, size_t nSize );

		void                            NT_SetEvent( CGSocket* pSocket, uint32 nEvent );//添加socket以及设置相应的nEvent绑定
		void                            NT_DelEvent( CGSocket* pSocket );
	};
}

#endif
