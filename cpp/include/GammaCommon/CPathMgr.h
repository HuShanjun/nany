/*
*	CPathMgr.h	文件路径管理器，管理资源路径操作
*/

#ifndef _GAMMA_CPATHMGR_H_
#define _GAMMA_CPATHMGR_H_


#ifdef _WIN32
#include <Windows.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string>

#include "GammaCommon/GammaCommonType.h"

namespace Gamma
{
	class GAMMA_COMMON_API CPathMgr
	{
	public:
		enum FTW_FLAG
		{
		#ifndef _WIN32
			eFTW_FLAG_FILE = 0,
			eFTW_FLAG_DIR  = 1,
			eFTW_FLAG_SL   = 2,
			eFTW_FLAG_NS   = 3,
			eFTW_FLAG_TMP,
		#else
			eFTW_FLAG_FILE = FILE_ATTRIBUTE_NORMAL,
			eFTW_FLAG_DIR  = FILE_ATTRIBUTE_DIRECTORY,
			eFTW_FLAG_TMP  = FILE_ATTRIBUTE_TEMPORARY,
			eFTW_FLAG_SL,
			eFTW_FLAG_NS,

		#endif
		};

		enum FTW_RESULT
		{
			eFTW_RESULT_STOP,
			eFTW_RESULT_IGNORE,
			eFTW_RESULT_CONTINUNE,
		};

		typedef FTW_RESULT(* FILE_PROC)( const wchar_t*, FTW_FLAG, void* );
		typedef FTW_RESULT(* FILE_PROC_UTF8)( const char*, FTW_FLAG, void* );

		// 得到当前路径
		static const char*		GetCurPath();
		// 设置当前路径,传入的是相对于根目录的相对路径
		static void				SetCurPath( const char* szPathName );	
		static void				SetCurPath( const wchar_t* szPathName );	
	
		//严重注意，这个函数不会搜索任何名为".svn"目录的内容
		static int32_t			FileTreeWalk( const wchar_t* szDir, FILE_PROC pfnFileProc,
									void* pContext, uint32_t nDepth = -1, bool bFullFilePath = true );
		static int32_t			FileTreeWalk( const char* szDir, FILE_PROC_UTF8 pfnFileProc,
									void* pContext, uint32_t nDepth = -1, bool bFullFilePath = true );

		static void				MakeDirectory( const wchar_t* szDirectory, uint32_t nAccessMode = 0 );
		static void				MakeDirectory( const char* szDirectory, uint32_t nAccessMode = 0 );
		static uint32_t			ShortPath( wchar_t* szPath );
		static uint32_t			ShortPath( char* szPath );
		static uint32_t			ShortPath(std::string& szPath);
		static void				DeleteDirectory( const wchar_t* szDirectory );
		static void				DeleteDirectory( const char* szDirectory );
		static void				DeleteFile( const wchar_t* szPath );
		static void				DeleteFile( const char* szPath );
		static void				RenamePath( const wchar_t* szOldName, const wchar_t* szNewName );
		static void				RenamePath( const char* szOldName, const char* szNewName );
		static bool				IsAbsolutePath( const wchar_t* szPath );
		static bool				IsAbsolutePath( const char* szPath );
		static bool				IsFileExist( const wchar_t* szPath );
		static bool				IsFileExist( const char* szPath );
		static const wchar_t*	ToAbsolutePath( const wchar_t* szSrcPath, wchar_t* szAbsolutePath, uint32_t nMaxSize );
		static const char*		ToAbsolutePath( const char* szSrcPath, char* szAbsolutePath, uint32_t nMaxSize );
		static const wchar_t*	ToPhysicalPath( const wchar_t* szAbsolutePath, wchar_t* szPhysicalPath, uint32_t nMaxSize );
		static const char*		ToPhysicalPath( const char* szAbsolutePath, char* szPhysicalPath, uint32_t nMaxSize );
		static void				GetPathSpaceInfo( const wchar_t* szPath, uint64* pAvailableSize, uint64* pFreeSize, uint64* pTotalSize );
		static void				GetPathSpaceInfo( const char* szPath, uint64* pAvailableSize, uint64* pFreeSize, uint64* pTotalSize );
		static FTW_FLAG			GetFlag( const wchar_t* szPath );
		static FTW_FLAG			GetFlag( const char* szPath );
		
		//* 检查是否存在目录
		static bool				CheckDirectory( const char* szDirectory );

		// 得到szCheckDir相对于szBaseDir的相对路径，
		// 例如 szBaseDir = /project/bin/debug/， szCheckDir = /project/misc/setting/
		// 则返回值为../../misc/setting/
		// windows下不同盘符下的路径没有相对路径，返回值为NULL
		static const char*		GetRelativePath( const char* szBaseDir, const char* szCheckDir, char* szRelativePath, size_t nMaxSize );

	}; // End of class CPathMgr


}// End of namespace Vfs

#endif // End of #define _VFS_FILEPATHMGR_H_

