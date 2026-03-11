//==============================================================
// TBitSet.h 
// 定义BitSet操作
// 柯达昭
// 2007-09-10
//==============================================================

#ifndef _GAMMA_BITSET_H_
#define _GAMMA_BITSET_H_

#include "GammaCommon/GammaHelp.h"

namespace Gamma
{
	template< int32_t nSize, class IntType = uint32_t, bool bLittleEndian = true >
	class TBitSet
	{
	public:
		enum { nBufSize = ( nSize - 1 )/8 + 1 };

		uint8_t _Buf[ nBufSize ];

		void Reset()
		{
			memset( _Buf, 0, sizeof( _Buf ) );
		}

		// param1: 要获取位值的位置
		// param2: 要获得多少位
		// return: 返回值为获得多少位的数值
		uint32_t GetBit( uint32_t pos, uint32_t Num = 1 ) const
		{
			if( bLittleEndian )
			{
				uint32_t nBytePos = pos >> 3;
				uint8_t nBitOffset = uint8_t( pos&0x7 );
				GammaAst( pos + Num <= nSize );

				IntType n = _Buf[nBytePos++];
				for( size_t i = 8 - nBitOffset, j = 8; i < Num && nBytePos < nBufSize; i += 8, j += 8 )
					n |= _Buf[nBytePos++] << j;
				return ( n >> nBitOffset )&( ( 1 << Num ) - 1 );
			}
			else
			{
				uint32_t n = 0;
				for( uint32_t i = 0; i < Num; i++, pos++ )
				{
					if( _Buf[ pos >> 3 ]&( 0x80 >> ( pos&0x07 ) ) )
						n = ( n << 1 )|0x1;
					else
						n = n << 1;
				}
				return (uint32_t)n;
			}
		}

		// param1: 要设置位值的位置
		// param2: 要设置的数据
		// param3: 要设置多少位
		void SetBit( uint32_t pos, uint32_t v, uint32_t Num = 1 )
		{
			if( bLittleEndian )
			{
				uint32_t nBytePos = pos >> 3;
				int32_t nBitOffset = pos&0x7;
				GammaAst( pos + Num <= nSize );

				IntType vv = IntType( v ) << nBitOffset;
				IntType m = IntType( ( ( IntType( 1 ) << Num ) - 1 ) << ( pos&0x07 ) );
				for( int32_t i = -nBitOffset, j = 0; i < (int32_t)Num && nBytePos < nBufSize; 
					i += 8, j += 8, m = m >> 8, vv = vv >> 8, nBytePos++ )
					_Buf[nBytePos] = (uint8_t)( ( _Buf[nBytePos]&(~m ) )|( vv&m ) );
			}
			else
			{
				for( int32_t i = Num - 1; i >= 0; i--, pos++ )
				{
					uint8_t n = (uint8_t)( 0x80 >> ( pos&0x07 ) );
					if( v&( 1 << i ) )
						_Buf[ pos >> 3 ] |= n;
					else
						_Buf[ pos >> 3 ] &= ~n;
				}
			}
		}
	};
}
#endif
