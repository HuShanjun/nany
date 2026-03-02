//===============================================
// GammaHelp.h 
// 定义常用帮助函数  
// 柯达昭  
// 2007-09-07
//===============================================

#ifndef __GAMMA_HELP_H__
#define __GAMMA_HELP_H__

#include "GammaCommon/GammaCommonType.h"
#include "GammaCommon/GammaPlatform.h"
#include "GammaCommon/GammaHash.h"
#include "GammaCommon/ILog.h"
#include <string>
#include <vector>
#include <utility>
#include <sstream>

#undef SAFE_RELEASE
#undef SAFE_DELETE
#undef SAFE_DEL_GROUP
#undef SUCCEEDED
#undef FAILED
#undef MAKE_DWORD

#define SAFE_RELEASE( p )				{ if( p ){ (p)->Release(); (p) = NULL; } }
#define SAFE_DELETE( p )				{ delete (p); (p) = NULL; }
#define SAFE_DEL_GROUP( p )				{ delete[] (p); (p) = NULL; }
#define SUCCEEDED(hr)					( (long)(hr) >= 0 )
#define FAILED(hr)						( (long)(hr) < 0 )
#define ELEM_COUNT( _array )			( (uint32_t)( sizeof( _array )/sizeof( _array[0] ) ) )

#define CURRENT_CONTEXT					__FILE__, __DATE__, __TIME__, __LINE__, __FUNCTION__
#define GammaThrow( n )					{ GammaErr << ( n ) << std::endl; Gamma::PrintStack( 256, 0, Gamma::GetErrStream() ); throw( n ); }
#define GammaAbort( szErr )				{ Gamma::PrintStack( 256, __LINE__, Gamma::GetErrStream() ); Gamma::GammaException( szErr, CURRENT_CONTEXT, true ); throw; }
#define GammaAbortRetryIgnore( szErr )	{ Gamma::PrintStack( 256, __LINE__, Gamma::GetErrStream() ); Gamma::GammaException( szErr, CURRENT_CONTEXT, false ); }

#define DEADLOOP_BREAK_BEGIN(T) int T = 0;
#define DEADLOOP_BREAK(T, NUM) if(++(T) > (NUM)){Gamma::PrintStack( 256, 0, Gamma::GetErrStream());break;}


#ifdef _DEBUG
#define	GammaAst(exp) do { if( !(exp) ) { Gamma::GetGlobAssertFun()( #exp, __FILE__, __LINE__ ); GammaThrow( "Assertion failure of expresion '" #exp "'" );}} while (0);
#else
#define	GammaAst(exp)
#endif

#if defined PROCESS_DEBUG
struct SStackLog { SStackLog( const char* szFile, uint32_t nLine, const void* pContext ){ EnterStack( szFile, nLine, pContext ); } ~SStackLog(){ LeaveStack(); } };
struct SProcessLog { SProcessLog( const char* szFile, uint32_t nLine ){ PollProcess( szFile, nLine ); } };
#define STATCK_LOG( Context )	SStackLog __StackLog__( __FILE__, __LINE__, Context )
#define PROCESS_LOG	SProcessLog( __FILE__, __LINE__ )
#else
#define STATCK_LOG( Context )
#define PROCESS_LOG
#endif

//Rwfor == ReturnWhenFailedOnRelease
#ifdef _DEBUG
#define	GammaAstReturn(exp, return_value) { if( !(exp) ) { GetGlobAssertFun()( #exp, __FILE__, __LINE__ );GammaThrow( "Assertion failure of expresion '" #exp "'" );} }
#else
#define	GammaAstReturn(exp, return_value) { if( !(exp) ) { GetGlobAssertFun()( #exp, __FILE__, __LINE__ );return return_value; } }
#endif

#ifdef _DEBUG
#define	GammaVer(exp) { if( !(exp) ) GammaThrow( "Verification failure of expresion '" #exp "'" ); }
#else
#define	GammaVer(exp) (exp)
#endif

#ifdef _WIN32
struct _EXCEPTION_POINTERS;
#endif

#define INVALID_64BITID		0xffffffffffffffffULL
#define INVALID_32BITID		0xffffffff
#define INVALID_16BITID		0xffff
#define INVALID_8BITID		0xff

#define MAX_INT64			((int64_t)0x7fffffffffffffffLL)
#define MIN_INT64			((int64_t)0x8000000000000000LL)
#define MAX_INT32			((int32_t)0x7fffffff)
#define MIN_INT32			((int32_t)0x80000000)
#define MAX_INT16			((int16_t)0x7fff)
#define MIN_INT16			((int16_t)0x8000)
#define MAX_INT8			((int8_t)0x7f)
#define MIN_INT8			((int8_t)0x80)

// 由4个字符组成一个DWORD
#define MAKE_DWORD(ch0, ch1, ch2, ch3)                            \
	((uint32_t)(uint8_t)(int8_t)(ch0) | ((uint32_t)(uint8_t)(int8_t)(ch1) << 8) |       \
	((uint32_t)(uint8_t)(int8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(int8_t)(ch3) << 24 ))

#define MAKE_UINT64( ch0, ch1 ) ((uint64_t)(uint32_t)(int32_t)(ch0) | ((uint64_t)(uint32_t)(int32_t)(ch1) << 32) )
#define MAKE_UINT32( ch0, ch1 ) ((uint32_t)(uint16_t)(int16_t)(ch0) | ((uint32_t)(uint16_t)(int16_t)(ch1) << 16) )
#define MAKE_UINT16( ch0, ch1 ) ((uint16_t)(uint8_t)(int8_t)(ch0) | ((uint16_t)(uint8_t)(int8_t)(ch1) << 8) )

#define HIUINT32(l)	((uint32_t)((uint64_t)((int64_t)(l)) >> 32))
#define LOUINT32(l)	((uint32_t)((uint64_t)((int64_t)(l)) & 0xffffffff))
#define HIUINT16(l)	((uint16_t)((uint32_t)((int32_t)(l)) >> 16))
#define LOUINT16(l)	((uint16_t)((uint32_t)((int32_t)(l)) & 0xffff))
#define HIUINT8(w)	((uint8_t)((uint16_t)((int16_t)(w)) >> 8))
#define LOUINT8(w)	((uint8_t)((uint16_t)((int16_t)(w)) & 0xff))	
#define HIINT32(l)	((int32_t)((uint64_t)((int64_t)(l)) >> 32))
#define LOINT32(l)	((int32_t)((uint64_t)((int64_t)(l)) & 0xffffffff))
#define HIINT16(l)	((int16_t)((uint32_t)((int32_t)(l)) >> 16))
#define LOINT16(l)	((int16_t)((uint32_t)((int32_t)(l)) & 0xffff))
#define HIINT8(w)	((int8_t)((uint16_t)((int16_t)(w)) >> 8))
#define LOINT8(w)	((int8_t)((uint16_t)((int16_t)(w)) & 0xff))

#define HOUR_OF_DAY	24
#define SECONDS_OF_MINUTE	60
#define MINUTE_OF_HOUR		60
#define SECONDS_OF_HOUR		(MINUTE_OF_HOUR*SECONDS_OF_MINUTE)
#define MINUTE_OF_DAY		(MINUTE_OF_HOUR*24)
#define SECONDS_OF_DAY		(MINUTE_OF_DAY*SECONDS_OF_MINUTE)
#define MINUTE_OF_WEEK		(MINUTE_OF_DAY*7)
#define SECONDS_OF_WEEK		(MINUTE_OF_WEEK*SECONDS_OF_MINUTE)

#define ERASE_IF( exp, container, i ) { if( exp ){ container.erase( i++ ); } else { ++i; } }

namespace Gamma
{
	typedef std::pair<const void*, size_t> SSendBuf;

	typedef void (*GlobalAssertFun)		( const char* , const char*, uint32_t );
	GAMMA_COMMON_API void				SetGlobAssertFun( GlobalAssertFun funGlobalLog );
	GAMMA_COMMON_API GlobalAssertFun	GetGlobAssertFun();


	GAMMA_COMMON_API void GammaException( const char* szMsg, const char* szFile, const char* szDate, 
		const char* szTime, uint32_t nLine, const char* szFunction, bool bForceAbort = false );

	//========================================================================
	// 产生dump文件
	//========================================================================
	GAMMA_COMMON_API void DebugGenMiniDump( void* pContext, const char* szFileName, bool bFullMemDump );

	//========================================================================
	// 代码地址到标识符的转换
	//========================================================================
	GAMMA_COMMON_API void DebugAddress2Symbol( void* pAddress, char* szSymbolBuf, uint32_t nSize );

	//========================================================================
	// 得到堆栈从nBegin层到nEnd层的堆栈，保存在pStack，
	// pStack 返回nBegin层到nEnd层的堆栈，由函数外部分配空间
	// pStack[0]为nBegin层堆栈， pStack[nEnd-nBegin]为nEnd层堆栈
	//========================================================================
	GAMMA_COMMON_API size_t GetStack( void** pStack, uint16_t nBegin, uint16_t nEnd );

	//========================================================================
	// 打印从nBegin层到nEnd层的堆栈
	//========================================================================
	GAMMA_COMMON_API size_t PrintStack( uint16_t nMax, int32_t nLine, std::ostream& Stream );

	//========================================================================
	// 打印指定堆栈
	//========================================================================
	GAMMA_COMMON_API size_t PrintStack( void** pStack, size_t nSize, int32_t nLine, std::ostream& Stream );

	//========================================================================
	// 指定堆栈输出到指定文件 
	//========================================================================
	GAMMA_COMMON_API size_t PrintStack2File( void** pStack, size_t nSize, FILE* pFile );

	//========================================================================
	// CPU是否支持SSE
	//========================================================================
	enum ECpuSuport{ eMMX = 0x00800000, eSSE = 0x02000000, eSSE2 = 0x04000000, e3DNOW = 0x80000000 };
	GAMMA_COMMON_API uint32_t GetCpuSuport();
	
	//========================================================================
	// 整数向上对其
	//========================================================================
	inline uint32_t AligenUp( uint32_t n, uint32_t nAligen )
	{
		return n ? ( ( n - 1 )/nAligen + 1 )*nAligen : 0;
	}

	//========================================================================
	// 整数向上对其
	//========================================================================
	inline uint32_t AligenDown( uint32_t n, uint32_t nAligen )
	{
		return ( n / nAligen )*nAligen;
	}

	//========================================================================
	// 整数向上对其
	//========================================================================
	inline uint32_t AligenUpTo2Power( uint32_t n )
	{
		if( n == 0 )
			return 1;
		uint32_t p = 1;
		for( n = n - 1; n; n = n >> 1 )
			p = p << 1;
		return p;
	}

	//========================================================================
	// 整数向上对其
	//========================================================================
	inline uint32_t AligenDownTo2Power( uint32_t n )
	{
		if( n == 0 )
			return 1;
		uint32_t p = 1;
		for( ; n; n = n >> 1 )
			p = p << 1;
		return p >> 1;
	}

	//========================================================================
	// 限制数的上下限 
	// 返回 a>=min && a<= max
	//=========================================================================
	template<class T>
	inline T Limit( const T& a, const T& min, const T& max )
	{
		if( a < min )
			return min;
		else if( a > max )
			return max;

		return a;
	}

	template<class T>
	inline T Min( const T& a, const T& b )
	{
		return a < b ? a : b;
	} 

	template<class T>
	inline T Max( const T& a, const T& b )
	{
		return a > b ? a : b;
	} 

	template<class T>
	inline T GetIntervalIntersect( const T& min1, const T& max1, const T& min2, const T& max2 )
	{
		return Min( max1, max2 ) - Max( min1, min2 );
	}

	template<class T>
	inline T Abs( const T& a )
	{
		return a < 0 ? -a : a;
	}

	// 常量求最小值 
	template<size_t...n>
	struct TMin {};

	template<size_t a>
	class TMin<a>
	{
	public:
		enum { eValue = a };
	};

	template<size_t a, size_t...n>
	class TMin<a, n...>
	{
	public:
		enum { eValue = a < TMin<n...>::eValue ? a : TMin<n...>::eValue };
	};
 
	// 常量求最大值
	template<size_t...n>
	struct TMax {};

	template<size_t a>
	class TMax<a>
	{
	public:
		enum { eValue = a };
	};
  
	template<size_t a, size_t...n>
	class TMax<a, n...>
	{
	public:
		enum { eValue = a > TMax<n...>::eValue ? a : TMax<n...>::eValue };
	};

	//========================================================================
	// 求常数的2的幂对齐
	//========================================================================
	template<uint32_t n>
	class TAligenUpTo2Power
	{
		template<uint32_t v,uint32_t l>	struct _A{ enum{ eValue = _A< v*2, l/2 >::eValue }; };
		template<uint32_t v>			struct _A<v, 0>{ enum{ eValue = v }; };
	public:
		enum { eValue = _A< 1, n - 1 >::eValue };
	};
	template<> class TAligenUpTo2Power<0> { public: enum { eValue = 0 }; };

	//========================================================================
	// 求常数的向上对齐
	//========================================================================
	template<uint32_t n, uint32_t nAligen>
	class TAligenUp
	{
	public:
		enum { eValue = n ? ( ( n - 1 )/nAligen + 1 )*nAligen : 0 };
	};

	//========================================================================
	// 求常数的向下对齐
	//========================================================================
	template<uint32_t n, uint32_t nAligen>
	class TAligenDown
	{
	public:
		enum { eValue = ( n / nAligen )*nAligen };
	};

	//========================================================================
	// 求常数的的对数
	//========================================================================
	template<uint32_t n>
	class TLog2
	{
	public:
		enum { eValue = TLog2< n/2 >::eValue + 1 };
	};
	template<> class TLog2<1> { public: enum { eValue = 0 }; };

	//========================================================================
	// 判断是否字母
	//========================================================================
	template<class CharType>
	inline bool IsLetter( CharType c )
	{
		return ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' );
	}

	//========================================================================
	// 判断是否文字
	//========================================================================
	template<class CharType>
	inline bool IsWordChar( CharType c )
	{
		return IsLetter( c ) || ( (uint32_t)c ) > 127;
	}

	//========================================================================
	// 判断是否数字
	//========================================================================
	template<class CharType>
	inline bool IsNumber( CharType c )
	{
		return c >= '0' && c <= '9';
	}

	//========================================================================
	// 判断是否10进制数字
	//========================================================================
	inline bool IsNumber(const char* str)
	{
		if (!str)
			return false;
		while (str && !IsNumber(*str++))
			return false;
		return true;
	}

	//========================================================================
	// 16进制数字转为数值
	//========================================================================
	template<class CharType>
	inline int ValueFromHexNumber( CharType c )
	{
		if( c >= '0' && c <= '9' )
			return c - '0';
		if( c >= 'A' && c <= 'F' )
			return c - 'A' + 10;
		if( c >= 'a' && c <= 'f' )
			return c - 'a' + 10;
		return -1;
	}

	//========================================================================
	// 数值转为16进制数字
	//========================================================================
	inline int ValueToHexNumber( int n )
	{
		GammaAst( n >= 0 && n <= 0xf );
		if( n < 10 )
			return '0' + n;
		return n - 10 + 'A';
	}

	//========================================================================
	// 判断是否16进制数字
	//========================================================================
	template<class CharType>
	inline bool IsHexNumber( CharType c )
	{
		return ValueFromHexNumber( c ) >= 0;
	}

	//========================================================================
	// 判断是空格
	//========================================================================
	template<class CharType>
	inline bool IsBlank( CharType c )
	{
		return c == ' ' || c == '\t' || c == '\r' || c == '\n';
	}

	//========================================================================
	// 从文件读入字符串
	//========================================================================
	template< class File, class Fun >
	inline void ReadString( File& fileRead, Fun funRead, std::basic_string<char>& sRead )
	{
		uint32_t uStringLen;
		( fileRead.*funRead )( (char*)&uStringLen, sizeof( uint32_t ) );
		if( !uStringLen )
			return sRead.clear();
		sRead.resize( uStringLen );
		( fileRead.*funRead )( (char*)&sRead[0], uStringLen*sizeof(char) );
	}

	//========================================================================
	template< class File, class Fun >
	inline void ReadString( File& fileRead, Fun funRead, std::basic_string<wchar_t>& sRead )
	{
		uint32_t uStringLen;
		( fileRead.*funRead )( (char*)&uStringLen, sizeof( uint32_t ) );
		if( !uStringLen )
			return sRead.clear();
		sRead.resize( uStringLen );
		for( uint32_t i = 0; i < uStringLen; i++ )
			( fileRead.*funRead )( (char*)&sRead[i], sizeof(uint16_t) );
	}

	//========================================================================
	// 将字符串写入文件
	//========================================================================
	template< class File, class Fun >
	inline void WriteString( File& fileWrite, Fun funWrite, const std::basic_string<char>& sWrite )
	{
		uint32_t uStringLen = (uint32_t)sWrite.size();
		( fileWrite.*funWrite )( (const char*)&uStringLen, sizeof( uint32_t ) );
		if( !uStringLen )
			return;
		( fileWrite.*funWrite )( (const char*)sWrite.c_str(), uStringLen*sizeof(char) );
	}

	template< class File, class Fun>
	inline void WriteString( File& fileWrite, Fun funWrite, const std::basic_string<wchar_t>& sWrite )
	{
		uint32_t uStringLen = (uint32_t)sWrite.size();
		( fileWrite.*funWrite )( (const char*)&uStringLen, sizeof( uint32_t ) );
		if( !uStringLen )
			return;
		for( uint32_t i = 0; i < uStringLen; i++ )
			( fileWrite.*funWrite )( (const char*)&sWrite[i], sizeof(uint16_t) );
	}

	//========================================================================
	// 将字符串进行异或
	//========================================================================
	inline void BufferXor( tbyte* pDes, uint32_t nSize, const uint8_t(&nKey)[16], 
		uint32_t nOffset = 0, const tbyte* pSrc = NULL )
	{
		if( !pSrc )
			pSrc = pDes;
		for( uint32_t i = 0; i < nSize; i++, nOffset++ )
			pDes[i] = pSrc[i] ^ nKey[ nOffset & 0xF ];
	}

	//========================================================================
	// 将字符串进行异或
	//========================================================================
	template<typename Elem>
	inline bool IsAllBeTheElem( const Elem* pBuffer, size_t nCount, const Elem& e )
	{
		for( size_t i = 0; i < nCount; i++ )
			if( pBuffer[i] != e )
				return false;
		return true;
	}

	//========================================================================
	// 将字符串进行"Gamma"化.
	// Gamma字符串是只有正斜杠'/'的字符串
	//========================================================================
	template<class CharType>
	inline CharType GammaChar( CharType c )
	{
		return c == '\\' ? '/' : c;
	}

	template<class CharType>
	inline CharType* GammaString( CharType* p )
	{
		for( int i = 0; p[i]; i++ ) 
			p[i] = GammaChar( p[i] );
		return p;
	}

	template<class CharType>
	inline int32_t GammaStringCompare( const CharType* p1, const CharType* p2, uint32_t nLen = INVALID_32BITID )
	{
		for( uint32_t i = 0; i < nLen; i++ )
		{
			CharType c1 = GammaChar( p1[i] );
			CharType c2 = GammaChar( p2[i] );
			if( c1 == 0 && c2 == 0 )
				return 0;
			if( c1 == 0 )
				return -1;
			if( c2 == 0 )
				return 1;
			if( c1 != c2 )
				return c1 - c2;
		}
		return 0;
	}

	template<class CharType>
	inline const std::basic_string<CharType>& GammaString( std::basic_string<CharType>& str )
	{
		GammaString( &str[0] );
		return str;
	}

	template<class CharType>
	inline std::basic_string<CharType> GammaString( const CharType* p )
	{
		std::basic_string<CharType> Out;
		for( int i = 0; p[i]; i++ ) 
			Out.push_back( GammaChar( p[i] ) );
		return Out;
	}

	template<class CharType>
	inline int32_t CompareGammaString( const CharType* p1, const CharType* p2 )
	{
		if( p1 == p2 )
			return 0;
		if( p1 && !p2 )
			return 1;
		if( !p1 && p2 )
			return -1;

		CharType c1 = GammaChar( *p1 );
		CharType c2 = GammaChar( *p2 );
		while( c1 == c2 && c1 != 0 ) 
		{
			c1 = GammaChar( *++p1 );
			c2 = GammaChar( *++p2 );
		}
		return ( c1 == c2 ) ? 0 : ( c1 > c2 ? 1 : -1 );
	}

	//========================================================================
	// 将GammaString 转化为数字ID
	//========================================================================
	template<class CharType>
	inline uint32_t GammaString2ID( const CharType* szStr )
	{
		CharType szBuf[1024];
		size_t i = 0;
		for( ; i < 1023 && szStr[i]; i++ )
			szBuf[i] = szStr[i];
		szBuf[i] = 0;
		return (uint32_t)GammaHash( GammaString( szBuf ), i*sizeof(CharType) );
	} 

	//========================================================================
	// 命令解释
	//========================================================================
	template<class CharType>
	std::vector< std::basic_string<CharType> > CmdParser( const CharType* szCmdLine )
	{
		std::vector< std::basic_string<CharType> > vecCmd;
		while( *szCmdLine )
		{
			while( *szCmdLine && IsBlank( *szCmdLine ) )
				szCmdLine++;

			bool bQuotation = *szCmdLine == '\"';
			if( bQuotation )
				szCmdLine++;

			if( !*szCmdLine )
				break;

			const wchar_t* szStr = szCmdLine;
			while( *szCmdLine && !( ( !bQuotation && IsBlank( *szCmdLine ) ) || *szCmdLine == '\"' ) )
				szCmdLine++;

			vecCmd.push_back( std::basic_string<CharType>( szStr, szCmdLine - szStr ) );
			if( bQuotation && *szCmdLine == '\"' )
				szCmdLine ++;
		}

		return vecCmd;
	}

	//========================================================================
	// 从路径中提取文件名
	//========================================================================
	template<class CharType>
	CharType* GetFileNameFromPath( CharType* szPath )
	{
		size_t nPos = 0;
		for( size_t i = 0; szPath[i]; i++ )
			if( szPath[i] == '/' || szPath[i] == '\\' )
				nPos = i + 1;
		return szPath + nPos;
	}

	//========================================================================
	// 从路径中提取扩展名
	//========================================================================
	template<class CharType>
	const CharType* GetFileExtend( const CharType* szPath )
	{
		uint32_t nPos = INVALID_32BITID;
		for( uint32_t i = 0; szPath[i]; i++ )
			if( szPath[i] == '.' )
				nPos = i + 1;
		return nPos == INVALID_32BITID ? NULL : szPath + nPos;
	}

	//========================================================================
	// 根据分隔符将字符串拆成字符串组
	//========================================================================

	template< class _Elem >
	inline std::vector< std::basic_string<_Elem> > SeparateString( const _Elem* szSrc, _Elem nSeparator )
	{
		std::vector< std::basic_string<_Elem> > vecStr;

		size_t nSize = 1;
		for( int32_t i = 0; szSrc[i]; i++ )
			if( szSrc[i] == nSeparator )
				nSize++;

		vecStr.resize( nSize );
		int32_t nPreItem = 0;
		for( int32_t i = 0, n = 0; ; i++ )
		{
			if( szSrc[i] == nSeparator )
			{
				vecStr[n++].assign( szSrc + nPreItem, i - nPreItem );
				nPreItem = i + 1;
			}
			else if( szSrc[i] == 0 )
			{
				vecStr[n++].assign( szSrc + nPreItem, i - nPreItem );
				break;
			}
		}
		return vecStr;
	}
	GAMMA_COMMON_API int32_t GammaA2I(const wchar_t* szStr);
	GAMMA_COMMON_API int32_t GammaA2I(const char* szStr);	
	GAMMA_COMMON_API int64_t GammaA2I64( const wchar_t* szStr );
	GAMMA_COMMON_API int64_t GammaA2I64( const char* szStr );
	GAMMA_COMMON_API double GammaA2F( const wchar_t* szStr );
	GAMMA_COMMON_API double GammaA2F( const char* szStr );
    template< class _CharType, class _intType >
    inline std::vector<_intType> SeparateStringToIntArray( const _CharType* szSrc, _CharType nSeparator )
    {        
        std::vector< _intType > results;
		_CharType szBuffer[64];		
		uint32_t n = 0;
		bool bDot = false;
        while( *szSrc )
        {
			if( *szSrc != nSeparator )
			{
				if( n < ELEM_COUNT( szBuffer ) - 1 )
					szBuffer[n++] = *szSrc;
				bDot = bDot || *szSrc == '.';
			}
			else
			{
				szBuffer[n] = 0;       
				if( bDot )
					results.push_back( (_intType)GammaA2F( szBuffer ) );
				else
					results.push_back( (_intType)GammaA2I64( szBuffer ) );
				bDot = false;
				n = 0;
			} 
			szSrc++;
		}
		szBuffer[n] = 0;       
		if( bDot )
			results.push_back( (_intType)GammaA2F( szBuffer ) );
		else
			results.push_back( (_intType)GammaA2I64( szBuffer ) );
        return results;
	}

	template< class _CharType, class _SepType, class _IntType >
	size_t SeparateStringFast( _CharType* szSrc, _SepType nSeparator,
		std::pair<_CharType*, _IntType>* vecResult, size_t nMaxSize )
	{        
		size_t nCount = 0;
		int32_t nPreItem = 0;
		for( int32_t i = 0; ; i++ )
		{
			if( nCount >= nMaxSize )
				return nCount;
			if( szSrc[i] == nSeparator )
			{
				vecResult[nCount].first = szSrc + nPreItem;
				vecResult[nCount].second = (_IntType)( i - nPreItem );
				nCount++;
				nPreItem = i + 1;
			}
			else if( szSrc[i] == 0 )
			{
				vecResult[nCount].first = szSrc + nPreItem;
				vecResult[nCount].second = (_IntType)( i - nPreItem );
				nCount++;
				break;
			}
		}
		return nCount;
	}

	//========================================================================
	// 将字符串组根据分隔符合成字符串
	//========================================================================
	template<class VecStr, class _CharType>
	inline std::basic_string<_CharType> CombinationString( const VecStr& vecStr, _CharType nSeparator )
	{
		std::basic_string<_CharType> strTemp;
		for( typename VecStr::const_iterator it = vecStr.begin(); it != vecStr.end(); it++ )
		{
			if( it != vecStr.begin() )
				strTemp += nSeparator;
			strTemp += *it;
		}
		return strTemp;
	}

	/**************************************
	*函数名称: GetStrItem
	*函数功能: 取字符串中被分隔符分隔的项目,
	例如在字符串"12|345|6789" 中取偏移为1的项,结果为"345" (以'|'为分隔符)
	*输入参数: src   - 源串
	ch    - 分隔符
	index - 项目的偏移值(从0开始)
	*输出参数: dest  - 目标串(应保证缓冲区足够大)
	*返 回 值: NULL  - 未找到指定偏移值的项
	非NULL - 目标串
	***************************************/
	inline char* GetStrItem(char* dest, const char* src, char ch, unsigned int index)
	{
		unsigned int i;
		int len;
		const char* p, * q;

		for (i = 0, p = src; i < index; i++, p++)
		{
			if ((p = strchr(p, ch)) == NULL)
				return NULL;
		}

		q = strchr(p, ch);

		len = (int)((q == NULL) ? strlen(p) : q - p);
		memset(dest, 0, len + 1);
		memcpy(dest, p, len);

		return dest;
	}

    /** 去除字符串首尾的引号
    */
    template< class CharType >
    inline void TrimRoundQuotes( std::basic_string<CharType>& s )
    {
        if ( s.empty() )
            return;

        if ( s[0] == (CharType)'\"' )
            s.erase(0, 1 );
        
        if ( !s.empty() )
        {
            size_t nLastIndex = s.length() - 1;
            if ( s[ nLastIndex ] == (CharType)'\"' )
                s.erase( nLastIndex, 1 );
        }
    }

	inline size_t UrlEncode( const char* szUtf8, size_t nSize, char* szOut, size_t nMaxSize )
	{
		if ( !szUtf8 )
			return 0;

		size_t nEncodeLen = 0;
		for ( size_t i = 0; i < nSize && szUtf8[i]; i++ )
		{
			unsigned char c = szUtf8[i];
			if ( IsNumber( c ) || IsLetter( c ) )
			{
				if( nEncodeLen >= nMaxSize )
					return nEncodeLen;
				szOut[nEncodeLen++] = c;
			}
			else
			{
				if( nEncodeLen + 2 >= nMaxSize )
					return nEncodeLen;
				const char* szX = "0123456789ABCDEF";
				szOut[nEncodeLen++] = '%';
				szOut[nEncodeLen++] = szX[ c >> 4 ];
				szOut[nEncodeLen++] = szX[ c & 0x0f ];
			}
		}

		return nEncodeLen;
	}

	//========================================================================
	// 检查标志是否存在
	//========================================================================
	inline bool IsHasFlag( uint32_t nStyle, uint32_t nFlag )
	{
		return (nStyle&nFlag) != 0; 
	}

	//========================================================================
	// 设定标志
	//========================================================================
	inline uint32_t SetFlag( uint32_t nStyle, uint32_t nFlag, int32_t bValue )
	{
		if( bValue )
			return nStyle|nFlag;
		return nStyle&(~nFlag);
	}

	//=========================================================================
	// 以\n为分隔符，从流中获取一个字符串
	//=========================================================================
	template<class stream_t, class _Elem>
	inline void GetString( stream_t& istream, std::basic_string<_Elem>& str )
	{
		_Elem n = (_Elem)istream.get();
		while( n == '\n' && istream )
			n = istream.get();
		while( n != '\n' && istream )
		{
			str.push_back( n );
			n = istream.get();
		}
	}

	//==========================================================================
	// 将数组求和
	//==========================================================================
	template<typename T1, typename T2>
	inline T2 Sum( const T1* pArray, size_t nSize )
	{
		T2 n = 0;
		for( size_t i = 0; i < nSize; i++ )
			n += pArray[i];
		return n;
	}

	template<class _Type>
	_Type tolower( _Type c ){ return c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c; }
	template<class _Type>
	_Type toupper( _Type c ){ return c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c; }

	template<class _Type>
	int32_t stricmp( const _Type* src, const _Type* dst )
	{
		int ret = 0 ;

		while( 0 == ( ret = tolower(*src) - tolower(*dst) ) && *dst )
			++src, ++dst;

		if ( ret < 0 )
			ret = -1 ;
		else if ( ret > 0 )
			ret = 1 ;

		return ret;
	}

	template<class _Type>
	int32_t strnicmp( const _Type* src, const _Type* dst, size_t nMaxLen )
	{
		int ret = 0;
		size_t n = 0;
		while( n < nMaxLen && 0 == ( ret = tolower(*src) - tolower(*dst) ) && *dst )
			++src, ++dst, ++n;

		if ( ret < 0 )
			ret = -1 ;
		else if ( ret > 0 )
			ret = 1 ;

		return ret;
	}

	template<class _Type>
	void strlwr( _Type* src )
	{
		while( *src )
			*src = (_Type)tolower(*src),++src;
	}

	template<typename _Type>
	uint32_t strcpy_safe( _Type* pDes, const _Type* pSrc, uint32_t nSize, uint32_t nMaxSrcLen )
	{		
		GammaAst( nSize );
		if( !pSrc )
		{
			pDes[0] = 0;
			return 0;
		}

		uint32_t i = 0;
		--nSize;
		while( i < nSize && i < nMaxSrcLen && *pSrc )
			pDes[i++] = *pSrc++;
		pDes[i] = 0;
		return i;
	}

	template<typename _Type>
	uint32_t strcat_safe( _Type* pDes, const _Type* pSrc, uint32_t nSize, uint32_t nMaxSrcLen )
	{		
		if( !pSrc )
			return 0;
		GammaAst( nSize );
		uint32_t i = 0;
		--nSize;
		while( i < nSize && pDes[i] )
			i++;

		uint32_t j = 0;
		while( i < nSize && j < nMaxSrcLen && pSrc[j] )
			pDes[i++] = pSrc[j++];
		pDes[i] = 0;
		return i;
	}


	template<typename _Type>
	int32_t strcmp_safe( const _Type* pDes, const _Type* pSrc, uint32_t nSize, uint32_t nMaxSrcLen )
	{ 
		if( !pSrc && !pDes )
			return 0;
		if( !pDes )
			return -1;
		if( !pSrc )
			return 1;
		uint32_t nCount = Min( nSize, nMaxSrcLen );
		for( uint32_t i = 0; i < nCount; ++i )
		{
			if( pDes[i] != pSrc[i] )
				return (int32_t)pDes[i] - (int32_t)pSrc[i];
			if( !pDes[i] )
				return 0;
		}

		int32_t nDesEnd = nCount == nSize ? 0 : pDes[nCount];
		int32_t nSrcEnd = nCount == nMaxSrcLen ? 0 : pSrc[nCount];
		return nDesEnd - nSrcEnd;
	}

	template<typename _Type, uint32_t n> 
	uint32_t strcpy2array_safe( _Type(&array_pointer)[n], const _Type* pSrc )
	{ 
		return strcpy_safe( array_pointer, pSrc, n, INVALID_32BITID );
	}

	template<typename _Type, uint32_t n> 
	uint32_t strcat2array_safe( _Type(&array_pointer)[n], const _Type* pSrc )
	{ 
		return strcat_safe( array_pointer, pSrc, n, INVALID_32BITID );
	}

	template<typename _Type, uint32_t n> 
	int32_t strcmp2array_safe( const _Type(&array_pointer)[n], const _Type* pSrc )
	{ 
		return strcmp_safe( array_pointer, pSrc, n, INVALID_32BITID );
	}

	template<typename _Type, uint32_t n> 
	uint32_t strncpy2array_safe( _Type(&array_pointer)[n], const _Type* pSrc, uint32_t nMaxSrcLen )
	{ 
		return strcpy_safe( array_pointer, pSrc, n, nMaxSrcLen );
	}

	template<typename _Type, uint32_t n> 
	uint32_t strncat2array_safe( _Type(&array_pointer)[n], const _Type* pSrc, uint32_t nMaxSrcLen )
	{ 
		return strcat_safe( array_pointer, pSrc, n, nMaxSrcLen );
	}

	template<typename _Type, uint32_t n> 
	int32_t strncmp2array_safe( const _Type(&array_pointer)[n], const _Type* pSrc, uint32_t nMaxSrcLen )
	{ 
		return strcmp_safe( array_pointer, pSrc, n, nMaxSrcLen );
	}
}
#endif
