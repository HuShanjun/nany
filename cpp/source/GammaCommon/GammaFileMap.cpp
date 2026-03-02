
#include "GammaCommon/GammaFileMap.h"
#include "GammaCommon/GammaCodeCvs.h"
#include "GammaCommon/CThread.h"
#include "GammaCommon/CPkgFile.h"
#include "GammaCommon/CPathMgr.h"
#include <stdlib.h>
#include <map>

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace Gamma
{
	struct SFileMapInfo 
	{
#ifdef _WIN32
		HANDLE						m_hFile;
		HANDLE						m_hMap;
#else
		int32						m_nFileDesc;
#endif
		size_t						m_nFileSize;
		size_t						m_nOffset;
		size_t						m_nSize;
		tbyte*						m_pBuffer;
	};

	HFILEMAP GammaMemoryMap( const char* szFile, bool bReadOnly, 
		uint32 nOffset, uint32 nSize, uint32 nTruncateSize )
	{
		SFileMapInfo Info;
		memset( &Info, 0, sizeof(Info) );

		// 文件以可读可写的方式打开
		// nTruncateSize 不指定的话，文件必须存在，否则返回NULL
		// 如果指定nTruncateSize ，文件不存在则创建
#ifdef _WIN32
		wchar_t szPhysicalPath[2048];
		CPathMgr::ToPhysicalPath( Utf8ToUcs( szFile ).c_str(), szPhysicalPath, ELEM_COUNT( szPhysicalPath ) );
		CPathMgr::ShortPath( szPhysicalPath );
		DWORD dwOpenFlag = nTruncateSize ? OPEN_ALWAYS : OPEN_EXISTING;
		Info.m_hFile = ::CreateFileW( szPhysicalPath, GENERIC_READ|GENERIC_WRITE, 
			FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, dwOpenFlag, FILE_ATTRIBUTE_NORMAL, NULL );
		if( INVALID_HANDLE_VALUE == Info.m_hFile )
			return NULL;
#else
		char szPhysicalPath[2048];
		CPathMgr::ToPhysicalPath( szFile, szPhysicalPath, ELEM_COUNT( szPhysicalPath ) );
		CPathMgr::ShortPath( szPhysicalPath );
		int32 flags = nTruncateSize ? ( O_RDWR|O_CREAT ) : O_RDWR;
		Info.m_nFileDesc = open( szPhysicalPath, flags, S_IRWXU|S_IRWXG|S_IRWXO );
		if( Info.m_nFileDesc < 0 )
			return NULL;
#endif

		if( !GammaMemoryRemapMap( &Info, bReadOnly, nOffset, nSize, nTruncateSize ) )
			return NULL;

		return new SFileMapInfo( Info );
	}

	bool GammaMemoryRemapMap( HFILEMAP hMap, bool bReadOnly, uint32 nOffset, uint32 nSize, uint32 nTruncateSize )
	{
		SFileMapInfo& Info = *hMap;

		// nTruncateSize 不指定的话，使用文件大小
		bool bTruncateFile = nTruncateSize != 0;
		if( !bTruncateFile )
		{
#ifdef _WIN32
			nTruncateSize = ::GetFileSize( Info.m_hFile, NULL );
#else
			nTruncateSize = (uint32)lseek( Info.m_nFileDesc, 0, SEEK_END );
#endif
		}

		if( nOffset >= nTruncateSize )
			return false;

		if( Info.m_pBuffer )
		{
#ifdef _WIN32
			::UnmapViewOfFile( Info.m_pBuffer );
			::CloseHandle( Info.m_hMap );
#else
			munmap( Info.m_pBuffer, Info.m_nSize );
#endif
		}

		// nTruncateSize不为0时则截取到指定长度
		if( bTruncateFile )
		{
#ifdef _WIN32
			::SetFilePointer( Info.m_hFile, nTruncateSize, NULL, FILE_BEGIN );
			::SetEndOfFile( Info.m_hFile );
			::SetFilePointer( Info.m_hFile, 0, NULL, FILE_BEGIN );
#else
			ftruncate( Info.m_nFileDesc, nTruncateSize );
#endif
		}

		Info.m_nFileSize = nTruncateSize;
		Info.m_nOffset = nOffset;
		Info.m_nSize = Min<uint32>( nSize, nTruncateSize - nOffset );

#ifdef _WIN32
		DWORD dwProtectec = bReadOnly ? PAGE_READONLY : PAGE_READWRITE;
		DWORD dwMapFlag = bReadOnly ? FILE_MAP_READ : FILE_MAP_WRITE;
		Info.m_hMap = ::CreateFileMapping( Info.m_hFile, 0, dwProtectec, 0, nTruncateSize, NULL );
		Info.m_pBuffer = (tbyte*)::MapViewOfFile( Info.m_hMap, dwMapFlag, 0, nOffset, Info.m_nSize );
#else
		int32 nProtected = bReadOnly ? PROT_READ : ( PROT_READ|PROT_WRITE );
		Info.m_pBuffer = (tbyte*)mmap( NULL, Info.m_nSize, nProtected, MAP_SHARED, Info.m_nFileDesc, Info.m_nOffset );
#endif
		return true;
	}

	void* GammaGetMapMemory( HFILEMAP hMap )
	{
		return hMap->m_pBuffer;
	}

	uint32 GammaGetMapMemorySize( HFILEMAP hMap )
	{
		return (uint32)hMap->m_nSize;
	}

	void GammaMemoryUnMap( HFILEMAP hMap )
	{
		SFileMapInfo& Info = *hMap;
#ifdef _WIN32
		::UnmapViewOfFile( Info.m_pBuffer );
		::CloseHandle( Info.m_hMap );
		::CloseHandle( Info.m_hFile );
#else
		munmap( Info.m_pBuffer, Info.m_nSize );
		close( (int32)Info.m_nFileDesc );
#endif
		SAFE_DELETE( hMap );
	}

	void GammaMemoryMapSyn( HFILEMAP hMap, size_t nOffset, size_t nSize, bool bAsync )
	{
#ifdef _WIN32
#else
		SFileMapInfo& Info = *hMap;
		if( nOffset >= Info.m_nSize )
			return;
		void* pStart = Info.m_pBuffer + nOffset;
		nSize = Min( nSize, Info.m_nSize - nOffset );
		msync( pStart, nSize, bAsync ? MS_ASYNC : MS_SYNC );
#endif
	}
}
