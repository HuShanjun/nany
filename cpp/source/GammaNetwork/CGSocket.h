//=========================================================================
// CGSocket.h 
// 定义socket连接基类
// 柯达昭
// 2007-12-02
//=========================================================================
#ifndef __GAMMA_SOCKET_H__
#define __GAMMA_SOCKET_H__

#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/TGammaList.h"
#include "GammaCommon/TConstString.h"
#include "GammaCommon/TCircleBuffer.h"
#include "GammaNetwork/INetHandler.h"
#include "GammaNetwork/INetworkInterface.h"
#include "GammaNetwork/CAddress.h"

namespace Gamma
{
	class CGSocket;
	class CGSocketUDP;
	class CGNetwork;
	struct SSendBuffer { union { tbyte aryData[8192]; SSendBuffer* pNext; }; };
	typedef TGammaList<CGSocket> CSocketList;
	typedef TGammaRBTree<CGSocketUDP> CGSocketUDPMap;

	struct SSendBufferAllocator
	{
		CGNetwork* m_pNetwork;
		SSendBufferAllocator( CGNetwork* pNetwork = nullptr );
		enum { eBlockSize = sizeof(SSendBuffer) };
		SSendBuffer* Alloc();
		void Free( void* pData );
	};

	class CGSocket 
		: public CSocketList::CGammaListNode
		, public TCircleBuffer<SSendBufferAllocator, true>
    {
    protected:
        CGNetwork*				m_pNetwork;
		CGNetThread*			m_pWorkThread;
		SOCKET					m_hSocket;
		bool					m_bListener;
		ptrdiff_t				m_nEventID;

		void*					m_pContext;
		gammacstring			m_strLocalSockAddr;
		gammacstring			m_strRemoteSockAddr;

		bool					m_bSendAllow;
		bool					m_bRecvAllow;

		virtual void			Create( int32 nAiFamily );
		bool					FetchLocalAddress();

		virtual bool			NT_ProcessListener( uint32 nEvent, bool bError ) = 0;
		virtual bool			NT_ProcessConnect( uint32 nEvent, bool bError ) = 0;
    public:
        friend class CGNetwork;
        CGSocket( CGNetwork* pNetwork );
		virtual ~CGSocket( void );
      
		void					Release();
		void					Bind( void* pContext );

		SOCKET					GetSocket() const { return m_hSocket; }
        bool					IsValid() const  { return m_hSocket != INVALID_SOCKET; }  
		bool					IsListener() const { return m_bListener; }
		gammacstring&			GetLocalAddress() { return m_strLocalSockAddr; }
		gammacstring&			GetRemoteAddress() { return m_strRemoteSockAddr; }
		void*					GetContext() const { return m_pContext; }
		bool					IsSendAllow() const;

		virtual EConnecterType	GetConnectType() const = 0;
		virtual ECloseType		Connect( const sockaddr* pAddr, uint32 nSize );
		virtual void			StartListener( const char* szAddres, uint16 nPort );
		virtual void			Send( const void* pBuf, size_t nSize ) = 0;

		ptrdiff_t				NT_GetEventID() const { return m_nEventID; }
		void					NT_SetEventID( ptrdiff_t nID ) { m_nEventID = nID; }

		virtual void			NT_Close();
		virtual bool			NT_ProcessEvent( uint32 nEvent, bool bError );
	};

	class CGSocketUDP : public CGSocket, public CGSocketUDPMap::CGammaRBTreeNode
	{
		// for listener
		CGSocketUDPMap			m_mapSockets;

		virtual bool			NT_ProcessListener( uint32 nEvent, bool bError );
		virtual bool			NT_ProcessConnect( uint32 nEvent, bool bError );
	public:
		CGSocketUDP( CGNetwork* pNetwork );
		virtual ~CGSocketUDP();

		virtual EConnecterType	GetConnectType() const { return eConnecterType_UDP; }
		virtual void			Send( const void* pBuf, size_t nSize );

		virtual void			NT_Close();

		bool					operator < ( const gammacstring& r ) const;
		operator				gammacstring() const { return m_strRemoteSockAddr; }
	};

	class CGSocketTCP : public CGSocket
	{
		virtual bool			NT_ProcessListener( uint32 nEvent, bool bError );
		virtual bool			NT_ProcessConnect( uint32 nEvent, bool bError );
		virtual CGSocketTCP*	NT_Accept( SOCKET hSocket, const sockaddr_in& addrRemote );
	public:
		CGSocketTCP( CGNetwork* pNetwork );
		virtual ~CGSocketTCP();

		virtual void			StartListener( const char* szAddres, uint16 nPort );
		virtual EConnecterType	GetConnectType() const { return eConnecterType_TCP; }
		virtual void			Send( const void* pBuf, size_t nSize );
	};

	class CGSocketTCPS : public CGSocketTCP
	{
		SSL_CTX*				m_sslContext;
		SSL*					m_sslSocket;
		uint8					m_sslState;
		bool					m_bReadToSSLWrite;
		bool					m_bWriteToSSLRead;

		virtual bool			NT_ProcessConnect( uint32 nEvent, bool bError );
		virtual CGSocketTCP*	NT_Accept( SOCKET hSocket, const sockaddr_in& addrRemote );
	public:
		CGSocketTCPS( CGNetwork* pNetwork, SSL_CTX* sslContext );
		virtual ~CGSocketTCPS();

		virtual void			Create( int32 nAiFamily );
		virtual EConnecterType	GetConnectType() const { return eConnecterType_TCPS; }

		virtual void			NT_Close();
	};


}

#endif
