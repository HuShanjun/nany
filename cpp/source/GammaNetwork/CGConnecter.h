//=========================================================================
// CGConnecter.h 
// 定义socket连接类
// 柯达昭
// 2007-12-02
//=========================================================================
#ifndef __GAMMA_CONNECTER_H__
#define __GAMMA_CONNECTER_H__

#include "CGSocket.h"
#include "CGNetBuffer.h"
#include "GammaCommon/TGammaList.h"
#include "GammaCommon/TGammaRBTree.h"
#include <vector>
#include <string>

using namespace std;

namespace Gamma
{
	class CConnectingNode	: public TGammaList<CConnectingNode>::CGammaListNode {};
	class CDisconnectNode	: public TGammaList<CDisconnectNode>::CGammaListNode {};

	class CGConnecter 
		: public IConnecter
		, public CConnectingNode
		, public CDisconnectNode
	{
	protected:
        CGNetwork*			m_pNetwork;
		CGSocket*			m_pSocket;
		IConnectHandler*	m_pHandler;
		CAddress			m_LocalAddress;
		CAddress			m_RemoteAddress;
		int64				m_nCreateTime;
		int64				m_nConnectTime;
		int64				m_nFirstEvenTime;
		int64				m_nConnectOKTime;
		size_t				m_nTotalSend;
		size_t				m_nTotalRecv;
		EConnectState       m_eState;
		ECloseType			m_eCloseType;			
		bool				m_bEverConnected;

		void				SetState( EConnectState eState )	{ m_eState = eState; }

	public:
		CGConnecter( CGNetwork* pNetwork, CGSocket* pSocket );
		~CGConnecter();

		bool				IsValid();
		void				Close( ECloseType eCloseType );		
		void				CmdClose( bool bGrace );			
		void				Send( const void* pBuf, size_t nSize );
		void				CheckRecvBuff() {};
		void				Connect( CAddrResolutionDelegate* pAddrResolution );

		void				SetHandler( IConnectHandler* pHandler );
		const CAddress&		GetLocalAddress()	const;
		void				SetRemoteAddress( const CAddress& Address );

		IConnectHandler*	GetHandler()		const { return m_pHandler; }
		const CAddress&		GetRemoteAddress()	const;
		EConnecterType		GetConnectType()	const { return m_pSocket->GetConnectType(); }
													  
		bool				IsConnecting()		const { return eCS_Connecting == m_eState; }
		bool				IsConnected()		const { return eCS_Connected  == m_eState; }
		bool				IsDisconnected()	const { return eCS_Disconnected == m_eState; }
		bool				IsDisconnecting()	const { return eCS_Disconnecting == m_eState; }
		bool				IsEverConnected()	const { return m_bEverConnected; }
													  
		ECloseType			GetCloseType()		const { return m_eCloseType ; }
		size_t				GetTotalSendSize()	const { return m_nTotalSend; }
		size_t				GetTotalRecvSize()	const { return m_nTotalRecv; }
		int64				GetCreateTime()		const { return m_nCreateTime; };
		int64				GetConnectTime()	const { return m_nConnectTime; };
		int64				GetConnectOKTime()	const { return m_nConnectOKTime; };
		int64				GetFirstEvenTime()	const { return m_nFirstEvenTime; };

		void				OnEvent( bool bError, const tbyte* pData, uint32 nSize );
		virtual void		RecvData( const tbyte* pData, uint32 nSize ) = 0;
	};

    class CGConnecterTCP : public CGConnecter
	{
		CGNetRecvBuffer		m_RecvBuf;

	public:
        CGConnecterTCP( CGNetwork* pNetwork, CGSocket* pSocket );
		~CGConnecterTCP();

		void			CheckRecvBuff();
		virtual void	RecvData( const tbyte* pData, uint32 nSize );
    };

	class CGConnecterUDP  : public CGConnecter
	{
	public:
		CGConnecterUDP( CGNetwork* pNetwork, CGSocket* pSocket );
		~CGConnecterUDP();

		void			CheckRecvBuff() {};
		virtual void	RecvData( const tbyte* pData, uint32 nSize );
	};
}

#endif
