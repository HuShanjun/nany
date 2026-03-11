#pragma once
//====================================================================================
// Log文件接口定义
// Log管理器根据接口提供的Log文件名关键字szFileName来操作log文件
// 完成以下功能：
// 1、Log文件根据时间每天凌晨自动切换文件
// 2、Log文件超过一定大小自动切换文件
// 3、当标识符改变自动切换文件
// 4、Log文件格式如下：文件名关键字_年-月-日-大小自动切换序.标识符.log"
//====================================================================================
#include "GammaCommon/GammaCommonType.h"
#include <stdarg.h>
#include <limits>
#include <map>
#include <string>
#include <iosfwd>
#include <iomanip>

#define LOG_TOTAL_LENGTH	1024
#define LOG_FILE_MAXLENG	1024*1024*10

namespace Gamma
{
	enum ELogLevel
	{
		eLL_Note = 1,
		eLL_Warn = 2,
		eLL_Info = 3,
		eLL_Error = 4,
	};
	enum ELogPathType
	{
		eLPT_Date		= 1 << 0,
		eLPT_Context	= 1 << 1,
		eLPT_Serial		= 1 << 2,
		eLPT_All		= 0xffffffff
	};

	class ILog
	{
	public:
		virtual	uint32_t				Format( const char*, ... ) = 0;
		virtual	uint32_t				Format( const char*, va_list ) = 0;
		virtual	void				Write( const char*, size_t ) = 0;	

		virtual const char*			GetPrefix() const = 0;
		virtual size_t				GetLogSize()const = 0;
		virtual time_t				GetCreateTime() const = 0;								

		virtual	void				Reset() = 0;
		virtual void				Save() = 0;
		virtual void				Release() = 0;
	};

	GAMMA_COMMON_API std::ostream&	operator<< ( std::ostream& ostr, const wchar_t* str );
	inline std::ostream&			operator<< ( std::ostream& ostr, std::wstring str )	{ return ostr << str.c_str();	}
	inline std::ostream&			operator<< ( std::ostream& ostr, wchar_t c )		{ wchar_t s[2] = { c, 0 }; return ostr << s; }

	GAMMA_COMMON_API void			SetLogPath( const char* szLogPath );
	GAMMA_COMMON_API ILog*			GetLogFile( const char* szPrefix, uint32_t nContext, ELogPathType eType );
	GAMMA_COMMON_API bool			WriteLog( const char* szPrefix, uint32_t nContext, ELogPathType eType, const char* szBuffer, size_t nlen );
	GAMMA_COMMON_API void			FlushAllLog();

	typedef void (*GlobalLog)		( const char*, size_t, uint32_t );
	GAMMA_COMMON_API void			SetGlobLogFun( GlobalLog funGlobalLog );
	GAMMA_COMMON_API GlobalLog		GetGlobLogFun();
	GAMMA_COMMON_API std::ostream&	GetLogStream(uint32_t nLevel);

	typedef void (*GlobalErr)       ( const char*, size_t );
	GAMMA_COMMON_API void           SetGlobErrFun( GlobalErr funGlobalErr );
	GAMMA_COMMON_API GlobalErr      GetGlobErrFun();
	GAMMA_COMMON_API std::ostream&  GetErrStream();

	GAMMA_COMMON_API void			EnterStack( const char* szFile, uint32_t nLine, const void* pContext );
	GAMMA_COMMON_API void			PollProcess( const char* szFile, uint32_t nLine );	
	GAMMA_COMMON_API void			LeaveStack();

	GAMMA_COMMON_API void			Redirect2StdConsole( bool bEnable );
	GAMMA_COMMON_API void			Redirect2Remote( const char* szIP, uint16_t nPort );
	GAMMA_COMMON_API void			SetConsoleSize( int32_t x, int32_t y );
	GAMMA_COMMON_API int32_t			ReadFromConsole(char* szBuffer, int32_t nCount);
	GAMMA_COMMON_API int32_t			ReadFileFromConsole(const char* szFileName, int nStartPos, char* szBuffer, int nSize);
	GAMMA_COMMON_API void			SetLogLevel(uint32_t nLevel);

// 新日志接口 nLevel为日志级别枚举ELogLevel
	GAMMA_COMMON_API void			GammaPrint(ELogLevel nLevel, const char* szFormat, ...);

	#define GammaNote				( Gamma::GetLogStream(1) )
	#define GammaWarn				( Gamma::GetLogStream(2) )
	#define GammaInfo				( Gamma::GetLogStream(3) )
	#define GammaError				( Gamma::GetLogStream(4) )

	#define GammaLog				( GammaInfo )
	#define ShowConsole( flag )		Gamma::Redirect2StdConsole( !!flag )
#ifdef _WIN32
	#define GammaErr				( GammaError )
#else
	#define GammaErr				( Gamma::PrintStack( 256, __LINE__, Gamma::GetErrStream() ), Gamma::GetErrStream() )
#endif
}