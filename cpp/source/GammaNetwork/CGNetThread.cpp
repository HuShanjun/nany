
#include "GammaCommon/CThread.h"
#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/TConstString.h"
#include "GammaCommon/GammaCodeCvs.h"
#include "GammaNetworkHelp.h"
#include "CGNetwork.h"
#include "CGListener.h"
#include "CGConnecter.h"
#include "CGNetThread.h"
#include <sstream>
#include <errno.h>
#include <string>

using namespace std;

namespace Gamma
{
	#define MAX_CONNECT_COUNT	(64*1024)

	static void setThreadName(const std::string& thread_name){
		#ifndef _WIN32
		pthread_setname_np(pthread_self(), thread_name.c_str());
		#endif
	}

	CGNetThread::CGNetThread( CGNetwork* pNetwork, uint32 nMaxConnect )
		: m_pNetwork(pNetwork)
		, m_nSocketCount(0)
		, m_hThread(nullptr)
		, m_bQuit(false)
		, m_strThreadName("NetThread")
	{
		if( nMaxConnect > MAX_CONNECT_COUNT )
			nMaxConnect = MAX_CONNECT_COUNT;

#ifndef FORCE_SELECT_MODE
#ifndef _WIN32
		m_nEpoll = epoll_create( nMaxConnect + 1 );
		GammaAst( -1 != m_nEpoll );
#endif
#endif

		auto funThread = (THREADPROC)&CGNetThread::NT_RunThread;
		GammaCreateThread( &m_hThread, 0, funThread, this );
	}

	CGNetThread::~CGNetThread()
	{
		m_bQuit = true;
		GammaJoinThread( m_hThread );
		NT_ThreadCheck();
		MainThreadCheck();

#ifndef FORCE_SELECT_MODE
#ifndef _WIN32
		close( m_nEpoll );
#endif
#endif
	}

	void CGNetThread::AddSocket( CGSocket* pSocket )
	{
		SNetCmd Cmd( eNC_AddSocket, pSocket );
		m_SendBuffer.PushBuffer( Cmd, nullptr, 0, false );
		m_nSocketCount++;
	}

	void CGNetThread::CloseSocket( CGSocket* pSocket )
	{
		SNetCmd Cmd( eNC_RemoveSocket, pSocket );
		m_SendBuffer.PushBuffer( Cmd, nullptr, 0, false );
		m_nSocketCount--;
	}

	void CGNetThread::StartSocket( CGSocket* pSocket )
	{
		SNetCmd Cmd( eNC_Start, pSocket );
		m_SendBuffer.PushBuffer( Cmd, nullptr, 0, false );
	}

 	void CGNetThread::OnAccept( CGSocket* pSocket, CGListener* pListener )
	{
		GammaAst( !pSocket->IsListener() );
		GammaAst( !pSocket->GetContext() );
		if( !pListener->IsValid() )
		{
			delete pSocket;
			return;
		}

		auto pConnect = m_pNetwork->CreateConnecter( pSocket );
		pListener->GetHandler()->OnAccept( *pConnect );
		pConnect->OnEvent( false, nullptr, 0 );
	}

	void CGNetThread::OnRemoved( CGSocket* pSocket )
	{
		delete pSocket;
	}

	void CGNetThread::OnConnected( CGSocket* pSocket )
	{
		GammaAst( !pSocket->IsListener() );
		auto pConnect = static_cast<CGConnecter*>( pSocket->GetContext() );
		pConnect->OnEvent( false, nullptr, 0 );
	}

	void CGNetThread::OnDataRecieve( CGSocket* pSocket, const void* pData, size_t nSize )
	{
		GammaAst( !pSocket->IsListener() );
		auto pConnect = static_cast<CGConnecter*>(pSocket->GetContext());
		pConnect->OnEvent( false, (const tbyte*)pData, (uint32)nSize );
	}

	bool CGNetThread::MainThreadCheck()
	{
		SNetCmd Cmd;
		SSendBuf SendBuf;
		uint32 nBufferCount = 1;
		while( m_RecvBuffer.PopBuffer( Cmd, &SendBuf, nBufferCount ) )
		{
			if( Cmd.first == eNC_Accept )
				OnAccept( Cmd.second, *(CGListener**)SendBuf.first );
			else if( Cmd.first == eNC_RemoveSocket )
				OnRemoved( Cmd.second );
			else if( Cmd.first == eNC_Connected && Cmd.second->GetContext() )
				OnConnected( Cmd.second );
			else if( Cmd.first == eNC_DataArrived && Cmd.second->GetContext() )
				OnDataRecieve( Cmd.second, SendBuf.first, SendBuf.second );
			nBufferCount = 1;
		}
		return true;
	}


	//===============================================================
	// 网络线程函数
	//===============================================================
	void CGNetThread::NT_OnAccept( CGSocket* pSocket, void* pContext )
	{
		SNetCmd Cmd( eNC_Accept, pSocket );
		SSendBuf SendBuf( &pContext, sizeof(pContext) );
		m_RecvBuffer.PushBuffer( Cmd, &SendBuf, 1, false );

		// tcp协议下，accept得到的socke是独立的，可以分配到不同的网络线程，
		// 因此需要在主线程建立Connect包装后，再分配工作线程来绑定socket
		if( pSocket->GetConnectType() != eConnecterType_UDP )
			return;
		// 而udp协议下，对于服务端的UDP的连接而言，其socket是和listen共用的，
		// 没有办法将不同的连接数据收发分配到不同网络线程，
		// 因此绑定在listener绑定的网络线程中直接完成，这里配合将soket加入列表
		// 特别说明，因为socket共用的关系，事件通知也是共用的，也不需要注册
		m_listSockets.PushBack( *pSocket );
	}

	void CGNetThread::NT_OnConnected( CGSocket* pSocket )
	{
		SNetCmd Cmd( eNC_Connected, pSocket );
		m_RecvBuffer.PushBuffer( Cmd, nullptr, 0, false );
		NT_SetEvent( pSocket, eNE_ConnectedRead | eNE_ConnectedWrite | eNE_Error );
	}

	void CGNetThread::NT_RecieveData( CGSocket* pSocket, const void* pData, size_t nSize )
	{
		SNetCmd Cmd( eNC_DataArrived, pSocket );
		SSendBuf SendBuf( pData, nSize );
		m_RecvBuffer.PushBuffer( Cmd, &SendBuf, 1, false );
	}

	void CGNetThread::NT_OnAdd( CGSocket* pSocket )
	{
		m_listSockets.PushBack( *pSocket );
		if( pSocket->NT_GetEventID() == 0 &&
			pSocket->GetLocalAddress().size() &&
			pSocket->GetRemoteAddress().size() )
		{
			GammaAst( pSocket->GetConnectType() != eConnecterType_UDP );
			// 运行到这里，一定是tcp协议accept得到的socket
			// 因为local地址connect时是没有赋值的
			// 而udp协议下，对于服务端的UDP的连接而言，其socket是和listen共用的，
			// 没有办法将不同的连接数据收发分配到不同网络线程，
			// 因此绑定在listener绑定的网络线程中直接完成，
			// 不需要主线程发送eNC_AddSocket命令到网络线程进行绑定
			// tcp协议下，accept得到的socke是独立的，可以分配到不同的网络线程，
			// 因此事件在建立Connect包装，分配好工作线程后再注册事件
			NT_SetEvent( pSocket, eNE_ConnectedRead | eNE_ConnectedWrite | eNE_Error );
		}
	}

	void CGNetThread::NT_OnRemove( CGSocket* pSocket )
	{
		
		if( pSocket->NT_GetEventID() )
			NT_DelEvent( pSocket );
		pSocket->NT_Close();
		SNetCmd Cmd( eNC_RemoveSocket, pSocket );
		m_RecvBuffer.PushBuffer( Cmd, nullptr, 0, false );
	}

	void CGNetThread::NT_OnStart( CGSocket* pSocket )
	{
		GammaAst( pSocket->IsInList() );
		if( !pSocket->IsListener() )
			NT_SetEvent( pSocket, eNE_Connecting | eNE_Error ); // Tcp or Udp client
		else if( pSocket->GetConnectType() != eConnecterType_UDP )
			NT_SetEvent( pSocket, eNE_Listen );
		else
			NT_SetEvent( pSocket, eNE_ConnectedRead | eNE_ConnectedWrite );
	}

	uint32 CGNetThread::NT_RunThread( void* pParam )
	{
		auto pThread = reinterpret_cast<CGNetThread*>(pParam);
		GammaSetThreadName(pThread->m_strThreadName.c_str());
		while( !pThread->m_bQuit )
			pThread->NT_ThreadCheck();
		return 0;
	}

	void CGNetThread::NT_ThreadCheck()
	{
		SNetCmd Cmd;
		SSendBuf SendBuf;
		uint32 nBufferCount = 1;
		uint32 nCount = 0;

		// 退出时要处理完所有命令
		while( nCount++ < 1000 || m_bQuit )
		{
			if (!m_SendBuffer.PopBuffer(Cmd, &SendBuf, nBufferCount))
			{
				GammaSleep(10);
				break;
			}
			if( Cmd.first == eNC_AddSocket )
				NT_OnAdd( Cmd.second );
			else if( Cmd.first == eNC_RemoveSocket )
				NT_OnRemove( Cmd.second );
			else if( Cmd.first == eNC_Start )
				NT_OnStart( Cmd.second );
			nBufferCount = 1;
		}
		NT_Loop();
	}

	bool CGNetThread::NT_Loop()
	{
		if( m_listSockets.IsEmpty() )
			return true;

		/* 检查发送缓冲区的数据 */
		uint32 nSocketCount = 0;
		for( auto pSocket = m_listSockets.GetFirst();
			pSocket; pSocket = pSocket->GetNext() )
		{
			nSocketCount++;
			if( !pSocket->IsSendAllow() )
				continue;
			pSocket->NT_ProcessEvent( eNE_ConnectedWrite, false );
		}

#ifdef FORCE_SELECT_MODE
		auto pSocket = m_listSockets.GetFirst();
		while( pSocket )
		{
			fd_set fdRecv, fdSend, fdError;
			FD_ZERO( &fdRecv );
			FD_ZERO( &fdSend );
			FD_ZERO( &fdError );

			uint32 nCount = 0;
			SOCKET hMaxSocket = 0;
			CGSocket* arySocket[FD_SETSIZE];
			while( nCount < FD_SETSIZE && pSocket )
			{
				SOCKET hSocket = pSocket->GetSocket();
				uint32 nEvent = (uint32)pSocket->NT_GetEventID();
				if( nEvent )
				{
					if( nEvent & (eNE_ConnectedRead) )
						FD_SET( hSocket, &fdRecv );
					if( nEvent & (eNE_ConnectedWrite) )
						FD_SET( hSocket, &fdSend );
					if( nEvent & eNE_Error )
						FD_SET( hSocket, &fdError );
					if( hSocket > hMaxSocket )
						hMaxSocket = hSocket;
					arySocket[nCount++] = pSocket;
				}
				pSocket = pSocket->CSocketList::CGammaListNode::GetNext();
			}

			if( nCount == 0 )
				break;

			struct timeval tv = { 0, 0 };
			if( select( hMaxSocket + 1, &fdRecv, &fdSend, &fdError, &tv ) == 0 )
				continue;

			for( uint32 i = 0; i < nCount; i++ )
			{
				uint32 nEvent = 0;
				SOCKET hSocket = arySocket[i]->GetSocket();
				if( FD_ISSET( hSocket, &fdRecv ) )
					nEvent |= eNE_ConnectedRead;
				if( FD_ISSET( hSocket, &fdSend ) )
					nEvent |= eNE_ConnectedWrite;
				if( FD_ISSET( hSocket, &fdError ) )
					nEvent |= eNE_Error;
				if( nEvent == 0 )
					continue;
				arySocket[i]->NT_ProcessEvent( nEvent, (nEvent & eNE_Error) != 0 );
			}
		}

#elif defined _WIN32
		CGSocket* arySocket[MAXIMUM_WAIT_OBJECTS];
		WSAEVENT aryEvent[MAXIMUM_WAIT_OBJECTS];
		auto pSocket = m_listSockets.GetFirst();
		while( pSocket )
		{
			uint32 nWaitCount = 0;
			while( pSocket && nWaitCount < MAXIMUM_WAIT_OBJECTS )
			{
				WSAEVENT hEvent = (WSAEVENT)pSocket->NT_GetEventID();
				if( hEvent )
				{
					arySocket[nWaitCount] = pSocket;
					aryEvent[nWaitCount++] = hEvent;
				}
				pSocket = pSocket->CSocketList::CGammaListNode::GetNext();
			}

			if( nWaitCount == 0 )
				break;

			uint32 nResult = WaitForMultipleObjects( 
				nWaitCount, aryEvent, FALSE, 0 );
			uint32 nIndex = nResult - WAIT_OBJECT_0;
			if( nIndex < nWaitCount )
			{
				WSANETWORKEVENTS NetworkEvents;
				memset( &NetworkEvents, 0, sizeof( WSANETWORKEVENTS ) );
				CGSocket* pSocket = arySocket[nIndex];
				WSAResetEvent( aryEvent[nIndex] );
				if( WSAEnumNetworkEvents( pSocket->GetSocket(),
					aryEvent[nIndex], &NetworkEvents ) )
				{
					ostringstream strm;
					strm << "WSAEnumNetworkEvents failed with error code:"
						<< GNWGetLastError();
					GammaThrow( strm.str() );
				}

				uint32 nEvent = NetworkEvents.lNetworkEvents;
				if( NetworkEvents.iErrorCode[FD_CONNECT_BIT] )
					nEvent |= eNE_Error;
				pSocket->NT_ProcessEvent( nEvent, (nEvent & eNE_Error) != 0 );
			}
			else if( WAIT_FAILED == nResult )
			{
				ostringstream strm;
				strm << "WaitForMultipleObjects failed with error code:"
					<< GNWGetLastError();
				GammaThrow( strm.str() );
			}
		}
#else
		if( m_vecEvent.size() < nSocketCount )
			m_vecEvent.resize( nSocketCount );

		int32 nEventCount = epoll_wait( m_nEpoll,
			&m_vecEvent[0], m_vecEvent.size(), 0 );
		if( -1 == nEventCount )
		{
			int nErrorCode = errno;
			if( nErrorCode == EINTR )//received a signal
				return false;
			ostringstream strm;
			strm << "epoll_wait failed with error:" 
				<< strerror( nErrorCode ) << "(" << nErrorCode << ")";
			GammaThrow( strm.str() );
		}

		for( int32 i = 0; i < nEventCount; ++i )
		{
			epoll_event& e = m_vecEvent[i];
			GammaAst( e.data.ptr );
			CGSocket* pSocket = (CGSocket*)e.data.ptr;
			uint32 nEvent = e.events;
			pSocket->NT_ProcessEvent( nEvent, (nEvent& eNE_Error) != 0 );
		}
#endif
		return true;
	}

	void CGNetThread::NT_SetEvent( CGSocket* pSocket, uint32 nEvent )
	{
		GammaAst( pSocket->IsInList() && nEvent );
#ifdef FORCE_SELECT_MODE
		pSocket->NT_SetEventID( nEvent );
#elif defined _WIN32
		if( !pSocket->NT_GetEventID() )
		{
			WSAEVENT hEvent = WSACreateEvent();
			if( WSA_INVALID_EVENT == hEvent )
			{
				ostringstream strm;
				strm << "WSACreateEvent failed with error code "
					<< GNWGetLastError() << "." << ends;
				GammaThrow( strm.str() );
			}

			pSocket->NT_SetEventID((ptrdiff_t)hEvent);
		}

		SOCKET hSocket = pSocket->GetSocket();
		WSAEVENT hEvent = (WSAEVENT)pSocket->NT_GetEventID();
		if( SOCKET_ERROR == WSAEventSelect( hSocket, hEvent, nEvent ) )
		{
			ostringstream strm;
			strm << "WSAEventSelect failed with error code:"
				<< GNWGetLastError();
			GammaThrow( strm.str() );
		}
#else
		epoll_event ee;
		ee.data.ptr = pSocket;
		ee.events = nEvent;

		int32 nOperator = pSocket->NT_GetEventID() ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
		if( -1 == epoll_ctl( m_nEpoll, nOperator, pSocket->GetSocket(), &ee ) )
		{
			ostringstream strm;
			strm << "epoll_ctl failed with error:" << strerror( errno ) << ends;
			GammaThrow( strm.str() );
		}
		pSocket->NT_SetEventID( nEvent );
#endif
	}

	void CGNetThread::NT_DelEvent( CGSocket* pSocket )
	{
		GammaAst( pSocket->NT_GetEventID() );
#ifdef FORCE_SELECT_MODE
#elif defined _WIN32
		WSACloseEvent( (WSAEVENT)pSocket->NT_GetEventID() );
#else
		epoll_event ee;
		ee.data.ptr = pSocket;
		ee.events = 0;
		int32 nOperator = EPOLL_CTL_DEL;
		if( -1 == epoll_ctl( m_nEpoll, nOperator, pSocket->GetSocket(), &ee ) )
		{
			ostringstream strm;
			strm << "epoll_ctl failed with error:" << strerror( errno ) << ends;
			GammaThrow( strm.str() );
		}
#endif
		pSocket->NT_SetEventID( 0 );
	}
}
