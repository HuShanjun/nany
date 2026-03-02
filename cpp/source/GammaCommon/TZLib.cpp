
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

	GAMMA_COMMON_API uint32 FlushZLibWriter( HZLIB zip, tbyte* pBuffer, uint32 nBufferSize, const void* pSrc, uint32 nSize )
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
		return (uint32)zipStream->total_out;
	}

	GAMMA_COMMON_API uint32 DestroyZLibWriter( HZLIB zip, tbyte* pBuffer, uint32 nBufferSize )
	{
		z_stream* zipStream = (z_stream*)zip;
		zipStream->next_out = pBuffer;
		zipStream->avail_out = nBufferSize;
		zipStream->total_out = 0;
		deflate( zipStream, Z_FINISH );
		deflateEnd( zipStream );
		uint32 nOut = (uint32)zipStream->total_out;
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

	GAMMA_COMMON_API uint32 FlushZLibReader( HZLIB zip, const tbyte* pBuffer, uint32 nBufferSize, void* pTarget, uint32& nSize )
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
		nSize = (uint32)zipStream->total_out;
		return (uint32)zipStream->total_in;
	}

	GAMMA_COMMON_API void DestroyZLibReader( HZLIB zip )
	{
		z_stream* zipStream = (z_stream*)zip;
		inflate( zipStream, Z_FINISH );
		inflateEnd( zipStream );
		delete zipStream;
	}

}
