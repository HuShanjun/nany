#pragma once
#include "GammaCommon/GammaTime.h"
#include "GammaCommon/TGammaList.h"
#include "GammaCommon/TTinyList.h"
#include "GammaCommon/CGammaRand.h"
#include "GammaNetwork/INetworkInterface.h"
#include "GammaConnects/CBaseConn.h"
#include "GammaConnects/TDispatch.h"
#include "GammaConnects/IConnectionMgr.h"
#include "CConnection.h"

#ifndef _WIN32
	#include <alloca.h>
#else
	#include <malloc.h>
#endif

namespace Gamma
{
	class CConnectionMgr;
	class CWebConnection;
	struct SWebSocketMsg;
	typedef TDispatch<CWebConnection, uint16, void, SWebSocketMsg> CWebConnBase;

	class CWebConnection
		: public CConnection
		, public CWebConnBase
	{
		IConnecter*				m_pConnectPending;
		string					m_strHostName;
		string					m_szAligenBuf;
		string					m_szSendBuff;
		uint32					m_nBufferID;

		uint32					m_nPreUpdateTime;

		uint32					m_nRecvCount;
		uint32					m_nSendCount;

		int64					m_nPreSendTime;
		uint32					m_nPingDelay;
		uint32					m_nHeartBeatInterval;

		void					SendHeartBeatMsg();

	protected:	
		virtual size_t			Dispatch( const char* pBuf, size_t nSize );
		size_t					CheckShakeHand( const size_t nSize, const char* pBuf );
		void					SendFrameData( SWebSocketMsg MsgHead, bool bShell,
									const SSendBuf* aryBuffer, uint32 nBufferCount );

		void					OnConnected();
		void					OnCheckTimeOut();
		void					OnHeartBeatStop();

		template<class T> void	OnProcessMsg( const T* pCmd, size_t nSize );
		template<class T> void	OnNetMsg( const T* pCmd );
	public:
		CWebConnection( CConnectionMgr* pConnMgr, 
			IConnecter* pConnect, CBaseConn* pBase, const char* szConnectAddress );
		virtual ~CWebConnection(void);

		MsgCheckFun_t			GetCheckFun( size_t nIndex );
		static void				RegisterMsgHandler();

		uint32					GetPingDelay() const;
		void					SetHeartBeatInterval( uint32 nSeconds );
		uint32					GetHeartBeatInterval() { return m_nHeartBeatInterval; }

		virtual void			SendShellMsg( bool bReliable, 
									const SSendBuf aryBuffer[], uint32 nBufferCount );

	};
}


