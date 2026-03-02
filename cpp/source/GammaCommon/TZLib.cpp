
#include "GammaCommon/TZLib.h"
#include "GammaCommon/GammaMemory.h"
#include "zlib.h"
#include <string.h>

namespace Gamma
{
	static voidpf ZLibAlloc( voidpf /*opaque*/, unsigned items, unsigned size )
	{
		return GammaAlloc( items*size );
	}

	static void ZLibFree( voidpf /*opaque*/, voidpf ptr )
	{
		GammaFree( ptr );
	}

	GAMMA_COMMON_API HZLIB CreateZLibWriter()
	{
		z_stream* zipStream = new z_stream;
		memset( zipStream, 0, sizeof(z_stream) );
		zipStream->zalloc = &ZLibAlloc;
		zipStream->zfree = &ZLibFree;
		deflateInit( zipStream, 9 );
		return zipStream;
	}

	GAMMA_COMMON_API uint32_t FlushZLibWriter( HZLIB zip, tbyte* pBuffer, uint32_t nBufferSize, const void* pSrc, uint32_t nSize )
	{
		z_stream* zipStream = (z_stream*)zip;
		if( pSrc && nSize )
		{
			zipStream->next_in = (tbyte*)pSrc;
			zipStream->avail_in = nSize;
		}
		zipStream->next_out = pBuffer;
		zipStream->avail_out = nBufferSize;
		zipStream->total_out = 0;
		deflate( zipStream, Z_SYNC_FLUSH );
		return (uint32_t)zipStream->total_out;
	}

	GAMMA_COMMON_API uint32_t DestroyZLibWriter( HZLIB zip, tbyte* pBuffer, uint32_t nBufferSize )
	{
		z_stream* zipStream = (z_stream*)zip;
		zipStream->next_out = pBuffer;
		zipStream->avail_out = nBufferSize;
		zipStream->total_out = 0;
		deflate( zipStream, Z_FINISH );
		deflateEnd( zipStream );
		uint32_t nOut = (uint32_t)zipStream->total_out;
		delete zipStream;
		return nOut;
	}

	GAMMA_COMMON_API HZLIB CreateZLibReader()
	{
		z_stream* zipStream = new z_stream;
		memset( zipStream, 0, sizeof(z_stream) );
		zipStream->zalloc = &ZLibAlloc;
		zipStream->zfree = &ZLibFree;
		inflateInit( zipStream );
		return zipStream;
	}

	GAMMA_COMMON_API uint32_t FlushZLibReader( HZLIB zip, const tbyte* pBuffer, uint32_t nBufferSize, void* pTarget, uint32_t& nSize )
	{
		z_stream* zipStream = (z_stream*)zip;
		if( pTarget )
		{
			zipStream->next_out = (tbyte*)pTarget;
			zipStream->avail_out = nSize;
			zipStream->total_out = 0;
		}
		zipStream->next_in = (tbyte*)pBuffer;
		zipStream->avail_in = nBufferSize;
		zipStream->total_in = 0;
		inflate( zipStream, Z_SYNC_FLUSH );
		nSize = (uint32_t)zipStream->total_out;
		return (uint32_t)zipStream->total_in;
	}

	GAMMA_COMMON_API void DestroyZLibReader( HZLIB zip )
	{
		z_stream* zipStream = (z_stream*)zip;
		inflate( zipStream, Z_FINISH );
		inflateEnd( zipStream );
		delete zipStream;
	}

}
