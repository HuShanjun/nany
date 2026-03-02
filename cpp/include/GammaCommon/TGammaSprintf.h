//===============================================
// TGammaSprintf.h 
// TGammaSprintf，替代swprintf以及sprintf
// 柯达昭
// 2013-11-21
//===============================================

#ifndef __GAMMA_SPRINTF_H__
#define __GAMMA_SPRINTF_H__
#include "GammaCommon/TGammaStrStream.h"

namespace Gamma
{
	template<class _Elem>
	class TGammaSprintf
	{
	public:

		static const _Elem* OutputBeforeFormat( _Elem* szBuf, 
			uint32 nMaxSize, uint32& nCount, const _Elem* szFormat )
		{
			uint32 i = 0;
			while( szFormat[i] )
			{
				if( nCount + 1 >= nMaxSize )
				{
					szBuf[nCount] = 0;
					return NULL;
				}

				if( szFormat[i] != '%' )
					szBuf[nCount++] = szFormat[i++];
				else if( szFormat[i+1] == '%' )
					szBuf[nCount++] = szFormat[++i];
				else
					break;
			}

			if( !szFormat[i] )
			{
				szBuf[nCount] = 0;
				return NULL;
			}
			return szFormat + i;
		}

		template<typename Type>
		static inline const _Elem* OutputFormat( _Elem* szBuf, 
			uint32 nMaxSize, uint32& nCount, const _Elem* szFormat, Type value )
		{
			if( nCount + 1 >= nMaxSize )
			{
				szBuf[nCount] = 0;
				return NULL;
			}

			TGammaStrStream<_Elem> ss( szBuf + nCount, 65536 - nCount );
			ss << value;
			if( nCount + (uint32)ss.length() >= nMaxSize )
			{
				szBuf[nCount] = 0;
				return NULL;
			}

			nCount += (uint32)ss.length();
			while( !IsLetter( *szFormat ) )
				++szFormat;
			return szFormat + 1;
		}

		static inline const _Elem* OutputFormat( _Elem* szBuf, 
			uint32 nMaxSize, uint32& nCount, const _Elem* szFormat, double fValue )
		{
			if( nCount + 1 >= nMaxSize )
			{
				szBuf[nCount] = 0;
				return NULL;
			}

			uint32 i = 0;
			char szFormatUtf8[32] = "%";
			for( ; szFormat[i] != 'f'; i++ )
				szFormatUtf8[i] = (char)szFormat[i];
			szFormatUtf8[i++] = (char)szFormat[i++];
			szFormatUtf8[i] = 0;

			char szOut[32];
			uint32 nLen = (uint32)sprintf( szOut, szFormatUtf8, fValue );
			if( nCount + (uint32)nLen >= nMaxSize )
			{
				szBuf[nCount] = 0;
				return NULL;
			}

			for( uint32 n = 0; n < nLen; n++ )
				szBuf[nCount++] = szOut[n];
			return szFormat + i;
		}

		static inline const _Elem* OutputFormat( _Elem* szBuf, 
			uint32 nMaxSize, uint32& nCount, const _Elem* szFormat, float fValue )
		{
			return OutputFormat( szBuf, nMaxSize, nCount, szFormat, (double)fValue );
		}

		template<typename MsgType0, typename MsgType1, typename MsgType2, typename MsgType3 >
		static uint32 Format( _Elem* szOutBuffer, uint32 nSize, const _Elem* szFormat, 
			MsgType0 msgParam0, MsgType1 msgParam1, MsgType2 msgParam2, MsgType3 msgParam3 )
		{
			uint32 nCount = 0;
			szFormat = OutputBeforeFormat( szOutBuffer, nSize, nCount, szFormat );
			if( !szFormat )
				return nCount;
			szFormat = OutputFormat( szOutBuffer, nSize, nCount, szFormat, msgParam0 );
			if( !szFormat )
				return nCount;

			szFormat = OutputBeforeFormat( szOutBuffer, nSize, nCount, szFormat );
			if( !szFormat )
				return nCount;
			szFormat = OutputFormat( szOutBuffer, nSize, nCount, szFormat, msgParam1 );
			if( !szFormat )
				return nCount;

			szFormat = OutputBeforeFormat( szOutBuffer, nSize, nCount, szFormat );
			if( !szFormat )
				return nCount;
			szFormat = OutputFormat( szOutBuffer, nSize, nCount, szFormat, msgParam2 );
			if( !szFormat )
				return nCount;

			szFormat = OutputBeforeFormat( szOutBuffer, nSize, nCount, szFormat );
			if( !szFormat )
				return nCount;
			szFormat = OutputFormat( szOutBuffer, nSize, nCount, szFormat, msgParam3 );
			if( !szFormat )
				return nCount;

			szOutBuffer[nCount] = 0;
			return nCount;
		}
	};
}

#endif
