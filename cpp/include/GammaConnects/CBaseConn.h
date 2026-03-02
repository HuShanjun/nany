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
		uint32			GetPingDelay() const;

		const CAddress&	GetRemoteAddress() const;
		const CAddress& GetLocalAddress() const;

		bool			IsMsgDispatchEnable() const;
		void			EnableMsgDispatch( bool bEnable );
		void			SetNetDelay( uint32 nMinDelay, uint32 nMaxDelay );

		void			EnableSendShellMsg( bool bValue );
		void			SetHeartBeatInterval( uint32 nSeconds );
		virtual	void	OnHeartBeatStop(){};

		// 统计收发流量
		void			EnableProfile(uint8 nSendIDBits, uint8 nRecvIDBits);
		void			PrintMsgSize();
		size_t			GetSendBufferSize( uint16 nShellID );
		size_t			GetRecvBufferSize( uint16 nShellID );
		size_t			GetTotalSendSize();
		size_t			GetTotalRecvSize();

		virtual size_t	OnShellMsg( const void* pCmd, size_t nSize, bool bUnreliable ) = 0;	/// shell层消息
		virtual void	SendShellMsg( const SSendBuf aryBuffer[], uint32 nBufferCount, bool bUnreliable = false );
		virtual void	SendShellMsg( const void* pData1, uint32 nSize1, bool bUnreliable = false );
	};
}
