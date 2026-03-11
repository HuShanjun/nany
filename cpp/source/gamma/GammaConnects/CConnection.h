#pragma once
#include "GammaCommon/GammaTime.h"
#include "GammaCommon/TGammaList.h"
#include "GammaCommon/TTinyList.h"
#include "GammaCommon/CGammaRand.h"
#include "GammaNetwork/INetworkInterface.h"
#include "GammaConnects/CBaseConn.h"
#include "GammaConnects/TDispatch.h"
#include "GammaConnects/IConnectionMgr.h"
#include <string>
using namespace std;

#ifndef _WIN32
#include <alloca.h>
#else
#include <malloc.h>
#endif

namespace Gamma
{
	class CConnectionMgr;
	class CConnection
		: public IConnectHandler
		, public TGammaList<CConnection>::CGammaListNode
		, public TTinyList<CConnection>::CTinyListNode
	{
	protected:
		friend class CBaseConn;
		friend class CConnectionMgr;
		typedef std::list<CConnection*>::iterator	ConnIter_t;
		enum EEnableState { eES_Disable, eES_Pending, eES_Enable };
		enum ECmdCloseType { eCT_None, eCT_Force, eCT_Grace };
		enum EConnectionType { eTcp = 1, eServer = 2 };

		int64_t					m_nCreateTime;
		uint8_t					m_eConnectionType;
		bool					m_bEnableSendShellMsg;

		uint8_t					m_nEnableDispatchState;
		uint8_t					m_eCloseType;
		string					m_strCloseLog;

		uint32_t					m_nMinDelay;
		uint32_t					m_nMaxDelay;
		string					m_szSendBuf;
		string					m_szRecvBuf;

		CBaseConn*				m_pConnHandler;
		IConnecter*				m_pConnecter;				/// 底层关联对象  
		CConnectionMgr*			m_pConnMgr;					/// 连接管理 

		vector<size_t>			m_vecTotalSendSize;
		vector<size_t>			m_vecTotalRecvSize;

	protected:
		virtual void			OnConnected();
		virtual void			OnDisConnect( ECloseType eTypeClose );
		virtual size_t			OnRecv( const char* pBuf, size_t nSize );
		virtual size_t			Dispatch( const char* pBuf, size_t nSize );
		virtual bool			OnUpdate();

		virtual void			OnCheckTimeOut(){};
		size_t					Process( const char* pBuf, size_t nSize );
		void					SendBuffer( const void* pCmd, size_t nSize );

	public:
		CConnection( CConnectionMgr* pConnMgr, IConnecter* pConnect, 
			CBaseConn* pBase, const char* szConnectAddress, bool bTcp );
		virtual ~CConnection(void);

		CConnectionMgr*			GetConnMgr() const;
		bool					IsServer() const;
		bool					IsConnecting() const;
		bool					IsConnected() const;
		bool					IsDisconnected() const;
		bool					IsDisconnecting() const;
		CBaseConn*				GetHandler();
		IConnecter*				GetConn();
		int64_t					GetCreateTime() const;
		bool					IsMsgDispatchEnable() const;
		void					EnableMsgDispatch( bool bEnable );
		void					SetNetDelay( uint32_t nMinDelay, uint32_t nMaxDelay );

		virtual void			ShutDown( bool bGrace, const char* szLogContext );
		const CAddress&			GetLocalAddress() const;
		const CAddress&			GetRemoteAddress() const;

		virtual uint32_t			GetPingDelay() const { return 0; }
		virtual void			SendShellMsg( bool bReliable, const SSendBuf aryBuffer[], uint32_t nBufferCount );

		virtual void			SetHeartBeatInterval( uint32_t nSeconds ){};
		virtual uint32_t			GetHeartBeatInterval() { return 0; }
		void					EnableSendShellMsg( bool bValue );
		bool					IsEnableSendShellMsg() { return m_bEnableSendShellMsg; }

		void					EnableProfile( uint8_t nSendIDBits, uint8_t nRecvIDBits );
		size_t					GetSendBufferSize( uint16_t nShellID );
		size_t					GetRecvBufferSize( uint16_t nShellID );
		size_t					GetTotalSendSize();
		size_t					GetTotalRecvSize();
		void					PrintMsgSize();
		void					AddSendSize( uint16_t nShellID, size_t nSize );
		void					AddRecvSize( uint16_t nShellID, size_t nSize );
		bool					IsGraceClose() const;
		bool					IsForceClose() const;
		const char*				GetCloseLog() const;
	};

	inline void CConnection::SendBuffer( const void* pCmd, size_t nSize )
	{
		if( m_nMaxDelay == 0 && m_szSendBuf.empty() )
			return m_pConnecter->Send( pCmd, nSize );

		uint32_t nDelayTime = CGammaRand::Rand( m_nMinDelay, m_nMaxDelay );
		int64_t nSendTime = GetGammaTime() + nDelayTime;
		m_szSendBuf.append( (const char*)&nSendTime, sizeof(int64_t) );
		m_szSendBuf.append( (const char*)&nSize, sizeof(size_t) );
		m_szSendBuf.append( (const char*)pCmd, nSize );
	}

	inline bool CConnection::IsMsgDispatchEnable() const
	{
		return m_nEnableDispatchState != eES_Disable;
	}

	inline void CConnection::AddSendSize( uint16_t nShellID, size_t nSize )
	{
		if( m_vecTotalSendSize.empty() || !nSize )
			return;
		if( m_vecTotalSendSize.size() > 256 )
			m_vecTotalSendSize[nShellID] += nSize;
		else
			m_vecTotalSendSize[(uint8_t)nShellID] += nSize;
	}

	inline void CConnection::PrintMsgSize()
	{
		if (m_vecTotalSendSize.empty())
			return;
		printf("\n=======================Send Msg==========================\n");
		size_t total = 0;
		for (uint32_t i = 0; i < m_vecTotalSendSize.size(); ++i)
		{
			total += m_vecTotalSendSize[i];
		}
		for (uint32_t i=0; i< m_vecTotalSendSize.size(); ++i)
		{
			if (m_vecTotalSendSize[i] != 0)
			{
				printf("Msg(%d) Send Size(%ld, %ld%%)\n", i, 
					(long)m_vecTotalSendSize[i], (long)(m_vecTotalSendSize[i] * 100 /total));
			}
		}
		printf("\n=======================Recv Msg==========================\n");
		total = 0;
		for (uint32_t i = 0; i < m_vecTotalRecvSize.size(); ++i)
		{
			total += m_vecTotalRecvSize[i];
		}
		for (uint32_t i = 0; i < m_vecTotalRecvSize.size(); ++i)
		{
			if (m_vecTotalRecvSize[i] != 0)
			{
				printf("Msg(%d) Recv Size(%ld, %ld%%)\n", i,
					(long)m_vecTotalRecvSize[i], (long)(m_vecTotalRecvSize[i] * 100 /total));
			}
		}
	}

	inline void CConnection::AddRecvSize( uint16_t nShellID, size_t nSize )
	{
		if( m_vecTotalRecvSize.empty() || !nSize )
			return;
		if( m_vecTotalRecvSize.size() > 256 )
			m_vecTotalRecvSize[nShellID] += nSize;
		else
			m_vecTotalRecvSize[(uint8_t)nShellID] += nSize;
	}
}


