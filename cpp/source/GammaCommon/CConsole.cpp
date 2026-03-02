
#include <fcntl.h>
#include <iostream>

#include "GammaCommon/ILog.h"
#include "GammaCommon/CThread.h"
#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/GammaCodeCvs.h"
#include "GammaCommon/CPkgFile.h"
#include "GammaCommon/GammaSignal.h"
#include "CConsole.h"

#ifdef _WIN32
	#include <winsock2.h>
	#include <Windows.h>
	#include <io.h>
	#include <stdio.h>
	#include <signal.h>
	#pragma comment( linker, "/defaultlib:ws2_32.lib" )
#else
	#include <alloca.h>
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <sys/ioctl.h>
	#include <unistd.h>
	#include <cerrno>
	#include <netdb.h>
	#define INVALID_SOCKET		-1
	#define closesocket			close
	#define ioctlsocket			ioctl
	typedef int32				SOCKET;
	typedef struct linger		LINGER;
	#define SOCKET_ERROR		-1
	#define SD_SEND				SHUT_WR
#endif

#ifdef _ANDROID
	#include <typeinfo>
	#include <sys/select.h>
	#include <android/log.h>
	#include <alloca.h>
#endif

namespace Gamma
{
	CConsole::CConsole()
		: m_nRemoteDebugConsole( INVALID_32BITID )
#ifndef OUTPUT_ON_MAIN_THREAD 
		, m_hSemConsole( GammaCreateSemaphore() )
#endif
		, m_hLockRead( GammaCreateLock() )
#ifdef _WIN32
		, m_nLogLevel(0)
		, m_hLockConsole( GammaCreateLock() )
		, m_nCurCount( 0 )
		, m_bAllocConsole( false )
#endif
	{
#ifdef _WIN32
		memset( m_fdAllocNew, 0, sizeof(m_fdAllocNew) );
		memset( m_hConsole, 0, sizeof(m_hConsole) );
#endif

#ifndef OUTPUT_ON_MAIN_THREAD 
		GammaCreateThread( &m_hThread, 0, (THREADPROC)&ThreadFun, this );
#endif
	}

	CConsole::~CConsole( void )
	{
#ifndef OUTPUT_ON_MAIN_THREAD 
		GammaPutSemaphore( m_hSemConsole );
		GammaSleep( 500 );
		GammaTerminateThread( m_hThread, 0 );
		GammaDestroySemaphore( m_hSemConsole );
#endif
		GammaDestroyLock( m_hLockRead );
		Write2Console();
		Redirect2StdConsole( false );

#ifdef _WIN32
		GammaDestroyLock( m_hLockConsole );
#endif
	}

	void CConsole::Redirect2StdConsole( bool bEnable )
	{
#ifdef _WIN32
		if( bEnable == !!m_hConsole[eInput] )
			return;

		if( !bEnable )
		{
			GammaLock( m_hLockConsole );
			FILE* fdCur[eCount] = { stdin, stdout, stderr };
			for( size_t i = 0; i < eCount; i++ )
			{
				std::swap( *fdCur[i], *m_fdAllocNew[i] );
				m_fdAllocNew[i] = NULL;
			}

			if( m_bAllocConsole )
				::FreeConsole();
			memset( m_hConsole, 0, sizeof( m_hConsole ) );
			m_bAllocConsole = false;
			GammaUnlock( m_hLockConsole );
			return;
		}

		if( !GetConsoleWindow() )
		{
			if( !::AllocConsole() )
				return;
			m_bAllocConsole = true;
		}

		GammaLock( m_hLockConsole );
		struct SWin32Handler
		{ 
			static BOOL WINAPI OnSignal( DWORD nControlType )
			{
				if( nControlType != CTRL_C_EVENT && 
					nControlType != CTRL_CLOSE_EVENT )
					return false;
				RaiseSignal( SIGTERM );
				return true;
			}
		};
		
		uint32 hStandar[eCount] = { STD_INPUT_HANDLE, STD_OUTPUT_HANDLE, STD_ERROR_HANDLE };
		const char* aryFlag[eCount] = { "r", "w", "w" };

		for( size_t i = 0; i < eCount; i++ )
		{
			m_hConsole[i] = ::GetStdHandle( hStandar[i] );
			int32 handler = _open_osfhandle( (ptrdiff_t)m_hConsole[i], _O_TEXT );
			m_fdAllocNew[i] = _fdopen( handler, aryFlag[i] );
		}

		GammaVer( ::SetConsoleMode( m_hConsole[eInput], ENABLE_ECHO_INPUT|ENABLE_PROCESSED_INPUT|
			ENABLE_PROCESSED_OUTPUT|ENABLE_WRAP_AT_EOL_OUTPUT) );
		TCHAR path[MAX_PATH] = {0};
		::GetModuleFileName(NULL, path, sizeof(path));
		::SetConsoleTitle( path );
		::SetConsoleTextAttribute( m_hConsole[eOutput], 7 );

		if( !SetConsoleCtrlHandler( &SWin32Handler::OnSignal, TRUE ) )
			GammaThrow( L"system call SetConsoleCtrlHandler failed." );

		FILE* fdCur[eCount] = { stdin, stdout, stderr };
		for( size_t i = 0; i < eCount; i++ )
			std::swap( *fdCur[i], *m_fdAllocNew[i] );
		GammaUnlock( m_hLockConsole );
#endif

		Resize( 80, 5000 );
	}

	void CConsole::Redirect2Remote( const char* szHost, uint16 nPort )
	{
		if( m_nRemoteDebugConsole != INVALID_SOCKET )
			closesocket( m_nRemoteDebugConsole );

		sockaddr_in addrHttp;
		memset( &addrHttp, 0, sizeof(addrHttp) );
		addrHttp.sin_port = htons( (u_short)nPort );
		addrHttp.sin_addr.s_addr = inet_addr( szHost );
		addrHttp.sin_family = AF_INET;

		m_nRemoteDebugConsole = (int32)socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
		if( m_nRemoteDebugConsole == INVALID_SOCKET )
			return;

		if( !connect( m_nRemoteDebugConsole,(struct sockaddr*)&addrHttp, sizeof(addrHttp) ) )
			return;
		closesocket( m_nRemoteDebugConsole );
		m_nRemoteDebugConsole = (int32)INVALID_SOCKET;
	}

	void CConsole::Resize( int32 x, int32 y )
	{
#ifdef _WIN32
		if( !m_hConsole[eOutput] )
			return;

		GammaLock( m_hLockConsole );
		COORD crd = { (int16)x, (int16)y };
		SetConsoleScreenBufferSize( m_hConsole[eOutput], crd );
		SMALL_RECT rc = { 0, 0, (int16)( x - 1 ), 25 };
		SetConsoleWindowInfo( m_hConsole[eOutput], true, &rc );
		GammaUnlock( m_hLockConsole );
#endif
	}

	void CConsole::Write( const char* szOut, uint32 nSize, uint32 nLevel)
	{
		SSendBuf SendBuff( szOut, nSize );
		m_OutputBuffer.PushBuffer(nLevel, &SendBuff, 1, false );
#ifndef OUTPUT_ON_MAIN_THREAD 
		GammaPutSemaphore( m_hSemConsole );
#else
		Write2Console();
#endif
	}

	int CConsole::Read( char* szBuffer, int nCount )
	{
		GammaLock( m_hLockRead );
		m_szFileName = "";
		m_nStartPos = 0;
		m_szReadBuffer = szBuffer;
		m_nSize = nCount;

#ifndef OUTPUT_ON_MAIN_THREAD 
		GammaPutSemaphore( m_hSemConsole );
		while( m_szFileName )
			GammaSleep( 100 );
#else
		Write2Console();
		while( m_szFileName )
			ReadFromConsole();
#endif
		GammaUnlock( m_hLockRead );
		return m_nSize;
	}

	int CConsole::ReadFile( const char* szFileName, int nStartPos, char* szBuffer, int nSize )
	{
		if( !szFileName || !szFileName[0] )
			return 0;
		GammaLock( m_hLockRead );
		m_szFileName = szFileName;
		m_nStartPos = nStartPos;
		m_szReadBuffer = szBuffer;
		m_nSize = nSize;

#ifndef OUTPUT_ON_MAIN_THREAD 
		GammaPutSemaphore( m_hSemConsole );
		while( m_szFileName )
			GammaSleep( 100 );
#else
		Write2Console();
		while( m_szFileName )
			ReadFileFromConsole();
#endif
		GammaUnlock( m_hLockRead );
		return m_nSize;
	}

#ifndef OUTPUT_ON_MAIN_THREAD 
	void CConsole::ThreadFun( CConsole* pConsole )
	{
		GammaSetThreadName( "CConsole" );
		while( true )
		{
			GammaGetSemaphore( pConsole->m_hSemConsole );
			pConsole->Write2Console();
			pConsole->ReadFromConsole();
			pConsole->ReadFileFromConsole();
		}
	}
#endif

	void CConsole::Write2Console()
	{
		uint32 nLevel;
		SSendBuf SendBuff;
		uint32 nBufferCount = 1;
		if( !m_OutputBuffer.PopBuffer(nLevel, &SendBuff, nBufferCount ) )
			return;

		if (nLevel != m_nLogLevel)
			FlushUserLog(nLevel);

		const char* szLog = (const char*)SendBuff.first;
		uint32 nSize = (uint32)SendBuff.second;
		m_strUserLog.append( szLog, nSize );

		if( nSize && szLog[nSize - 1] == '\n' )
			FlushUserLog(nLevel);

#ifdef _WIN32
		GammaLock( m_hLockConsole );
		if( m_hConsole[eOutput] )
		{
			DWORD dwOut = 0;
			wchar_t* szBuffer = (wchar_t*)alloca( nSize*sizeof(wchar_t) );
			if(m_nLogLevel != nLevel)
			{
				SetConsoleTextAttribute( m_hConsole[eOutput], nLevel == eLL_Error ? 12 : 7 );
				m_nLogLevel = nLevel;
			}

			uint32 nLen = Utf8ToUcs( szBuffer, nSize, szLog, nSize );
			WriteConsoleW( m_hConsole[eOutput], szBuffer, nLen, &dwOut, NULL );
		}
		GammaUnlock( m_hLockConsole );
#else
#ifdef _ANDROID
		if( bError )
			__android_log_write( ANDROID_LOG_INFO, "NDK_LOG", szLog );
		else
			__android_log_write( ANDROID_LOG_ERROR, "NDK_ERR", szLog );
#else
		std::cout << szLog;
#endif
		m_nLogLevel = nLevel;
#endif
		if( m_nRemoteDebugConsole == INVALID_SOCKET )
			return;

		send( m_nRemoteDebugConsole, "\x00", 1, 0 );
		send( m_nRemoteDebugConsole, (const char*)&nSize, 4, 0 );		
		send( m_nRemoteDebugConsole, szLog, nSize, 0 );
	}

	void CConsole::FlushUserLog( uint32 nLevel )
	{
		if(GetGlobLogFun())
			GetGlobLogFun()(m_strUserLog.c_str(), m_strUserLog.size(), nLevel);
		m_strUserLog.clear();
	}

	void CConsole::ReadFromConsole()
	{
		if( !m_szFileName || m_szFileName[0] )
			return;

		Write2Console();
		if( m_nRemoteDebugConsole == INVALID_SOCKET )
		{
			fflush( stdout );
#ifndef _WIN32
			fgets( m_szReadBuffer, m_nSize, stdin );
			m_nSize = (int32)strnlen( m_szReadBuffer, m_nSize );
#else
			char szUtf8[8];
			uint32 nWritePos = 0;
			bool bFinished = false;
			while( !bFinished )
			{
				uint32 nReadPos = 0;
				if( !m_nCurCount )
				{
					HANDLE hInput = m_hConsole[eInput];
					DWORD dwIn = ELEM_COUNT(m_aryReadCount);
					ReadConsoleW( hInput, m_aryReadCount, dwIn, &dwIn, NULL );
					m_nCurCount = (uint32)dwIn;
				}

				while( nReadPos < m_nCurCount && nWritePos < (uint32)( m_nSize - 1 ) )
				{
					wchar_t cRead = m_aryReadCount[nReadPos];
					if( cRead == '\r' )
					{
						nReadPos++;
						continue;
					}

					uint32 nLen = UcsToUtf8( szUtf8, 8, &cRead, 1 );
					if( m_nSize - 1 - nWritePos < nLen || cRead == '\n' )
					{
						if( cRead == '\n' )
						{
							m_szReadBuffer[nWritePos++] = '\n'; 
							nReadPos++;
						}
						m_szReadBuffer[m_nSize = nWritePos] = 0; 
						bFinished = true;
						break;
					}

					memcpy( m_szReadBuffer + nWritePos, szUtf8, nLen );
					nWritePos += nLen;
					nReadPos++;
				}
				m_nCurCount = m_nCurCount - nReadPos;
				memcpy( m_szReadBuffer, m_szReadBuffer + nReadPos, m_nCurCount );
			}
#endif
		}
		else
		{
			send( m_nRemoteDebugConsole, "\x01", 1, 0 );
			send( m_nRemoteDebugConsole, "\x00\x00\x00\x00", 4, 0 );
			int32 i = 0;
			while( i < m_nSize )
			{
				recv( m_nRemoteDebugConsole, &m_szReadBuffer[i], 1, 0 );
				if( m_szReadBuffer[i] == '\n' )
				{
					m_szReadBuffer[i] = 0;
					m_nSize = i;
					break;
				}
				i++;
			}
		}

		m_szFileName = NULL;
	}

	void CConsole::ReadFileFromConsole()
	{
		if( !m_szFileName || !m_szFileName[0] )
			return;

		Write2Console();
		if( m_nRemoteDebugConsole == INVALID_SOCKET )
		{
			CPkgFile File;
			if( !File.Open( m_szFileName ) )
			{
				m_szReadBuffer[0] = 0;
				m_nSize = 1;
			}
			else
			{
				int32 nSendSize = Min<int32>( m_nSize, File.Size() - m_nStartPos );
				memcpy( m_szReadBuffer, File.GetFileBuffer() + m_nStartPos, nSendSize );	
				m_nSize = Max<int32>( nSendSize, 1 );
				m_szReadBuffer[m_nSize - 1] = 0;
			}
		}
		else
		{
			int nLen = (int)strlen( m_szFileName ) + sizeof(m_nStartPos) + sizeof(m_nSize);
			send( m_nRemoteDebugConsole, "\x02", 1, 0 );
			send( m_nRemoteDebugConsole, (const char*)&nLen, 4, 0 );
			send( m_nRemoteDebugConsole, (const char*)&m_nStartPos, sizeof(m_nStartPos), 0 );
			send( m_nRemoteDebugConsole, (const char*)&m_nSize, sizeof(m_nSize), 0 );
			send( m_nRemoteDebugConsole, m_szFileName, nLen - 8, 0 );

			int32 i = 0;
			while( i < m_nSize )
			{
				int nRecv = m_nSize - i;
				int n = (int32)recv( m_nRemoteDebugConsole, &m_szReadBuffer[i], nRecv, 0 );
				if( nRecv == n )
					break;;
				if( m_szReadBuffer[i + n - 1] == 0 )
				{
					m_nSize = i + n - 1;
					break;
				}
				i += n;
			}
		}
		m_szFileName = NULL;
	}

}


