
#include <cstdarg>
#include <ctime>
#include <algorithm>
#include "CLog.h"
#include "GammaCommon/CThread.h"
#include "GammaCommon/CPathMgr.h"
#include "GammaCommon/GammaCodeCvs.h"
#include "Win32.h"

#define  _LOGTEST

uint32_t g_nLogLevel = 0;

namespace Gamma
{
	//================================================================================
	// log管理器
	//================================================================================
	CLogManager::CLogManager()
	{
	}

	CLogManager::~CLogManager()
	{
		while( !empty() )
			begin()->second->Release();
		clear();
	}

	CLogManager& CLogManager::Instance()
	{
		static CLogManager s_LogManager;
		return s_LogManager;
	}

	void CLogManager::SetLogPath( const char* szLogPath )
	{
		m_szLogPath = szLogPath;
	}

	const char* CLogManager::GetLogPath() const
	{
		return m_szLogPath.c_str();
	}

	CLog* CLogManager::GetLog( const char* szPrefix, uint32_t nContext, ELogPathType eType )
	{		
		map<std::string,CLog*>::iterator iter = find( szPrefix );

		time_t nCurTime;
		time( &nCurTime );

		if ( iter == end() )
		{
			CLog* pLog = new CLog( szPrefix, nContext, eType, nCurTime, 0 );
			iter = insert( std::pair<std::string,CLog*>( szPrefix, pLog ) ).first;
		}

		return iter->second;
	}

	CConsole& CLogManager::GetConsole()
	{
		return m_Console;
	}

	void SetLogPath( const char* szLogPath )
	{
		CLogManager::Instance().SetLogPath( szLogPath );
	}
	
	void Redirect2StdConsole( bool bEnable )
	{
		CLogManager::Instance().GetConsole().Redirect2StdConsole( bEnable );
	}

	void Redirect2Remote( const char* szIP, uint16_t nPort )
	{
		CLogManager::Instance().GetConsole().Redirect2Remote( szIP, nPort );
	}
	
	int32_t ReadFromConsole( char* szBuffer, int32_t nCount )
	{
		return CLogManager::Instance().GetConsole().Read( szBuffer, nCount );
	}

	int32_t ReadFileFromConsole( const char* szFileName, int nStartPos, char* szBuffer, int nSize )
	{
		return CLogManager::Instance().GetConsole().ReadFile( szFileName, nStartPos, szBuffer, nSize );
	}


	GAMMA_COMMON_API void SetLogLevel(uint32_t nLevel)
	{
		g_nLogLevel = nLevel;
	}

	void GammaPrint(ELogLevel nLevel, const char* szFormat, ...)
	{
		/*if (g_pApp->GetSettingInt("App Setting", "LogLevel") > nLevel)
			return;

		char szbuff[LOG_TOTAL_LENGTH + 100] = "";
		memset(szbuff, 0, sizeof(szbuff));

		char* pbegin = szbuff;
		int32_t remainlen = sizeof(szbuff);
		int32_t writesize = 0;
		switch (nLevel)
		{
		case eLL_Note: writesize = snprintf(pbegin, remainlen, "%s", "[note]"); break;
		case eLL_Warn: writesize = snprintf(pbegin, remainlen, "%s", "[warn]"); break;
		case eLL_Info: writesize = snprintf(pbegin, remainlen, "%s", "[info]"); break;
		case eLL_Error: writesize = snprintf(pbegin, remainlen, "%s", "[error]"); break;
		default: writesize = snprintf(pbegin, remainlen, "%s", "[info]"); break;
		}
		pbegin += writesize;
		remainlen -= writesize;

		va_list vlArgs;
		va_start(vlArgs, szFormat);
		writesize = _vsnprintf(pbegin, remainlen, szFormat, vlArgs);
		va_end(vlArgs);
		if (nLevel == eLL_Error)
			GetErrStream() << szbuff;
		else
			GetLogStream() << szbuff;*/
	}

	void SetConsoleSize( int32_t x, int32_t y )
	{
		CLogManager::Instance().GetConsole().Resize( x, y );
	}

#if( defined PROCESS_DEBUG )
	struct SStackLogInfo
	{
		const char* szFile;
		uint32_t		nLine;
		const void* pContext;
		const void* pStackPoint;
	} g_aryStackLogBuffer[8192];
	uint32_t g_nProcessIndex = 0;

	void EnterStack( const char* szFile, uint32_t nLine, const void* pContext )
	{
		if( g_nProcessIndex >= ELEM_COUNT( g_aryStackLogBuffer ) )
			return;
		uint32_t nDummy = 0;
		g_aryStackLogBuffer[g_nProcessIndex].szFile = szFile;
		g_aryStackLogBuffer[g_nProcessIndex].nLine = nLine;
		g_aryStackLogBuffer[g_nProcessIndex].pContext = pContext;
		g_aryStackLogBuffer[g_nProcessIndex].pStackPoint = &nDummy;
		++g_nProcessIndex;
	}

	void PollProcess( const char* szFile, uint32_t nLine )
	{
		g_aryStackLogBuffer[g_nProcessIndex].szFile = szFile;
		g_aryStackLogBuffer[g_nProcessIndex].nLine = nLine;
		g_aryStackLogBuffer[g_nProcessIndex].pContext = NULL;
		g_aryStackLogBuffer[g_nProcessIndex].pStackPoint = NULL;
		++g_nProcessIndex;
	}

	void LeaveStack()
	{
		while( g_nProcessIndex && g_aryStackLogBuffer[g_nProcessIndex - 1].pStackPoint == NULL )
			--g_nProcessIndex;
		if( g_nProcessIndex && g_aryStackLogBuffer[g_nProcessIndex - 1].pStackPoint )
			--g_nProcessIndex;
	}
#else
	void EnterStack( const char* szFile, uint32_t nLine, const void* pContext ){}
	void PollProcess( const char* szFile, uint32_t nLine ){}
	void LeaveStack(){}
#endif

	#define DIR_FORMAT			"%4d%02d/"
	#define DATE_FORMAT			"_%4d%02d%02d"
	#define SERIAL_FORMAT		"_%d"
	#define CONTEXT_FORMAT		"_%d"

	//================================================================================
	// log
	//================================================================================
	CLog::CLog( const char* szPrefix, uint32_t nContext, ELogPathType eType, time_t nCreateTime, uint32_t nSerialNum )
		: m_nLastDay(0)
		, m_nCreateTime( nCreateTime )
		, m_szPrefix( szPrefix )
		, m_eType( eType )
		, m_nContext( nContext )
		, m_nSerialNum( nSerialNum )
	{
		m_Lock = GammaCreateLock();
		GammaAst( m_Lock );
		CreateLogFile();
	}

	CLog::~CLog(void)
	{
		fclose( m_fdLog );
		GammaDestroyLock( m_Lock );
		m_Lock = NULL;
	}

	void CLog::CreateLogFile()
	{
#ifdef _WIN32
		tm tmCurTime = *::localtime( &m_nCreateTime );
#else
		tm tmCurTime;
		::localtime_r( &m_nCreateTime, &tmCurTime );
#endif

		char szDir[256];
		sprintf( szDir, DIR_FORMAT, tmCurTime.tm_year + 1900, tmCurTime.tm_mon + 1 );

		std::string szFileName = CLogManager::Instance().GetLogPath();
		szFileName += szDir;
		CPathMgr::MakeDirectory( Utf8ToUcs( szFileName.c_str() ).c_str() );

		szFileName += m_szPrefix;

		if( m_eType&eLPT_Date )
		{
			char szBuf[256];
			sprintf( szBuf, DATE_FORMAT, tmCurTime.tm_year + 1900, tmCurTime.tm_mon + 1, tmCurTime.tm_mday );
			szFileName += szBuf;
		}

		if( m_eType&eLPT_Serial )
		{
			char szBuf[256];
			sprintf( szBuf, SERIAL_FORMAT, m_nSerialNum );
			szFileName += szBuf;
		}

		if( m_eType&eLPT_Context )
		{
			char szBuf[256];
			sprintf( szBuf, CONTEXT_FORMAT, m_nContext );
			szFileName += szBuf;
		}

		szFileName += ".log";
		char szPhysicalPath[2048];
		CPathMgr::ToPhysicalPath( szFileName.c_str(), szPhysicalPath, ELEM_COUNT( szPhysicalPath ) );

#ifndef _WIN32
		m_fdLog = fopen( szPhysicalPath, "r+b" );
#elif(defined _UNICODE)
		m_fdLog = _wfopen( Utf8ToUcs( szPhysicalPath ).c_str(), L"r+b" );
#else
		m_fdLog = fopen( UcsToAnsi( Utf8ToUcs( szPhysicalPath ).c_str() ).c_str(), "r+b" );
#endif

		if( m_fdLog == NULL )
		{
#ifndef _WIN32
			m_fdLog = fopen( szPhysicalPath, "w+b" );
#elif(defined _UNICODE)
			m_fdLog = _wfopen( Utf8ToUcs( szPhysicalPath ).c_str(), L"w+b" );
#else
			m_fdLog = fopen( UcsToAnsi( Utf8ToUcs( szPhysicalPath ).c_str() ).c_str(), "w+b" );
#endif
		}

		GammaAst( m_fdLog );
		fseek( m_fdLog, 0, SEEK_END );
		m_nBytesWrited = (uint32_t)ftell( m_fdLog );
		m_nMaxBytesWrited = m_nBytesWrited;
	}

	void CLog::CheckDate()
	{
		time_t nCurTime;
		time( &nCurTime );

#ifdef _WIN32
		tm tmCurTime = *::localtime( &nCurTime );
#else
		tm tmCurTime;
		::localtime_r( &nCurTime, &tmCurTime );
#endif

		time_t nCreateTime	= (time_t)GetCreateTime();
#ifdef _WIN32
		tm tmCreateTime = *::localtime( &nCreateTime );
#else
		tm tmCreateTime;
		::localtime_r( &nCreateTime, &tmCreateTime );
#endif

		//* 不同一天的新开文件
		if( tmCurTime.tm_mday != tmCreateTime.tm_mday || GetLogSize() > LOG_FILE_MAXLENG )
		{
			// 即刻保存Log
			Save();
			fclose( m_fdLog );
			m_nSerialNum++;
			m_nCreateTime = nCurTime;
			CreateLogFile();
		}
	}

	void CLog::Write( const char* szBuffer, size_t stLen )
	{
		if( stLen == 0 )
			return;
		GammaLock( m_Lock );		
		CheckDate();
		m_nBytesWrited += stLen;
		m_nMaxBytesWrited = Max( m_nBytesWrited, m_nMaxBytesWrited );
		GammaUnlock( m_Lock );
		fwrite( szBuffer, 1, stLen, m_fdLog );
		fflush( m_fdLog );
	}

	uint32_t CLog::Format( const char* szFormat, ... )
	{
		va_list vlArgs;
		va_start(vlArgs,szFormat);
		uint32_t uResult = Format( szFormat, vlArgs );
		va_end(vlArgs);
		return uResult;
	}

	uint32_t CLog::Format( const char* szFormat, va_list vlArgs )
	{
		char szBuffer[LOG_TOTAL_LENGTH + 100];
		memset( szBuffer, 0, sizeof( szBuffer ) );

		char* pWritePos = szBuffer;
		int32_t nWritelen = 0;

		//vsnprintf在windows和linux下行为不一致，当缓冲区不够长时，linux把最后一个字符填写为'\0',windows不会
		//两者返回的长度在任何情况下都不包含最后的'\0'
		nWritelen = _vsnprintf( pWritePos, LOG_TOTAL_LENGTH, szFormat, vlArgs );
		pWritePos += nWritelen;
		Write( szBuffer, pWritePos - szBuffer );

		return (int32_t)( pWritePos - szBuffer );
	}

	void CLog::Reset()
	{
		char szBuffer[1024];
		memset( szBuffer, ' ', sizeof(szBuffer) );
		fseek( m_fdLog, 0, SEEK_SET );
		for( uint32_t i = 0; i < m_nBytesWrited; i += sizeof(szBuffer) )
			fwrite( szBuffer, 1, Min<size_t>( m_nBytesWrited - i, sizeof(szBuffer) ), m_fdLog );
		fseek( m_fdLog, 0, SEEK_SET );
		m_nBytesWrited = 0;
	}

	void CLog::Save()
	{
		fflush( m_fdLog );
	}

	void CLog::Release()
	{
		CLogManager::Instance().Lock();
		for( std::map< std::string, CLog* >::iterator iter = CLogManager::Instance().begin();
			iter != CLogManager::Instance().end(); iter++ )
		{
			if( iter->second == this )
			{
				CLogManager::Instance().erase( iter );
				delete this;
				break;
			}
		}
		CLogManager::Instance().Unlock();
	}

	ILog* GetLogFile( const char* szPrefix, uint32_t nContext, ELogPathType eType )
	{
		CLogManager::Instance().Lock();
		CLog* pLog = CLogManager::Instance().GetLog( szPrefix, nContext, eType );
		CLogManager::Instance().Unlock();
		return pLog;
	}

	bool WriteLog( const char* szPrefix, uint32_t nContext, ELogPathType eType, const char* szBuffer, size_t nlen )
	{
		ILog* pLog = GetLogFile( szPrefix, nContext, eType );
		if( pLog )
		{
			pLog->Write(szBuffer, nlen);
			return true;
		}
		return false;
	}

	void FlushAllLog()
	{
		CLogManager::Instance().Lock();
		for( std::map<std::string,CLog*>::iterator it = CLogManager::Instance().begin();
			it != CLogManager::Instance().end(); it++ )
			it->second->Save();
		CLogManager::Instance().Unlock();
	}
	//////////////////////////////////////////////////////////////////////////

	GlobalLog& _GetGlobLogFun()
	{
		static GlobalLog s_GlobalLog = NULL;
		return s_GlobalLog;
	}

	void SetGlobLogFun( GlobalLog funGlobalLog )
	{
		_GetGlobLogFun() = funGlobalLog;
	}

	GlobalLog GetGlobLogFun()
	{
		return _GetGlobLogFun();
	}

	std::ostream& GetLogStream(uint32_t nLevel)
	{
		static logstream s_inst;
		s_inst.SetLogLevels(g_nLogLevel, nLevel);
		return s_inst;
	}

	GlobalErr& _GetGlobErrFun()
	{
		static GlobalErr s_GlobalErr = NULL;
		return s_GlobalErr;
	}

	void SetGlobErrFun( GlobalErr funGlobalErr )
	{
		_GetGlobErrFun() = funGlobalErr;
	}

	GlobalErr GetGlobErrFun()
	{
		return _GetGlobErrFun();
	}

	std::ostream& GetErrStream()
	{
		static errstream s_inst;
		return s_inst;
	}

	std::ostream& operator<< ( std::ostream& ostr, const wchar_t* str )
	{
		std::string strTemp = UcsToUtf8( str );
		return ostr << strTemp.c_str();
	}
}
