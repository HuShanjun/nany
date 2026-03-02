/*
*	PathHelp.h	文件路径管理器的实现
*/

#ifndef _GAMMA_PATH_HELP_H_
#define _GAMMA_PATH_HELP_H_


#include "GammaCommon/GammaCommonType.h"
#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/GammaCodeCvs.h"
#include "GammaCommon/CPathMgr.h"

#include <errno.h>
#include <string>
#include <sstream>
#include <algorithm>

using namespace std;

#ifndef _WIN32
#if ( defined _IOS || defined _MAC )
#include <sys/param.h>
#include <sys/mount.h>
#else
#include <sys/statfs.h>
#endif
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#else
#include <direct.h>
#endif

namespace Gamma
{
	//===================================================================
	// 平台相关函数
	//===================================================================
	struct FILE_FOUND
	{
#ifdef _WIN32
		HANDLE			pFound;
		WIN32_FIND_DATAW FindData;
#else       
		DIR*			pFound;
		dirent*			pDrt;
#endif
	};

	inline FILE_FOUND*	GammaFindFirstFile( const wchar_t* szDir )
	{
		FILE_FOUND* pFound = new FILE_FOUND;
#ifdef _WIN32
		if( ( pFound->pFound = FindFirstFileW( ( wstring( szDir ) + L"\\*.*" ).c_str(), &pFound->FindData ) ) == INVALID_HANDLE_VALUE )
			SAFE_DELETE( pFound );
#else       
		if( !( pFound->pFound = opendir( UcsToUtf8(szDir).c_str() ) ) 
			|| !( pFound->pDrt = readdir( pFound->pFound ) ) )
			SAFE_DELETE( pFound );
#endif
		return pFound;
	}

	inline FILE_FOUND*	GammaFindFirstFile( const char* szDir )
	{
		FILE_FOUND* pFound = new FILE_FOUND;
#ifdef _WIN32
		if( ( pFound->pFound = FindFirstFileW( Utf8ToUcs( string( szDir ) + "\\*.*" ).c_str(), &pFound->FindData ) ) == INVALID_HANDLE_VALUE )
			SAFE_DELETE( pFound );
#else
		pFound->pFound = opendir( szDir );
		if( !pFound->pFound || !( pFound->pDrt = readdir( pFound->pFound ) ) )
			SAFE_DELETE( pFound );
#endif
		return pFound;
	}

	inline bool GammaFindNextFile( FILE_FOUND* pFound )
	{
#ifdef _WIN32
		if( !FindNextFileW( pFound->pFound, &pFound->FindData ) )
			return false;
#else       
		if( !( pFound->pDrt = readdir( pFound->pFound ) ) )
			return false;
#endif
		return true;
	}

	inline uint32_t GammaGetFileName( FILE_FOUND* pFound, wchar_t szFileName[], uint32_t nSize )
	{
#ifdef _WIN32
		return strcpy_safe( szFileName, pFound->FindData.cFileName, nSize, INVALID_32BITID );
#else    
		return Utf8ToUcs( szFileName, nSize, pFound->pDrt->d_name );
#endif
	}

	inline uint32_t GammaGetFileName( FILE_FOUND* pFound, char szFileName[], uint32_t nSize )
	{
#ifdef _WIN32
		return UcsToUtf8( szFileName, nSize, pFound->FindData.cFileName );
#else
		return strcpy_safe( szFileName, pFound->pDrt->d_name, nSize, INVALID_32BITID );
#endif
	}

	inline void GammaFindClose( FILE_FOUND* pFound )
	{
		if( !pFound )
			return;
#ifdef _WIN32
		FindClose( pFound->pFound );
#else       
		closedir( pFound->pFound );
#endif
		delete pFound;
	}

	inline bool GammaIsDirectory( FILE_FOUND* pFound )
	{
#ifdef _WIN32
		return ( pFound->FindData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY )
			== FILE_ATTRIBUTE_DIRECTORY;
#else
		return pFound->pDrt->d_type == DT_DIR;
#endif
	}

	inline void GammaDeleteDirectory( const wchar_t* szFileName )
	{
#ifdef _WIN32
		_wrmdir( szFileName );
#else
		char szDirUtf8[2048];
		UcsToUtf8( szDirUtf8, 2000, szFileName );
		rmdir( szDirUtf8 );
#endif
	}

	inline void GammaDeleteDirectory( const char* szFileName )
	{
#ifdef _WIN32
		wchar_t szDirUcs2[2048];
		Utf8ToUcs( szDirUcs2, 2000, szFileName );
		_wrmdir( szDirUcs2 );
#else
		rmdir( szFileName );
#endif
	}

	// 	这个C++函数，GammaShortPath，看起来是一个路径标准化函数。它接受一个字符数组szPath作为输入，并在原地修改它以标准化路径。
	// 该函数处理以下情况：
	// 删除冗余的当前目录引用（.）
	// 解析父目录引用（..）
	// 将所有目录分隔符转换为正斜线（/）
	// 删除尾部目录分隔符
	// 该函数返回标准化路径的长度。
	// 注意：该函数修改了原始输入数组。
	// 与 std::filesystem::path::lexically_normal() 等同
	template<class CharType>
	inline uint32_t GammaShortPath( CharType* szPath )
	{
		uint32_t nPos[256];
		uint32_t nCount = 0;
		uint32_t nCurPos = 0;
		uint32_t nPrePos = 0;

		for( uint32_t i = 0; szPath[i]; i++ )
		{
			if( szPath[i] == '.' && ( szPath[i+1] == '/' || szPath[i+1] == '\\' ) )
			{
				i += 1;
				continue;
			}
			else if( szPath[i] == '.' && szPath[i+1] == '.' && ( szPath[i+2] == '/' || szPath[i+2] == '\\' ) )
			{
				i += 2;
				if( nCount )
				{
					nCurPos = nPos[--nCount];
					nPrePos = nCurPos;
					continue;
				}
				else
				{
					szPath[nCurPos++] = '.';
					szPath[nCurPos++] = '.';
					szPath[nCurPos++] = '/';
					nPrePos = nCurPos;
					continue;
				}
			}

			if( szPath[i] == '/' || szPath[i] == '\\' )
			{
				szPath[nCurPos++] = '/';
				nPos[nCount++] = nPrePos;
				nPrePos = nCurPos;
			}
			else
			{
				szPath[nCurPos++] = szPath[i];
			}
		}	

		szPath[nCurPos] = 0;
		return (int32_t)nCurPos;
	}
	
	template<typename CharType, uint32_t n, typename FunctionType> 
	inline int32_t _FileTreeWalk( CharType( &szFullPath )[n], uint32_t nStrLen,
		FunctionType pfnFileProc, void* pContext, uint32_t nDepth, bool bFullFilePath )
	{
		FILE_FOUND* pFound = GammaFindFirstFile( szFullPath );
		if ( !pFound )
			return 0;

		CharType* szFileName = szFullPath + nStrLen;

		do
		{
			uint32_t nLen = GammaGetFileName( pFound, szFileName, n - nStrLen );
			if( szFileName[0] == '.' )
				continue;

			CPathMgr::FTW_FLAG eFlag = GammaIsDirectory( pFound ) ? 
				CPathMgr::eFTW_FLAG_DIR : CPathMgr::eFTW_FLAG_FILE;
			CPathMgr::FTW_RESULT ret = pfnFileProc( 
				bFullFilePath ? szFullPath : szFileName, eFlag, pContext );

			if( ret == CPathMgr::eFTW_RESULT_STOP )
			{
				GammaFindClose( pFound );
				return 1;
			}

			if( eFlag == CPathMgr::eFTW_FLAG_DIR && ret != CPathMgr::eFTW_RESULT_IGNORE )
			{
				if( szFileName[ nLen - 1 ] != '/' && szFileName[ nLen - 1 ] != '\\' )
					szFileName[ nLen++ ] =  '/';
				szFileName[nLen] = 0;

				if( nDepth && _FileTreeWalk( szFullPath, 
					nStrLen + nLen, pfnFileProc, pContext, nDepth - 1, bFullFilePath ) )
				{
					GammaFindClose( pFound );				
					return 1;
				}
			}
		}
		while( GammaFindNextFile( pFound ) );
		GammaFindClose( pFound );
		return 0;
	}

	template<typename CharType, uint32_t n> 
	inline void _DeleteDirectory( CharType( &szFullPath )[n], uint32_t nStrLen )
	{
		FILE_FOUND* pFound = GammaFindFirstFile( szFullPath );
		if ( !pFound )
			return;

		CharType* szFileName = szFullPath + nStrLen;

		do
		{
			uint32_t nLen = GammaGetFileName( pFound, szFileName, n - nStrLen );
			if( ( szFileName[0] == '.' && szFileName[1] == 0 ) ||
				( szFileName[0] == '.' && szFileName[1] == '.' && szFileName[2] == 0 ) )
				continue;

			if( GammaIsDirectory( pFound ) )
			{
				if( szFileName[ nLen - 1 ] != '/' && szFileName[ nLen - 1 ] != '\\' )
					szFileName[ nLen++ ] =  '/';
				szFileName[nLen] = 0;
				_DeleteDirectory( szFullPath, nStrLen + nLen );
			}
			else
			{
				CPathMgr::DeleteFile( szFullPath );
			}
		}
		while( GammaFindNextFile( pFound ) );
		GammaFindClose( pFound );

		szFullPath[nStrLen] = 0;
		GammaDeleteDirectory( szFullPath );
	}
}

#endif
