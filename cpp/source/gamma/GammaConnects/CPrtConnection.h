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

typedef struct IKCPCB ikcpcb;

namespace Gamma
{
	class CConnectionMgr;
	class CPrtConnection
		: public CConnection
		, public TDispatch<CPrtConnection, uint8_t>
	{
		ikcpcb*					m_pKCP;
		uint32_t					m_nNextKcpUpdateTime;
		string					m_strKCPRecvBuffer;
		uint32_t					m_nKCPBufferSize; 
		string					m_strKCPSendBuffer;
		uint32_t					m_nPreUpdateTime;

		uint32_t					m_nRecvCount;
		uint32_t					m_nSendCount;

		int64_t					m_nPreSendTime;
		uint32_t					m_nPingDelay;
		uint32_t					m_nHeartBeatInterval;

		void					SendHeartBeatMsg();

#if( defined _ANDROID || defined _IOS )
		string					m_szAligenBuf;
#endif
	protected:	
		virtual size_t			Dispatch( const char* pBuf, size_t nSize );

		bool					OnUpdate();

		void					OnCheckTimeOut();
		void					OnHeartBeatStop();

		const void*				AligenBuffer( const void* pBuffer, uint32_t nLen );
		void					SendBuffer( bool bReliable, const void* pCmd, size_t nSize );

		template<class T> void	OnNetMsg( const T* pCmd, size_t nSize );

	public:
		CPrtConnection( CConnectionMgr* pConnMgr, IConnecter* pConnect, 
			CBaseConn* pBase, const char* szConnectAddress, bool bTcp );
		virtual ~CPrtConnection(void);

		static void				RegisterMsgHandler();
		MsgCheckFun_t			GetCheckFun( size_t nIndex );
		uint32_t					GetPingDelay() const;
		void					SetHeartBeatInterval( uint32_t nSeconds );
		uint32_t					GetHeartBeatInterval() { return m_nHeartBeatInterval; }

		virtual void			SendShellMsg( bool bReliable, const SSendBuf aryBuffer[], uint32_t nBufferCount );
	};

	inline const void* CPrtConnection::AligenBuffer( const void* pBuffer, uint32_t nLen )
	{
#if( defined _ANDROID || defined _IOS )
		if( m_szAligenBuf.size() < nLen )
			m_szAligenBuf.resize( nLen );
		memcpy( &m_szAligenBuf[0], pBuffer, nLen );
		return &m_szAligenBuf[0];
#endif
		return pBuffer;
	}
	
	inline void CPrtConnection::SendBuffer( bool bReliable, const void* pCmd, size_t nSize )
	{
		if( m_pKCP && bReliable )
			m_strKCPSendBuffer.append( (const char*)pCmd, nSize );
		else
			CConnection::SendBuffer( pCmd, nSize );
	}
}


