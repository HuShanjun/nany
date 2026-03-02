
#include "GammaCommon/GammaHelp.h"
#include "GammaNetworkHelp.h"
#include "CGNetBuffer.h"
#include "CGNetwork.h"
#include "CGSocket.h"
#include "CGNetThread.h"
#include <sstream>
#include <errno.h>
#include <fcntl.h>

using namespace std;

namespace Gamma
{
	//========================================================
	// SSocketSendBuffer memory manager
	//========================================================
	SSendBufferAllocator::SSendBufferAllocator( CGNetwork* pNetwork ) 
		: m_pNetwork( pNetwork )
	{
	}

	SSendBuffer* SSendBufferAllocator::Alloc()
	{
		return m_pNetwork->Alloc();
	}

	void SSendBufferAllocator::Free( void* pData )
	{
		m_pNetwork->Free( (SSendBuffer*)pData );
	}

    //========================================================
    // SOCKET基类构造函数
    //========================================================
    CGSocket::CGSocket( CGNetwork* pNetwork )
        : TCircleBuffer<SSendBufferAllocator, true>( pNetwork )
		, m_pNetwork( pNetwork )
		, m_pWorkThread( nullptr )
        , m_hSocket(INVALID_SOCKET)
		, m_bListener( false )
		, m_pContext( nullptr )
		, m_nEventID( 0 )
		, m_bSendAllow( true )
		, m_bRecvAllow( false )
    {
		GammaAst( pNetwork );
    }

    CGSocket::~CGSocket(void)
	{
		GammaAst( !GetContext() );
		GammaAst( !IsInList() );
		m_pWorkThread = nullptr;
		m_pNetwork = nullptr;

		if( IsValid() )
		{
			closesocket( m_hSocket );
			m_hSocket = INVALID_SOCKET;
#ifdef _WIN32
			while( SleepEx( 0, TRUE ) );
#endif
		}
    }

    //========================================================
    // 创建socket
    //========================================================
    void CGSocket::Create( int32 nAiFamily )
    {
        //创建socket
		bool bUDP = GetConnectType() == eConnecterType_UDP;
		uint32 nType = bUDP ? SOCK_DGRAM : SOCK_STREAM;
        m_hSocket = (uint32)socket( nAiFamily, nType, 0 );
        if( !IsValid() )
        {
            ostringstream strm;
            strm<<"socket failed with error code "<< GNWGetLastError() <<"."<<ends;
            GammaThrow( strm.str() );
        } 

#ifdef _WIN32
        u_long nParam = 1;
        if( SOCKET_ERROR == ioctlsocket( m_hSocket, FIONBIO, &nParam ) )
        {
            closesocket( m_hSocket );
            m_hSocket = INVALID_SOCKET;
            ostringstream strm;
            strm<<"ioctlsocket failed with error code "<< GNWGetLastError() <<"."<<ends;
            GammaThrow( strm.str() );
        }
#else
		int32 opts;
		opts = fcntl( m_hSocket, F_GETFL );
		if( opts < 0)
		{
           closesocket( m_hSocket );
            m_hSocket = INVALID_SOCKET;
            ostringstream strm;
            strm<<"fcntl get failed with error code "<< GNWGetLastError() <<"."<<ends;
            GammaThrow( strm.str() );			
		}
		opts |= O_NONBLOCK;
		if( fcntl( m_hSocket, F_SETFL, opts ) < 0 )
		{
           closesocket( m_hSocket );
            m_hSocket = INVALID_SOCKET;
            ostringstream strm;
            strm<<"fcntl set failed with error code "<< GNWGetLastError() <<"."<<ends;
            GammaThrow( strm.str() );
		}
#endif

		int32 nVal = 1;
		if( SOCKET_ERROR == setsockopt( m_hSocket, SOL_SOCKET, 
			SO_REUSEADDR, (const char*)(&nVal), sizeof(nVal) ) )
		{
			closesocket( m_hSocket );
			m_hSocket = INVALID_SOCKET;
			ostringstream strm;
			strm<<"setsockopt failed with error code "<< GNWGetLastError() <<"."<<ends;
			GammaThrow( strm.str() );
		}

		if( nType != SOCK_STREAM )
			return;

		if( SOCKET_ERROR == setsockopt( m_hSocket, IPPROTO_TCP, 
			TCP_NODELAY, (const char*)(&nVal), sizeof(nVal) ) )
		{
			closesocket( m_hSocket );
			m_hSocket = INVALID_SOCKET;
			ostringstream strm;
			strm<<"setsockopt failed with error code "<< GNWGetLastError() <<"."<<ends;
			GammaThrow( strm.str() );
		}
    }

   //========================================================
    // 关闭socket
    //========================================================
	void CGSocket::Release()
	{
		GammaAst( m_pWorkThread && m_pContext );
		m_pWorkThread->CloseSocket( this );
		m_pContext = nullptr;
	}

	void CGSocket::Bind( void* pContext )
	{
		GammaAst( m_pContext == nullptr && m_pNetwork && pContext );
		m_pContext = pContext;

		// 对于服务端的UDP的socket而言，和listen是共用的，
		// 没有办法将不同的连接数据收发分配到不同网络线程，
		// 因此绑定在listener绑定的网络线程中完成
		if( m_pWorkThread != nullptr )
			return;
		m_pWorkThread = m_pNetwork->GetMinSocketThread();
		m_pWorkThread->AddSocket( this );
	}

	bool CGSocket::FetchLocalAddress()
	{
		GammaAst( m_hSocket != INVALID_SOCKET );

		char szAddress[TMax<sizeof( sockaddr_in ), sizeof( sockaddr_in6 )>::eValue];
		socklen_t nSize = sizeof( szAddress );
		memset( &szAddress, 0, sizeof( szAddress ) );

		if( getsockname( m_hSocket, (sockaddr*)szAddress, &nSize ) )
		{
			if( m_pNetwork && m_pNetwork->IsLogEnable() )
				GammaLog << "FetchLocalAddress() failed on error:" << GNWGetLastError() << endl;
			return false;
		}

		m_strLocalSockAddr.assign( szAddress, (uint32)nSize );
		return true;
	}

	bool CGSocket::IsSendAllow() const
	{
		return IsValid() && !IsListener() && m_bSendAllow && 
			!m_strLocalSockAddr.empty() && !m_strRemoteSockAddr.empty();
	}

	ECloseType CGSocket::Connect( const sockaddr* pAddr, uint32 nSize )
	{
		m_strRemoteSockAddr.assign( (const char*)pAddr, nSize );
		Create( pAddr->sa_family );

		int32 nResult = connect( m_hSocket, pAddr, nSize );
		if( SOCKET_ERROR == nResult )
		{
			uint32 nError = GNWGetLastError();
#ifdef _WIN32
			if( nError != NE_EWOULDBLOCK )
#else
			if( nError != NE_EWOULDBLOCK && nError != NE_EINPROGRESS )
#endif
			{
				if( m_pNetwork && m_pNetwork->IsLogEnable() )
					GammaLog << "Connect failed with error code " << nError << "." << endl;
				return eCE_ConnectRefuse;
			}
		}

		m_pWorkThread->StartSocket( this );
		return eCE_Null;
	}

	void CGSocket::StartListener( const char* szAddres, uint16 nPort )
	{
		m_bListener = true;
		Create( AF_INET );

		sockaddr_in Address;
		memset( &Address, 0, sizeof( Address ) );
		Address.sin_addr.s_addr = inet_addr( szAddres );
		Address.sin_port = htons( nPort );
		Address.sin_family = AF_INET;

		if( ::bind( m_hSocket, (sockaddr*)(&Address), sizeof( sockaddr ) ) )
		{
			ostringstream strm;
			strm << "bind failed with error code " << GNWGetLastError() << "." << ends;
			closesocket( m_hSocket );
			m_hSocket = INVALID_SOCKET;
			GammaThrow( strm.str() );
		}

		m_strLocalSockAddr.assign( (const char*)&Address, sizeof( sockaddr ), false );
		m_pWorkThread->StartSocket( this );
	}

	void CGSocket::NT_Close()
	{
		Remove();

		if( IsValid() )
		{
			closesocket( m_hSocket );
			m_hSocket = INVALID_SOCKET;
#ifdef _WIN32
			while( SleepEx( 0, TRUE ) );
#endif
		}
	}

	bool CGSocket::NT_ProcessEvent( uint32 nEvent, bool bError )
	{
		if( IsListener() )
			return NT_ProcessListener( nEvent, bError );
		return NT_ProcessConnect( nEvent, bError );
	}

	//---------------------------------------------------------
	// CGSocketUDP
	//---------------------------------------------------------
	CGSocketUDP::CGSocketUDP( CGNetwork* pNetwork )
		: CGSocket( pNetwork )
	{
	}

	CGSocketUDP::~CGSocketUDP()
	{
	}

	bool CGSocketUDP::NT_ProcessListener( uint32 nEvent, bool bError )
	{
		GammaAst( IsValid() );

		if( nEvent & eNE_ConnectedRead )
		{
			char szRecvBuf[MAX_UDP_SIZE];
			char szAddress[TMax<sizeof( sockaddr_in ), sizeof( sockaddr_in6 )>::eValue];

			while( true )
			{
				socklen_t nLen = sizeof( szAddress );
				int32 nResult = (int32)recvfrom( m_hSocket,
					szRecvBuf, MAX_UDP_SIZE, 0, (sockaddr*)szAddress, &nLen );
				if( nResult == SOCKET_ERROR )
					break;

				gammacstring strKey( szAddress, (uint32)nLen );
				auto pSocket = m_mapSockets.Find( strKey );
				if( pSocket == NULL )
				{
					pSocket = new CGSocketUDP( m_pNetwork );
					m_mapSockets.Insert( *pSocket );
					pSocket->m_strRemoteSockAddr.assign(
						szAddress, nLen, false );
					pSocket->m_strLocalSockAddr.assign(
						m_strLocalSockAddr.c_str(), m_strLocalSockAddr.size(), false );
					pSocket->m_hSocket = m_hSocket;
					pSocket->m_pWorkThread = m_pWorkThread;
					m_pWorkThread->NT_OnAccept( pSocket, m_pContext );
				}

				m_pWorkThread->NT_RecieveData( pSocket, szRecvBuf, nResult );
			}
		}

		if( nEvent & eNE_ConnectedWrite )
		{
			for( auto pSocket = m_mapSockets.GetFirst(); pSocket; 
				pSocket = pSocket->CGSocketUDPMap::CGammaRBTreeNode::GetNext() )
			{
				pSocket->NT_ProcessEvent( eNE_ConnectedWrite, false );
			}			
		}

		return true;
	}

	bool CGSocketUDP::NT_ProcessConnect( uint32 nEvent, bool bError )
	{
		if( bError )
		{
			m_pWorkThread->NT_RecieveData( this, "", 0 );
			return false;
		}

		// 这是发起connect动作的连接，不是accept的
		if( !m_strRemoteSockAddr.empty() && m_strLocalSockAddr.empty() )
		{
			// linux上有一个bug，
			// 当连接本地端口时如果系统分配的端口正好等于要连接的端口，
			// 居然可以连接的上，这是不应该的
			if( !FetchLocalAddress() || m_strLocalSockAddr == m_strRemoteSockAddr )
			{
				m_pWorkThread->NT_RecieveData( this, "", 0 );
				return false;
			}

			m_pWorkThread->NT_DelEvent( this );
			sockaddr* addr = (sockaddr*)m_strLocalSockAddr.c_str();

			// UDP客户端使用了connect以后，就绑定了远端地址，
			// 因此不能再接收别的地址发过来的消息
			// 某些服务器上发送的socket和接受的socket不是同一个，导致接收不到消息
			// 最稳妥的方式还是基于非连接型的
			// 非连接型的很难获取正确的本地地址
			// 因此采用先连接获取本地地址，再绑定的方式
			closesocket( m_hSocket );
			Create( addr->sa_family );

			if( FAILED( ::bind( m_hSocket, addr, m_strLocalSockAddr.size() ) ) )
			{
				m_pWorkThread->NT_RecieveData( this, "", 0 );
				return false;
			}

			m_pWorkThread->NT_OnConnected( this );
		}

		char aryBuffer[MAX_UDP_SIZE];
		if( nEvent & eNE_ConnectedRead )
		{
			m_bRecvAllow = true;
			char szAddress[TMax<sizeof( sockaddr_in ), sizeof( sockaddr_in6 )>::eValue];

			while( !bError && m_bRecvAllow )
			{
				socklen_t nLen = sizeof( szAddress );
				int32 nResult = recvfrom( m_hSocket, 
					aryBuffer, MAX_UDP_SIZE, 0, (sockaddr*)szAddress, &nLen );
				if( nResult != SOCKET_ERROR )
				{
					m_pWorkThread->NT_RecieveData( this, aryBuffer, nResult );
					// 正常关闭
					if( nResult == 0 )
						break;
					continue;
				}

				if( NE_EWOULDBLOCK == GNWGetLastError() )
					m_bRecvAllow = false;
				else
					bError = true;
				break;
			}
		}

		const sockaddr* addrRemote = (const sockaddr*)m_strRemoteSockAddr.c_str();
		uint32 nAddrSize = (uint32)m_strRemoteSockAddr.size();
		if( !bError && ( nEvent & eNE_ConnectedWrite ) )
		{
			m_bSendAllow = true;
			SQuerryNodePtr pWriteBuffer = m_pWriteBuffer;
			uint32 nWritePos = pWriteBuffer->m_nWritePos;

			// 尚未写入完毕（当前已经写入结束的Buffer的位置不可能是nNodeBufferSize）
			if( nWritePos == nNodeBufferSize )
				return true;

			SQuerryNodePtr pReadBuffer = m_pReadBuffer;
			uint32 nReadPos = pReadBuffer->m_nReadPos;
			// 没有写入完整的数据可读
			if( pReadBuffer == pWriteBuffer &&
				nReadPos == nWritePos )
				return true;

			while( !bError && m_bSendAllow )
			{
				if( pReadBuffer->m_nWritePos == nReadPos )
					break;
				uint32 nBufferSize;
				ReadBuffer( pReadBuffer, nReadPos, &nBufferSize, sizeof( uint32 ) );
				ReadBuffer( pReadBuffer, nReadPos, aryBuffer, nBufferSize );
				int32 nResult = sendto( m_hSocket, aryBuffer, nBufferSize,
					0, addrRemote, nAddrSize );

				if( nResult != SOCKET_ERROR )
				{
					// 保证原子操作，一次query完整读出
					pReadBuffer->m_nReadPos = nReadPos;
					m_pReadBuffer = pReadBuffer;
					m_nPopCount++;
					continue;
				}

				if( NE_EWOULDBLOCK == GNWGetLastError() )
					m_bSendAllow = false;
				else
					bError = true;
				break;
			}
		}

		if( !bError )
			return true;
		m_pWorkThread->NT_RecieveData( this, "", 0 );
		return false;
	}

	void CGSocketUDP::Send( const void* pBuf, size_t nSize )
	{
		if( nSize > MAX_UDP_SIZE )
			GammaThrow( "UDP segment size must less than 1400!!!" );
		GammaAst( m_pWorkThread );

		SQuerryNodePtr pBufferStart = m_pWriteBuffer;
		SQuerryNodePtr pBufferEnd = m_pWriteBuffer;
		SQuerryNodePtr pReadBuffer = m_pReadBuffer;
		uint32 nWritePos = pBufferStart->m_nWritePos;
		WriteBuffer( pBufferEnd, nWritePos, pReadBuffer, &nSize, sizeof(uint32) );
		WriteBuffer( pBufferEnd, nWritePos, pReadBuffer, pBuf, nSize );

		// 保证原子操作，一次query完整写入
		while( pBufferStart != pBufferEnd )
		{
			pBufferStart->m_nWritePos = nNodeBufferSize;
			pBufferStart = pBufferStart->m_pNextBuffer;
		}

		pBufferEnd->m_nWritePos = nWritePos;
		m_pWriteBuffer = pBufferEnd;
		m_nPushCount++;
	}

	void CGSocketUDP::NT_Close()
	{
		CGSocket::NT_Close();
		CGSocketUDPMap::CGammaRBTreeNode::Remove();
	}

	bool CGSocketUDP::operator< ( const gammacstring& r ) const
	{
		return m_strRemoteSockAddr < r;
	}

	//---------------------------------------------------------
	// CGSocketTCP
	//---------------------------------------------------------
	CGSocketTCP::CGSocketTCP( CGNetwork* pNetwork )
		: CGSocket( pNetwork )
	{
	}

	CGSocketTCP::~CGSocketTCP()
	{
	}

	void CGSocketTCP::StartListener( const char* szAddres, uint16 nPort )
	{
		CGSocket::StartListener( szAddres, nPort );
		if( listen( m_hSocket, INVALID_16BITID ) )
		{
			closesocket( m_hSocket );
			m_hSocket = INVALID_SOCKET;
			ostringstream strm;
			strm << "listen failed with error code:" << GNWGetLastError() << "." << ends;
			GammaThrow( strm.str() );
		}
	}

	bool CGSocketTCP::NT_ProcessListener( uint32 nEvent, bool bError )
	{
		GammaAst( IsValid() );

		if( !(nEvent & eNE_Listen) )
			return true;

		for( ;; )
		{
			sockaddr_in Address;
			socklen_t nSize = sizeof( sockaddr_in );
			SOCKET hSocket = (SOCKET)accept( m_hSocket, (sockaddr*)&Address, &nSize );

			if( INVALID_SOCKET == hSocket )
			{
				int32 nErrorCode = GNWGetLastError();
				switch( nErrorCode )
				{
#ifndef _WIN32
				case NE_ECONNABORTED:
				case NE_EWOULDBLOCK:
#else
				case WSAEWOULDBLOCK:
#endif
				case EMFILE:
					return true;
				}

				ostringstream strm;
				strm << "accept failed with error code:" << nErrorCode << "." << ends;
				GammaThrow( strm.str() );
			}

			u_long uParam = 1;
			if( SOCKET_ERROR == ioctlsocket( hSocket, FIONBIO, &uParam ) )
			{
				closesocket( hSocket );
				m_hSocket = INVALID_SOCKET;

				ostringstream strm;
				strm << "ioctlsocket failed with error code " << GNWGetLastError() << "." << ends;
				GammaThrow( strm.str() );
			}

			m_pWorkThread->NT_OnAccept( NT_Accept( hSocket, Address ), m_pContext );
		}
	}

	CGSocketTCP* CGSocketTCP::NT_Accept( SOCKET hSocket, const sockaddr_in& addrRemote )
	{
		CGSocketTCP* pNewSocket = new CGSocketTCP( m_pNetwork );
		pNewSocket->m_strRemoteSockAddr.assign( 
			(const char*)&addrRemote, sizeof( sockaddr_in ), false );
		pNewSocket->m_strLocalSockAddr.assign(
			m_strLocalSockAddr.c_str(), m_strLocalSockAddr.size(), false );
		pNewSocket->m_hSocket = hSocket;
		return pNewSocket;
	}

	bool CGSocketTCP::NT_ProcessConnect( uint32 nEvent, bool bError )
	{
		if( bError )
		{
			m_pWorkThread->NT_RecieveData( this, "", 0 );
			return false;
		}

		// 这是发起connect动作的连接，不是accept的
		if( !m_strRemoteSockAddr.empty() && m_strLocalSockAddr.empty() )
		{
			if( nEvent&eNE_Connecting )
			{
				if( !FetchLocalAddress() || m_strLocalSockAddr == m_strRemoteSockAddr )
				{
					m_pWorkThread->NT_RecieveData( this, "", 0 );
					return false;
				}

				m_pWorkThread->NT_OnConnected( this );
				nEvent |= eNE_ConnectedRead;
			}
		}
		
		if( nEvent & eNE_ConnectedRead )
		{
			m_bRecvAllow = true;
			char aryBuffer[65536];

			while( !bError && m_bRecvAllow )
			{
				int32 nResult = recv( m_hSocket,
					aryBuffer, ELEM_COUNT(aryBuffer), 0 );
				if( nResult != SOCKET_ERROR )
				{
					m_pWorkThread->NT_RecieveData( this, aryBuffer, nResult );
					// 正常关闭
					if( nResult == 0 )
						break;
					continue;
				}
				
				if( NE_EWOULDBLOCK == GNWGetLastError() )
					m_bRecvAllow = false;
				else
					bError = true;
				break;
			}
		}

		if( !bError && (nEvent & eNE_ConnectedWrite) )
		{
			m_bSendAllow = true;
			bool bError = false;

			SQuerryNodePtr pWriteBuffer = m_pWriteBuffer;
			uint32 nWritePos = pWriteBuffer->m_nWritePos;

			// 尚未写入完毕（当前已经写入结束的Buffer的位置不可能是nNodeBufferSize）
			if( nWritePos == nNodeBufferSize )
				return true;

			SQuerryNodePtr pReadBuffer = m_pReadBuffer;
			uint32 nReadPos = pReadBuffer->m_nReadPos;
			// 没有写入完整的数据可读
			if( pReadBuffer == pWriteBuffer &&
				nReadPos == nWritePos )
				return true;

			while( !bError && m_bSendAllow )
			{
				uint32 nWritePos = pReadBuffer->m_nWritePos;
				uint32 nLeftSize = nWritePos - nReadPos;
				if( nLeftSize == 0 )
					break;
				const tbyte* pSrcBuf = pReadBuffer->m_aryBuffer + nReadPos;
				int32 nResult = (int32)send( m_hSocket,
					(const char*)pSrcBuf, (int32)nLeftSize, 0 );
				if( nResult != SOCKET_ERROR )
				{
					nReadPos += nResult;
					if( nReadPos != nNodeBufferSize )
						break;
					pReadBuffer = pReadBuffer->m_pNextBuffer;
					nReadPos = 0;
					continue;
				}

				if( NE_EWOULDBLOCK == GNWGetLastError() )
					m_bSendAllow = false;
				else
					bError = true;
				break;
			}

			// 保证原子操作，一次query完整读出
			pReadBuffer->m_nReadPos = nReadPos;
			m_pReadBuffer = pReadBuffer;
			m_nPopCount++;
		}

		if( !bError )
			return true;
		m_pWorkThread->NT_RecieveData( this, "", 0 );
		return false;
	}

	void CGSocketTCP::Send( const void* pBuf, size_t nSize )
	{
		GammaAst( m_pWorkThread );
		SSendBuf SendBuffer( pBuf, nSize );
		PushRaw( &SendBuffer, 1 );
	}

	//---------------------------------------------------------
	// CGSocketTCPS
	//---------------------------------------------------------
	CGSocketTCPS::CGSocketTCPS( CGNetwork* pNetwork, SSL_CTX* sslContext )
		: CGSocketTCP( pNetwork )
		, m_sslContext( sslContext )
		, m_sslSocket( nullptr )
		, m_bReadToSSLWrite( false )
		, m_bWriteToSSLRead( false )
		, m_sslState( eCS_Unknow )
	{
		GammaAst( m_sslContext );
	}

	CGSocketTCPS::~CGSocketTCPS()
	{
	}

	void CGSocketTCPS::Create( int32 nAiFamily )
	{
		CGSocket::Create( nAiFamily );
		m_sslSocket = SSL_new( m_sslContext );
		SSL_set_fd( m_sslSocket, m_hSocket );
		SSL_set_connect_state( m_sslSocket );
	}

	void CGSocketTCPS::NT_Close()
	{
		if( m_sslSocket != nullptr )
		{
			if( eCS_Disconnected != m_sslState )
				SSL_shutdown( m_sslSocket );
			SSL_free( m_sslSocket );
			m_sslContext = nullptr;
			m_sslSocket = nullptr;
		}
		CGSocketTCP::NT_Close();
	}

	bool CGSocketTCPS::NT_ProcessConnect( uint32 nEvent, bool bError )
	{
		if( bError )
		{
			m_pWorkThread->NT_RecieveData( this, "", 0 );
			return false;
		}

		// 这是发起connect动作的连接，不是accept的
		if( !m_strRemoteSockAddr.empty() && m_strLocalSockAddr.empty() )
		{
			int32 nResult = SSL_do_handshake( m_sslSocket );
			m_sslState = eCS_Connecting;
			if( 1 != nResult )
			{
				int32 nError = 0;
				// 没有立刻握手成功，需要通过错误码来判断现在的状态
				nError = SSL_get_error( m_sslSocket, nResult );
				if( SSL_ERROR_WANT_READ == nError || SSL_ERROR_WANT_WRITE == nError )
				{
					m_pWorkThread->NT_SetEvent( this, eNE_ConnectedWrite | eNE_ConnectedRead | eNE_Error );
					return true;
				}

				if( m_pNetwork && m_pNetwork->IsLogEnable() )
					GammaLog << "SSL handshake Error( " << nResult << "," << nError << " )\n";
				m_pWorkThread->NT_RecieveData( this, "", 0 );
				return false;
			}
			
			if( !FetchLocalAddress() || m_strLocalSockAddr == m_strRemoteSockAddr )
			{
				m_pWorkThread->NT_RecieveData( this, "", 0 );
				return false;
			}

			m_pWorkThread->NT_OnConnected( this );
			m_sslState = eCS_Connected;
			nEvent |= eNE_ConnectedRead|eNE_ConnectedWrite;
		}

		uint32 nEventForSSL = nEvent;
		if( (nEvent & eNE_ConnectedRead) && m_bReadToSSLWrite )
		{
			nEventForSSL &= ~eNE_ConnectedRead;
			nEventForSSL |= ~eNE_ConnectedWrite;
		}

		if( (nEvent & eNE_ConnectedWrite) && m_bWriteToSSLRead )
		{
			nEventForSSL &= ~eNE_ConnectedRead;
			nEventForSSL |= ~eNE_ConnectedWrite;
		}
		nEvent = nEventForSSL;

		if( nEvent & eNE_ConnectedRead )
		{
			m_bRecvAllow = true;
			char aryBuffer[65536];

			while( !bError && m_bRecvAllow )
			{
				int32 nResult = SOCKET_ERROR;
				uint32 nError = 0;

				nResult = (int32)SSL_read( m_sslSocket, 
					aryBuffer, ELEM_COUNT(aryBuffer) );

				if( nResult >= 0 )
				{
					m_pWorkThread->NT_RecieveData( this, aryBuffer, nResult );
					// 正常关闭
					if( nResult == 0 )
						break;
					continue;
				}
				else
				{
					nError = SSL_get_error( m_sslSocket, nResult );
					if( nError == SSL_ERROR_WANT_READ )
					{
						nError = NE_EWOULDBLOCK;
						m_bReadToSSLWrite = false;
					}
					else if( nError == SSL_ERROR_WANT_WRITE )
					{
						nError = NE_EWOULDBLOCK;
						m_bWriteToSSLRead = true;
					}
					else
					{
						nError = 0x8000 + nError;
						m_bReadToSSLWrite = false;
						m_bWriteToSSLRead = false;
					}
				}

				if( NE_EWOULDBLOCK == nError )
					m_bRecvAllow = false;
				else
					bError = true;
				break;
			}
		}

		if( !bError && (nEvent & eNE_ConnectedWrite) )
		{
			m_bSendAllow = true;
			SQuerryNodePtr pWriteBuffer = m_pWriteBuffer;
			uint32 nWritePos = pWriteBuffer->m_nWritePos;

			// 尚未写入完毕（当前已经写入结束的Buffer的位置不可能是nNodeBufferSize）
			if( nWritePos == nNodeBufferSize )
				return true;

			SQuerryNodePtr pReadBuffer = m_pReadBuffer;
			uint32 nReadPos = pReadBuffer->m_nReadPos;
			// 没有写入完整的数据可读
			if( pReadBuffer == pWriteBuffer &&
				nReadPos == nWritePos )
				return true;

			while( !bError && m_bSendAllow )
			{
				uint32 nError = 0;
				uint32 nWritePos = pReadBuffer->m_nWritePos;
				uint32 nLeftSize = nWritePos - nReadPos;
				if( nLeftSize == 0 )
					break;
				const tbyte* pSrcBuf = pReadBuffer->m_aryBuffer + nReadPos;

				int32 nResult = (int32)SSL_write( m_sslSocket, 
					(const char*)pSrcBuf, (int32)nLeftSize );
				if( nResult >= 0 )
				{
					nReadPos += nResult;
					if( nReadPos != nNodeBufferSize )
						break;
					pReadBuffer = pReadBuffer->m_pNextBuffer;
					nReadPos = 0;
					continue;
				}

				nError = SSL_get_error( m_sslSocket, nResult );
				if( nError == SSL_ERROR_WANT_READ )
				{
					nError = NE_EWOULDBLOCK;
					m_bReadToSSLWrite = true;
				}
				else if( nError == SSL_ERROR_WANT_WRITE )
				{
					nError = NE_EWOULDBLOCK;
					m_bWriteToSSLRead = false;
				}
				else
				{
					nError = 0x8000 + nError;
					m_bReadToSSLWrite = false;
					m_bWriteToSSLRead = false;
				}

				if( NE_EWOULDBLOCK == nError )
					m_bSendAllow = false;
				else
					bError = true;
				break;
			}

			// 保证原子操作，一次query完整读出
			pReadBuffer->m_nReadPos = nReadPos;
			m_pReadBuffer = pReadBuffer;
			m_nPopCount++;
		}

		if( !bError )
			return true;
		m_pWorkThread->NT_RecieveData( this, "", 0 );
		return true;
	}

	CGSocketTCP* CGSocketTCPS::NT_Accept( SOCKET hSocket, const sockaddr_in& addrRemote )
	{
		CGSocketTCPS* pNewSocket = new CGSocketTCPS( m_pNetwork, m_sslContext );
		pNewSocket->m_strRemoteSockAddr.assign(
			(const char*)&addrRemote, sizeof( sockaddr_in ), false );
		pNewSocket->m_strLocalSockAddr.assign(
			m_strLocalSockAddr.c_str(), m_strLocalSockAddr.size(), false );
		pNewSocket->m_hSocket = hSocket;
		pNewSocket->m_sslSocket = SSL_new( m_sslContext );
		SSL_set_fd( pNewSocket->m_sslSocket, hSocket );
		SSL_set_accept_state( pNewSocket->m_sslSocket );
		return pNewSocket;
	}
}
