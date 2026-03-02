#pragma once
#include "GammaCommon/CDynamicObject.h"
#include "GammaNetwork/CAddress.h"
#include "GammaConnects/IConnectionMgr.h"
#include "GammaNetwork/INetHandler.h"

namespace Gamma
{
	class CConnection;

	class GAMMA_CONNECTS_API CBaseConn : public CDynamicObject
	{
		CConnection*	m_pConnection;
		friend class CConnection;

	public:
		CBaseConn(void);
		virtual ~CBaseConn(void);;

		virtual void	OnConnected() = 0;									/// 连接成功
		virtual void	OnDisConnect() = 0;									/// 开始断开

		CConnection*	GetConnection() const { return m_pConnection; }
		void			BindConnection( CConnection* pConnection );
		void			ShellCmdClose( const char* szLogContext = NULL );
		void			ForceClose( const char* szLogContext = NULL );

		bool			IsGraceClose() const;
		bool			IsForceClose() const;
		const char*		GetCloseLog() const;
		ECloseType		GetCloseType() const;
	
		bool			IsConnected() const;
		uint32_t			GetPingDelay() const;

		const CAddress&	GetRemoteAddress() const;
		const CAddress& GetLocalAddress() const;

		bool			IsMsgDispatchEnable() const;
		void			EnableMsgDispatch( bool bEnable );
		void			SetNetDelay( uint32_t nMinDelay, uint32_t nMaxDelay );

		void			EnableSendShellMsg( bool bValue );
		void			SetHeartBeatInterval( uint32_t nSeconds );
		virtual	void	OnHeartBeatStop(){};

		// 统计收发流量
		void			EnableProfile(uint8_t nSendIDBits, uint8_t nRecvIDBits);
		void			PrintMsgSize();
		size_t			GetSendBufferSize( uint16_t nShellID );
		size_t			GetRecvBufferSize( uint16_t nShellID );
		size_t			GetTotalSendSize();
		size_t			GetTotalRecvSize();

		virtual size_t	OnShellMsg( const void* pCmd, size_t nSize, bool bUnreliable ) = 0;	/// shell层消息
		virtual void	SendShellMsg( const SSendBuf aryBuffer[], uint32_t nBufferCount, bool bUnreliable = false );
		virtual void	SendShellMsg( const void* pData1, uint32_t nSize1, bool bUnreliable = false );
	};
}
