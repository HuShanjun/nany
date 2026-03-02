//==============================================================
// TQuickCompressZero.h
// 定义行程压缩算法
// 柯达昭
// 2008-12-17
//==============================================================

#ifndef _QUICK_COMPRESS_ZERO_H_
#define _QUICK_COMPRESS_ZERO_H_

#include "GammaCommon/GammaHelp.h"
#include <string>

namespace Gamma
{
	template< class OutFile >
	class TQuickCompressZero
	{
	public:
		typedef		void (OutFile::*OutFun)( const void*, uint32_t );

		template<typename OutFunction>
		TQuickCompressZero( OutFile& fileOut, OutFunction funOut )
			: m_fileOut( &fileOut ), m_funOut( (OutFun)funOut )
			, m_nBuffer(0), m_nPos(0), m_nPreZero(false){}

		int32_t Write( const void* pData, size_t nSize )
		{
			const tbyte* pBuffer = (const tbyte*)pData;
			for( int32_t i = 0; i < (int32_t)nSize; i++ )
			{
				uint32_t nTemp = pBuffer[i];
				if( m_nPreZero )
				{
					nTemp = 0;
					i--;
				}
				m_nPreZero = false;

				if( nTemp )
				{
					m_nBuffer = m_nBuffer|( 0x3 << m_nPos );
					m_nBuffer = m_nBuffer|( nTemp << ( 2 + m_nPos ) );
					m_nPos += 10;
				}
				else
				{
					if( i + 1 >= (int32_t)nSize )
					{
						m_nPreZero = true;
						return (int32_t)nSize;
					}

					if( pBuffer[ i + 1 ] )
					{
						m_nBuffer = m_nBuffer|( 0x1 << m_nPos );
						m_nPos += 2;
					}
					else
					{
						m_nPos += 1;
						i++;
					}

					if( m_nPos < 8 )
						continue;
				}

				int8_t nByte = m_nPos >> 3;
				( m_fileOut->*m_funOut )( (const char*)&m_nBuffer, nByte );
				int8_t nBit = nByte << 3;
				m_nBuffer = m_nBuffer >> nBit;
				m_nPos = (int8_t)( m_nPos - nBit );
			}

			return (int32_t)nSize;
		}

		void Flush()
		{
			if( m_nPreZero )
			{
				m_nBuffer = m_nBuffer|( 0x1 << m_nPos );
				m_nPos += 2;
				m_nPreZero = false;
			}
			else if( !m_nPos )
			{
				return;
			}

			if( m_nPos <= 4 )
			{
				m_nBuffer = m_nBuffer|( 0x1 << m_nPos )|0x80;
				m_nPos = 8;
			}
			else
			{
				m_nBuffer = m_nBuffer|( 0x1 << m_nPos )|0x8000;
				m_nPos = 16;
			}

			( m_fileOut->*m_funOut )( (const char*)&m_nBuffer, m_nPos >> 3 );
			m_nBuffer = m_nPos = 0;
		}

	private:
		OutFile*	m_fileOut;
		OutFun		m_funOut;
		uint32_t		m_nBuffer;
		int8_t		m_nPos;
		bool		m_nPreZero;
	};

	//==============================================================
	// Huffman压缩
	//==============================================================
	template< class InFile>
	class TQuickDecompressZero
	{
	public:
		typedef		int32_t (InFile::*InFun)( const void*, uint32_t );

		template<typename InFunction>
		TQuickDecompressZero( InFile& fileIn, InFunction funIn )
			: m_fileIn( &fileIn ), m_funIn( (InFun)funIn )
			, m_nBuffer(0), m_nBitCount(0), m_bDoubleZero( false ){}

		int32_t Read( void* pData, size_t nSize )
		{
			tbyte* pBuffer = (tbyte*)pData;
			int32_t nCount = 2;
			for( int32_t i = 0; i < (int32_t)nSize; i++ )
			{
				if( m_nBitCount < 10 && nCount == 2 )
				{
					uint16_t nTemp = 0;
					nCount = ( m_fileIn->*m_funIn )( (char*)&nTemp, 2 );
					if( nCount > 0 )
					{
						m_nBuffer = m_nBuffer|( nTemp << m_nBitCount );
						m_nBitCount = (uint8_t)( ( nCount << 3 ) + m_nBitCount );
					}
				}

				if( m_nBitCount == 0 )
					return i;

				if( m_nBuffer&0x1 )
				{
					// 数据不完整
					if( m_nBitCount < 2 )
						return i;

					if( m_nBuffer&0x2 )
					{
						// 数据不完整
						if( m_nBitCount < 10 )
							return i;

						m_nBuffer = m_nBuffer >> 2;
						m_nBitCount -= 10;
						pBuffer[i] = (uint8_t)m_nBuffer;
						m_nBuffer = m_nBuffer >> 8;
					}
					else
					{
						if( ( m_nBuffer&0x4 ) || m_bDoubleZero )
						{
							m_nBuffer = m_nBuffer >> 2;
							m_nBitCount -= 2;
							pBuffer[i] = 0;
						}
						else
						{
							// 数据不完整
							if( m_nBitCount < 3 )
								return i;

							uint32_t nTempBuffer = m_nBuffer >> 2;
							uint8_t nTempBitCount = m_nBitCount - 2;

							while( ( nTempBuffer&0x1 ) == 0 )
							{
								// 数据不完整
								if( nTempBitCount < 2 )
									return i;
								nTempBuffer = nTempBuffer >> 1;
								nTempBitCount--;
							}
							m_nBuffer = nTempBuffer >> 1;
							m_nBitCount = nTempBitCount - 1;
							i--;
						}
					}
					m_bDoubleZero = false;
				}
				else
				{
					pBuffer[i] = 0;
					m_nBuffer = ( m_nBuffer << 1 )|0x1;
					m_nBitCount++;
					m_bDoubleZero = true;
				}
			}

			return (int32_t)nSize;
		}

	private:
		InFile*		m_fileIn;
		InFun		m_funIn;
		uint32_t		m_nBuffer;
		uint8_t		m_nBitCount;
		bool		m_bDoubleZero;
	};
}

#endif