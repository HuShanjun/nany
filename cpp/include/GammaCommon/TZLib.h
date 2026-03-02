//==============================================================
// CZLibFile.h
// 使用zlib压缩算法的文件类
// 柯达昭
// 2015-10-08
//==============================================================
#include "GammaCommon/GammaCommonType.h"
#include <string.h>

namespace Gamma
{
	#define DEFAULT_READ_TYPE int32 (FileType::*)( void*, uint32 )
	#define DEFAULT_WRITE_TYPE void (FileType::*)( const void*, uint32 )
	typedef void* HZLIB;

	GAMMA_COMMON_API HZLIB CreateZLibWriter();
	GAMMA_COMMON_API uint32 FlushZLibWriter( HZLIB zip, tbyte* pBuffer, uint32 nBufferSize, const void* pSrc, uint32 nSize );
	GAMMA_COMMON_API uint32 DestroyZLibWriter( HZLIB zip, tbyte* pBuffer, uint32 nBufferSize );

	GAMMA_COMMON_API HZLIB CreateZLibReader();
	GAMMA_COMMON_API uint32 FlushZLibReader( HZLIB zip, const tbyte* pBuffer, uint32 nBufferSize, void* pTarget, uint32& nSize );
	GAMMA_COMMON_API void DestroyZLibReader( HZLIB zip );

	template<typename FileType, typename FunWirter = DEFAULT_WRITE_TYPE, uint32 nBufferSize = 1024 >
	class TZLibWriter
	{
		HZLIB		m_hZip;
		FileType&	m_File;
		FunWirter	m_funWirte;

		TZLibWriter& operator= ( const TZLibWriter& );

	public:
		TZLibWriter( FileType& OutFile, FunWirter funWrite )
			: m_File( OutFile )
			, m_funWirte( funWrite )
			, m_hZip( CreateZLibWriter() )
		{
		}

		~TZLibWriter()
		{
			Flush();
		}

		void Write( const void* pBuf, uint32 nLen )
		{ 
			if( !m_hZip )
				return;
			tbyte nBuffer[nBufferSize];
			uint32 nCompress = FlushZLibWriter( m_hZip, nBuffer, nBufferSize, pBuf, nLen );
			while( nCompress == nBufferSize )
			{
				( m_File.*m_funWirte )( nBuffer, nCompress );
				nCompress = FlushZLibWriter( m_hZip, nBuffer, nBufferSize, NULL, 0 );
			}
			( m_File.*m_funWirte )( nBuffer, nCompress );
		}

		void Flush()
		{
			if( !m_hZip )
				return;
			tbyte nBuffer[nBufferSize];
			uint32 nCompress = DestroyZLibWriter( m_hZip, nBuffer, nBufferSize );
			( m_File.*m_funWirte )( nBuffer, nCompress );
			m_hZip = NULL;
		}

		template<typename DataType>
		TZLibWriter& operator << ( const DataType& data )
		{
			Write( &data, sizeof(data) );
			return *this;
		}
	};

	template<typename FileType, typename FunReader = DEFAULT_READ_TYPE, uint32 nBufferSize = 1024 >
	class TZLibReader
	{
		HZLIB		m_hZip;
		FileType&	m_File;
		FunReader	m_funRead;

		tbyte		m_aryData[nBufferSize];
		int32		m_nDataSize;

		tbyte		m_aryCache[nBufferSize];
		uint32		m_nCacheSize;
		uint32		m_nCacheReadPos;

		TZLibReader& operator= ( const TZLibReader& );

		void ReadCache()
		{
			m_nCacheReadPos = 0;
			m_nCacheSize = nBufferSize;
			uint32 nUsedData = FlushZLibReader( m_hZip, m_aryData, m_nDataSize, m_aryCache, m_nCacheSize );
			while( (int32)nUsedData == m_nDataSize )
			{
				m_nDataSize = ( m_File.*m_funRead )( m_aryData, nBufferSize );
				if( m_nDataSize < 0 )
				{
					m_nDataSize = 0;
					return;
				}
				nUsedData = FlushZLibReader( m_hZip, m_aryData, m_nDataSize, NULL, m_nCacheSize );
			}

			m_nDataSize -= nUsedData;
			memmove( m_aryData, m_aryData + nUsedData, m_nDataSize );
		}

	public:
		TZLibReader( FileType& InFile, FunReader funRead )
			: m_File( InFile )
			, m_funRead( funRead )
			, m_hZip( CreateZLibReader() )
			, m_nDataSize(0)
			, m_nCacheSize(nBufferSize)
			, m_nCacheReadPos(nBufferSize)
		{
		}

		~TZLibReader()
		{
			Flush();
		}

		uint32 Read( void* pBuf, uint32 nLen )
		{ 
			if( !m_hZip )
				return 0;

			uint32 nTotalRead = 0;
			while( m_nCacheSize - m_nCacheReadPos < nLen )
			{
				uint32 nReadSize = m_nCacheSize - m_nCacheReadPos;
				memcpy( pBuf, m_aryCache + m_nCacheReadPos, nReadSize );
				pBuf = ( (char*)pBuf ) + nReadSize;
				nLen -= nReadSize;
				nTotalRead += nReadSize;
				m_nCacheReadPos += nReadSize;
				if( nReadSize == 0 && m_nCacheSize != nBufferSize )
					return nTotalRead;
				ReadCache();
			}

			memcpy( pBuf, m_aryCache + m_nCacheReadPos, nLen );
			m_nCacheReadPos += nLen;
			return nLen + nTotalRead;
		}

		void Flush()
		{
			if( !m_hZip )
				return;
			DestroyZLibReader( m_hZip );
			m_hZip = NULL;
		}

		template<typename DataType>
		DataType PopData()		
		{ 
			DataType data;
			Read( &data, sizeof(data) );
			return data;
		}

		template<typename DataType>
		TZLibReader& operator >> ( DataType& data )
		{
			Read( &data, sizeof(data) );
			return *this;
		}
	};
}
