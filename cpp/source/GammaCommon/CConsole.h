#pragma once
#include "GammaCommon/GammaCommonType.h"
#include "GammaCommon/TCircleBuffer.h"
#include "GammaCommon/ILog.h"
#include <fcntl.h>
#include <stdio.h>

//#define OUTPUT_ON_MAIN_THREAD

namespace Gamma
{
	class CConsole
	{
	public:
		enum { eInput, eOutput, eError, eCount };

		CConsole();
		~CConsole(void);

		void			Write( const char* szOut, uint32 nSize, uint32 nLevel );
		int				Read( char* szBuffer, int nCount );
		int				ReadFile( const char* szFileName, int nStartPos, char* szBuffer, int nSize );

		void			Redirect2Remote( const char* szIP, uint16 nPort );
		void			Redirect2StdConsole( bool bEnable );
		void			Resize( int32 x, int32 y );

	private:
		enum { eMaxConsoleSize = 4096 };

#ifndef OUTPUT_ON_MAIN_THREAD 
		HTHREAD			m_hThread;
		HSEMAPHORE		m_hSemConsole;
		static void		ThreadFun( CConsole* pConsole );
#endif

#ifdef _WIN32
		HLOCK			m_hLockConsole;
		HANDLE			m_hConsole[eCount];
		FILE*			m_fdAllocNew[eCount];
		wchar_t			m_aryReadCount[256];
		uint32			m_nCurCount;
		bool			m_bAllocConsole;
#endif

		uint32			m_nLogLevel;
		std::string		m_strUserLog;
		CCircleBuffer	m_OutputBuffer;

		HLOCK			m_hLockRead;
		const char*		m_szFileName;
		int32			m_nStartPos;
		char*			m_szReadBuffer;
		int32			m_nSize;

		int32			m_nRemoteDebugConsole;

		void			ReadFileFromConsole();
		void			ReadFromConsole();
		void			Write2Console();
		void			FlushUserLog(uint32 nLevel);

	};
}
