#pragma once
#include "GammaCommon/GammaCommonType.h"
#include "GammaCommon/ILog.h"
#include "GammaCommon/CThread.h"
#include "GammaCommon/TRefString.h"
#include "GammaCommon/TGammaList.h"
#include "CConsole.h"

#include <limits>
#include <map>
#include <string>
#include <iostream>

#define LOG_TOTAL_LENGTH	1024

namespace Gamma
{
	class CLog;

	class CLogManager 
		: public std::map<std::string,CLog*>
		, public CLock
	{
		std::string			m_szLogPath;
		CConsole			m_Console;

	public:
		CLogManager();
		~CLogManager();	

		void				SetLogPath( const char* szLogPath );
		const char*			GetLogPath() const;
		CLog*				GetLog( const char* szPrefix, uint32_t nContext, ELogPathType eType );
		CConsole&			GetConsole();

		static CLogManager&	Instance();
	};

	class CLog: public ILog
	{
		Gamma::HLOCK		m_Lock;
		FILE*				m_fdLog;
		size_t				m_nBytesWrited;
		size_t				m_nMaxBytesWrited;
		std::string			m_szPrefix;
		ELogPathType		m_eType;
		time_t				m_nCreateTime;
		uint32_t				m_nSerialNum;
		uint32_t				m_nContext;
		int32_t				m_nLastDay;

		void				CheckDate();
		void				CreateLogFile();

		CLog( const char* szPrefix, uint32_t nContext, ELogPathType eType, time_t nCreateTime, uint32_t nSerialNum );
		~CLog(void);
	public:
		friend class CLogManager;
		// log file name format : szLogPath/yyyymmdd/szPrefix[yyyymmddformat][serialformat],  
		uint32_t				Format( const char*, va_list );
		uint32_t				Format( const char*, ... );
		void				Write( const char*, size_t );	

		size_t				GetLogSize() const { return m_nBytesWrited; };
		const char*			GetPrefix() const { return m_szPrefix.c_str(); };
		time_t				GetCreateTime() const { return m_nCreateTime; }
		uint32_t				GetSerialNum() const { return m_nSerialNum; }

		void				Reset();
		void				Save();
		void				Release();
	};

	//=======================================================================
	// gammastream
	//=======================================================================
	enum ELogType
	{
		eLogType_Log,
		eLogType_Err,
		eLogType_Count,
	};

	struct log_type
	{
		typedef GlobalLog stream_fun_type;
		enum{ eHandle = eLogType_Log };
	};

	struct err_type
	{
		typedef GlobalErr stream_fun_type;
		enum{ eHandle = eLogType_Err };
	};

	template<class stream_type>
	typename stream_type::stream_fun_type GetGlobalFun()
	{
		return NULL;
	}

	template<>
	log_type::stream_fun_type GetGlobalFun<log_type>()
	{
		return GetGlobLogFun();
	}

	template<>
	err_type::stream_fun_type GetGlobalFun<err_type>()
	{
		return GetGlobErrFun();
	}


	template<class stream_type>
	class gammastream 
		: public std::ostream
		, public std::basic_streambuf<char>
	{
		Gamma::HLOCK	m_Lock;
		std::string		m_szLog;
		uint32_t			m_nPrintLevel;
		uint32_t			m_nConfLevel;

		void _flush()
		{
			uint32_t nLogSize = (uint32_t)m_szLog.size();
			CConsole& console = CLogManager::Instance().GetConsole();
			console.Write( m_szLog.c_str(), nLogSize, m_nPrintLevel);
			m_szLog.clear();
		}
	public:
		typedef std::basic_streambuf<char> Parent_t;
		typedef Parent_t::int_type int_type;
		typedef Parent_t::pos_type pos_type_t;
		typedef Parent_t::off_type off_type_t;

		void SetLogLevels(uint32_t nConf, uint32_t nPrint)
		{
			m_nPrintLevel = nPrint;
			m_nConfLevel = nConf;
		}

		gammastream() : std::ostream( static_cast<std::basic_streambuf<char>*>( this ) )
		{
			m_Lock = GammaCreateLock();
			GammaAst( m_Lock );
		}

		~gammastream()
		{
			GammaDestroyLock( m_Lock );
			m_Lock = NULL;
		}

		std::streamsize xsputn( const char* _Ptr, std::streamsize _Count )
		{
			if( _Count == 0 )
				return 0;
			if (m_nConfLevel > m_nPrintLevel)
				return 0;
			GammaLock( m_Lock );
			m_szLog.append( _Ptr, _Count );
			_flush();
			GammaUnlock( m_Lock );
			return _Count;
		}

		int_type overflow( int_type n )
		{
			GammaLock( m_Lock );
			m_szLog.push_back( (char)n );
			if( (char)n == '\n' )
				_flush();
			GammaUnlock( m_Lock );
			return n;
		}
	};

	typedef gammastream<log_type> logstream;
	typedef gammastream<err_type> errstream;
}
