/*
*	CPathMgr.cpp	文件路径管理器的实现
*/


#include "GammaCommon/CPathMgr.h"
#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/GammaCodeCvs.h"
#include "GammaCommon/TGammaStrStream.h"
#include "GammaCommon/CThread.h"
#include "CGammaFileMgr.h"
#include "PathHelp.h"

#include "Android.h"
#include "IOS.h"

#include <errno.h>
#include <filesystem>
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
	//===================================================================================
	//
	//===================================================================================
	struct SPathContext
	{
		CLock	m_Lock;
		char	m_szCurrPath[2048];		// 当前路径
        uint32_t  m_nPkgRootLen;
        uint32_t  m_nExternalLen;
		SPathContext();
	};

	static SPathContext s_PathContex;

	// 设置root路径和当前路径
	SPathContext::SPathContext()
	{
		m_Lock.Lock();
#ifdef WIN32
		wchar_t szBuffer[ELEM_COUNT( m_szCurrPath )];
		if( NULL == _wgetcwd( szBuffer, ELEM_COUNT( szBuffer ) ) )
		{
			m_Lock.Unlock();
			GammaThrow( "init cur path error" );
		}
		UcsToUtf8( m_szCurrPath, ELEM_COUNT( m_szCurrPath ), szBuffer );
#else
		if( NULL == getcwd( m_szCurrPath, ELEM_COUNT( m_szCurrPath ) ) )
		{
			m_Lock.Unlock();
			GammaThrow( "init cur path error" );
		}
#endif

		GammaString( m_szCurrPath );
		size_t nLen = strlen( m_szCurrPath );
		if( nLen && m_szCurrPath[nLen - 1] != '/' )
			m_szCurrPath[nLen++] = '/';
		m_szCurrPath[nLen] = 0;
		m_Lock.Unlock();
        
        m_nPkgRootLen = (uint32_t)strlen( "pkgroot:/" );
        m_nExternalLen = (uint32_t)strlen( "external:/" );
	}

	// 取得当前文件路径
	const char* CPathMgr::GetCurPath()
	{
		return s_PathContex.m_szCurrPath;
	}

	/******************************************************************************
	// Purpose		: 设置当前文件路径
	// Argument		: wchar_t* szPathName[in]	传入的是相对于根目录的相对路径或者绝对路径
	// Comments		: 如果传入的是 NULL 指针,则将原始的根目录设为当前目录
	*****************************************************************************/
	void CPathMgr::SetCurPath( const char* szPathName )
	{
		s_PathContex.m_Lock.Lock();
		if( IsAbsolutePath( szPathName ) )
			strcpy2array_safe( s_PathContex.m_szCurrPath, szPathName );
		else
			strcat2array_safe( s_PathContex.m_szCurrPath, szPathName );

		uint32_t nLen = ShortPath( s_PathContex.m_szCurrPath );
		if( s_PathContex.m_szCurrPath[ nLen - 1 ] != '/' )
			s_PathContex.m_szCurrPath[nLen++] = '/';
		s_PathContex.m_szCurrPath[nLen] = 0;

#ifdef _WIN32
		_wchdir( Utf8ToUcs( s_PathContex.m_szCurrPath ).c_str() );
#else
		uint32_t nOffset = 0;

#ifdef _ANDROID
		if( !memcmp( "pkgroot:/", s_PathContex.m_szCurrPath, sizeof(char)*s_PathContex.m_nPkgRootLen ) )
		{
			s_PathContex.m_Lock.Unlock();
			return;
		}
		else if( !memcmp( "external:/", s_PathContex.m_szCurrPath, sizeof(char)*s_PathContex.m_nExternalLen ) )
		{
			chdir( CAndroidApp::GetInstance().GetExternalPath() );
			nOffset = strlen( "external:/" );
        }
#elif defined _IOS
        if( !memcmp( "pkgroot:/", s_PathContex.m_szCurrPath, sizeof(char)*s_PathContex.m_nPkgRootLen ) )
        {
            chdir( CIOSApp::GetInstance().GetPackagePath() );
            const char* szFind = strchr( s_PathContex.m_szCurrPath, ':' );
            nOffset = (uint32_t)( szFind - s_PathContex.m_szCurrPath + 2 );
        }
        
        if( !memcmp( "external:/", s_PathContex.m_szCurrPath, sizeof(char)*s_PathContex.m_nExternalLen ) )
        {
            chdir( CIOSApp::GetInstance().GetCachePath() );
            const char* szFind = strchr( s_PathContex.m_szCurrPath, ':' );
            nOffset = (uint32_t)( szFind - s_PathContex.m_szCurrPath + 2 );
        }
#endif
        chdir( s_PathContex.m_szCurrPath + nOffset );
#endif
        s_PathContex.m_Lock.Unlock();
	}

	void CPathMgr::SetCurPath( const wchar_t* szPathName )
	{
		SetCurPath( UcsToUtf8( szPathName ).c_str() );
	}
    
    bool CPathMgr::IsAbsolutePath( const wchar_t* szPath )
    {
        return szPath[0] == L'/' || ::wcschr( szPath, L':' );
    }

	bool CPathMgr::IsAbsolutePath( const char* szPath )
	{
        return szPath[0] == '/' || ::strchr( szPath, ':' );
	}

	bool CPathMgr::IsFileExist( const wchar_t* szPath )
	{
		wchar_t szPhysicalPath[2048];
		ToPhysicalPath( szPath, szPhysicalPath, ELEM_COUNT(szPhysicalPath) );
#ifdef _WIN32
		FILE* fp = _wfopen( szPhysicalPath, L"r" );
#else
		FILE* fp = fopen( UcsToUtf8( szPhysicalPath ).c_str(), "r" );
#endif
		if( fp )
			fclose( fp );
		return fp != NULL;
	}

	bool CPathMgr::IsFileExist( const char* szPath )
	{
		CPackageMgr& PackageMgr = CGammaFileMgr::Instance().GetFilePackageManager();
		if( !GammaStringCompare( szPath, "pkgroot:/", 9 ) )
			return PackageMgr.IsFileInCurrentPackage( szPath + 9 );

		char szPhysicalPath[2048];
		ToPhysicalPath( szPath, szPhysicalPath, ELEM_COUNT(szPhysicalPath) );
#ifdef _WIN32
		FILE* fp = _wfopen( Utf8ToUcs( szPhysicalPath ).c_str(), L"r" );
#else
		FILE* fp = fopen( szPhysicalPath, "r" );
#endif
		if( fp )
			fclose( fp );
		return fp != NULL;
	}

	const wchar_t* CPathMgr::ToAbsolutePath( const wchar_t* szSrcPath, wchar_t* szAbsolutePath, uint32_t nMaxSize )
	{
		if( IsAbsolutePath( szSrcPath ) )
		{
			wcsncpy( szAbsolutePath, szSrcPath, nMaxSize );
			szAbsolutePath[ nMaxSize - 1 ] = 0;
			return szAbsolutePath;
		}

		s_PathContex.m_Lock.Lock();
		Utf8ToUcs( szAbsolutePath, (uint32_t)nMaxSize, s_PathContex.m_szCurrPath );
		s_PathContex.m_Lock.Unlock();

		szAbsolutePath[ nMaxSize - 1 ] = 0;
		wcsncat( szAbsolutePath, szSrcPath, nMaxSize - 1 - wcslen( szAbsolutePath ) );
		szAbsolutePath[ nMaxSize - 1 ] = 0;
		return szAbsolutePath;
	}

	const char* CPathMgr::ToAbsolutePath( const char* szSrcPath, char* szAbsolutePath, uint32_t nMaxSize )
	{
		if( IsAbsolutePath( szSrcPath ) )
		{
			strcpy_safe( szAbsolutePath, szSrcPath, (uint32_t)nMaxSize, INVALID_32BITID );
			szAbsolutePath[ nMaxSize - 1 ] = 0;
			return szAbsolutePath;
		}

		s_PathContex.m_Lock.Lock();
		strcpy_safe( szAbsolutePath, s_PathContex.m_szCurrPath, (uint32_t)nMaxSize, INVALID_32BITID );
		s_PathContex.m_Lock.Unlock();

		szAbsolutePath[ nMaxSize - 1 ] = 0;
		strcat( szAbsolutePath, szSrcPath );
		szAbsolutePath[ nMaxSize - 1 ] = 0;
		return szAbsolutePath;
	}

	const wchar_t* CPathMgr::ToPhysicalPath( const wchar_t* szAbsolutePath, wchar_t* szPhysicalPath, uint32_t nMaxSize )
	{
#ifdef _ANDROID
		uint32_t nExtLen = wcslen( L"external:/" );
		if( memcmp( L"external:/", szAbsolutePath, sizeof(wchar_t)*nExtLen ) )
			return CPathMgr::ToAbsolutePath( szAbsolutePath, szPhysicalPath, nMaxSize );
		wchar_t szExternalPath[1024];
		Utf8ToUcs( szExternalPath, ELEM_COUNT( szExternalPath ), CAndroidApp::GetInstance().GetExternalPath() );
		uint32_t nPhysicalExtLen = wcslen( szExternalPath );
		CPathMgr::ToAbsolutePath( szAbsolutePath, szPhysicalPath + nPhysicalExtLen - nExtLen, nMaxSize - nPhysicalExtLen + nExtLen );
		memcpy( szPhysicalPath, szExternalPath, nPhysicalExtLen*sizeof(wchar_t) );
        return szPhysicalPath;
#elif defined _IOS
        uint32_t nExtLen = 0;
        string strSysPath;
        if( !memcmp( L"pkgroot:/", szAbsolutePath, sizeof(wchar_t)*wcslen( L"pkgroot:/" ) ) )
        {
            strSysPath = CIOSApp::GetInstance().GetPackagePath() + string( "assets/" );
            nExtLen = s_PathContex.m_nPkgRootLen;
        }
        else if( !memcmp( L"external:/", szAbsolutePath, sizeof(wchar_t)*wcslen( L"external:/" ) ) )
        {
            strSysPath = CIOSApp::GetInstance().GetCachePath();
            nExtLen = s_PathContex.m_nExternalLen;
        }
        
        if( strSysPath.empty() )
            return CPathMgr::ToAbsolutePath( szAbsolutePath, szPhysicalPath, nMaxSize );
        
        wchar_t szExternalPath[1024];
        Utf8ToUcs( szExternalPath, ELEM_COUNT( szExternalPath ), strSysPath.c_str() );
        uint32_t nPhysicalExtLen = (uint32_t)wcslen( szExternalPath );
        CPathMgr::ToAbsolutePath( szAbsolutePath, szPhysicalPath + nPhysicalExtLen - nExtLen, nMaxSize - nPhysicalExtLen + nExtLen );
        memcpy( szPhysicalPath, szExternalPath, nPhysicalExtLen*sizeof(wchar_t) );
        return szPhysicalPath;
#else
		return CPathMgr::ToAbsolutePath( szAbsolutePath, szPhysicalPath, nMaxSize );
#endif
	}

	const char* CPathMgr::ToPhysicalPath( const char* szAbsolutePath, char* szPhysicalPath, uint32_t nMaxSize )
	{
#ifdef _ANDROID
		uint32_t nExtLen = strlen( "external:/" );
		if( memcmp( "external:/", szAbsolutePath, sizeof(char)*nExtLen ) )
			return CPathMgr::ToAbsolutePath( szAbsolutePath, szPhysicalPath, nMaxSize );
		uint32_t nPhysicalExtLen = strlen( CAndroidApp::GetInstance().GetExternalPath() );
		CPathMgr::ToAbsolutePath( szAbsolutePath, szPhysicalPath + nPhysicalExtLen - nExtLen, nMaxSize - nPhysicalExtLen + nExtLen );
		memcpy( szPhysicalPath, CAndroidApp::GetInstance().GetExternalPath(), nPhysicalExtLen*sizeof(char) );
		return szPhysicalPath;
#elif defined _IOS
		uint32_t nExtLen = 0;
		string strSysPath;
        if( !memcmp( "pkgroot:/", szAbsolutePath, strlen( "pkgroot:/" ) ) )
		{
			strSysPath = CIOSApp::GetInstance().GetPackagePath() + string( "assets/" );
            nExtLen = s_PathContex.m_nPkgRootLen;
        }
        else if( !memcmp( "external:/", szAbsolutePath, strlen( "external:/" ) ) )
        {
            strSysPath = CIOSApp::GetInstance().GetCachePath();
            nExtLen = s_PathContex.m_nExternalLen;
        }
        
        if( strSysPath.empty() )
            return CPathMgr::ToAbsolutePath( szAbsolutePath, szPhysicalPath, nMaxSize );
        
        uint32_t nPhysicalExtLen = (uint32_t)strSysPath.size();
        CPathMgr::ToAbsolutePath( szAbsolutePath, szPhysicalPath + nPhysicalExtLen - nExtLen, nMaxSize - nPhysicalExtLen + nExtLen );
        memcpy( szPhysicalPath, strSysPath.c_str(), nPhysicalExtLen*sizeof(char) );
        return szPhysicalPath;
#else
		return CPathMgr::ToAbsolutePath( szAbsolutePath, szPhysicalPath, nMaxSize );
#endif
	}
	
	void CPathMgr::GetPathSpaceInfo( const wchar_t* szPath, uint64* pAvailableSize, uint64* pFreeSize, uint64* pTotalSize )
	{
		wchar_t szAbsolutePath[2048];
		szPath = ToPhysicalPath( szPath, szAbsolutePath, ELEM_COUNT( szAbsolutePath ) );

#ifdef _WIN32
		ULARGE_INTEGER nFreeBytes, nTotalBytes, nAvailableSize;
		GetDiskFreeSpaceExW( szPath, &nFreeBytes, &nTotalBytes, &nAvailableSize );
		if( pFreeSize ) *pFreeSize = nFreeBytes.QuadPart;
		if( pTotalSize ) *pTotalSize = nTotalBytes.QuadPart;
		if( pAvailableSize ) *pAvailableSize = nAvailableSize.QuadPart;
#else
		struct statfs FileStat;
		statfs( UcsToUtf8( szPath ).c_str(), &FileStat );
		if( pFreeSize ) *pFreeSize = uint64( FileStat.f_bavail )*uint64( FileStat.f_bsize );
		if( pTotalSize ) *pTotalSize = uint64( FileStat.f_blocks )*uint64( FileStat.f_bsize );
		if( pAvailableSize ) *pAvailableSize = uint64( FileStat.f_bfree )*uint64( FileStat.f_bsize );
#endif
	}

	void CPathMgr::GetPathSpaceInfo( const char* szPath, uint64* pAvailableSize, uint64* pFreeSize, uint64* pTotalSize )
	{
		char szAbsolutePath[2048];
		szPath = ToPhysicalPath( szPath, szAbsolutePath, ELEM_COUNT( szAbsolutePath ) );

#ifdef _WIN32
		ULARGE_INTEGER nFreeBytes, nTotalBytes, nAvailableSize;
		GetDiskFreeSpaceExW( Utf8ToUcs( szPath ).c_str(), &nFreeBytes, &nTotalBytes, &nAvailableSize );
		if( pFreeSize ) *pFreeSize = nFreeBytes.QuadPart;
		if( pTotalSize ) *pTotalSize = nTotalBytes.QuadPart;
		if( pAvailableSize ) *pAvailableSize = nAvailableSize.QuadPart;
#else
		struct statfs FileStat;
		statfs( szPath, &FileStat );
		if( pFreeSize ) *pFreeSize = uint64( FileStat.f_bavail )*uint64( FileStat.f_bsize );
		if( pTotalSize ) *pTotalSize = uint64( FileStat.f_blocks )*uint64( FileStat.f_bsize );
		if( pAvailableSize ) *pAvailableSize = uint64( FileStat.f_bfree )*uint64( FileStat.f_bsize );
#endif
	}

	CPathMgr::FTW_FLAG CPathMgr::GetFlag( const wchar_t* szPath )
	{
		wchar_t szAbsolutePath[2048];
		szPath = ToPhysicalPath( szPath, szAbsolutePath, ELEM_COUNT( szAbsolutePath ) );

#ifdef _WIN32
		struct _stat FileStat;
		_wstat( szPath, &FileStat );
#else
		struct stat FileStat;
		stat( UcsToUtf8( szPath ).c_str(), &FileStat );
#endif
		return ( FileStat.st_mode & S_IFDIR ) ? eFTW_FLAG_DIR : eFTW_FLAG_FILE;
	}

	CPathMgr::FTW_FLAG CPathMgr::GetFlag( const char* szPath )
	{
		char szAbsolutePath[2048];
		szPath = ToPhysicalPath( szPath, szAbsolutePath, ELEM_COUNT( szAbsolutePath ) );

#ifdef _WIN32
		struct _stat FileStat;
		_wstat( Utf8ToUcs( szPath ).c_str(), &FileStat );
#else
		struct stat FileStat;
		stat( szPath, &FileStat );
#endif
		return ( FileStat.st_mode & S_IFDIR ) ? eFTW_FLAG_DIR : eFTW_FLAG_FILE;
	}

	static CPathMgr::FTW_RESULT Utf8ToUcsWalkCallBack( const char* szFileName, CPathMgr::FTW_FLAG eFlag, void* pContext )
	{
		wchar_t szUcs2Path[2048];
		Utf8ToUcs( szUcs2Path, 2000, szFileName );
		pair<CPathMgr::FILE_PROC, void*> CallbackContext = *(pair<CPathMgr::FILE_PROC, void*>*)pContext;
		return CallbackContext.first( szUcs2Path, eFlag, CallbackContext.second );
	}

	int32_t CPathMgr::FileTreeWalk( const wchar_t* szDir, FILE_PROC pfnFileProc, void* pContext, uint32_t nDepth, bool bFullFilePath )
	{
		pair<FILE_PROC, void*> CallbackContext( pfnFileProc, pContext );
		string strUtf8Dir = UcsToUtf8( szDir );
		if( CGammaFileMgr::Instance().GetFilePackageManager().FileTreeWalk( 
			strUtf8Dir.c_str(), Utf8ToUcsWalkCallBack, &CallbackContext, nDepth, bFullFilePath ) )
			return 0;

		wchar_t szAbsolutePath[2048];
		ToPhysicalPath( szDir, szAbsolutePath, ELEM_COUNT( szAbsolutePath ) );
		uint32_t nLen = (uint32_t)wcslen( szAbsolutePath );
		if( szAbsolutePath[ nLen - 1 ] != '/' && szAbsolutePath[ nLen - 1 ] != '\\' )
			szAbsolutePath[ nLen++ ] =  '/';
		szAbsolutePath[nLen] = 0;
		const wchar_t* szFind = wcschr( szAbsolutePath, ':' );
		if( ( !szFind && szAbsolutePath[0] != L'/' ) || !GammaStringCompare( szAbsolutePath, L"http://", 7 ) )
			return 0;
		return _FileTreeWalk( szAbsolutePath, nLen, pfnFileProc, pContext, nDepth, bFullFilePath );
	}

	int32_t CPathMgr::FileTreeWalk( const char* szDir, FILE_PROC_UTF8 pfnFileProc, void* pContext, uint32_t nDepth, bool bFullFilePath )
	{
		if( CGammaFileMgr::Instance().GetFilePackageManager().FileTreeWalk( szDir, pfnFileProc, pContext, nDepth, bFullFilePath ) )
			return 0;

		char szAbsolutePath[2048];
		ToPhysicalPath( szDir, szAbsolutePath, ELEM_COUNT( szAbsolutePath ) );
		uint32_t nLen = (uint32_t)strlen( szAbsolutePath );
		if( szAbsolutePath[ nLen - 1 ] != '/' && szAbsolutePath[ nLen - 1 ] != '\\' )
			szAbsolutePath[ nLen++ ] =  '/';
		szAbsolutePath[nLen] = 0;
		const char* szFind = strchr( szAbsolutePath, ':' );
		if( ( !szFind && szAbsolutePath[0] != L'/' ) || !GammaStringCompare( szAbsolutePath, "http://", 7 ) )
			return 0;
		return _FileTreeWalk( szAbsolutePath, nLen, pfnFileProc, pContext, nDepth, bFullFilePath );
	}

	void CPathMgr::MakeDirectory( const wchar_t* szDirectory, uint32_t nAccessMode )
	{
		wchar_t szBuf[2048];
		ToPhysicalPath( szDirectory, szBuf, ELEM_COUNT( szBuf ) );

#ifdef _WIN32
		wchar_t* pCur = szBuf;
		for(;;)
		{
			wchar_t* pCurDir = pCur;
			while( pCurDir[0] != '\\' && pCurDir[0] != '/' )
			{
				if( !pCurDir[0] )
				{
					if( pCurDir != pCur )
						_wmkdir( szBuf );
					return;
				}
				pCurDir++;
			}
			pCurDir[0] = 0;
			_wmkdir( szBuf ); 
			pCurDir[0] = L'/';
			pCur = pCurDir + 1;
		}
#else
		if( nAccessMode == 0 )
			nAccessMode = S_IRWXU;

		char szUtf8[2048] = { 0 };
		UcsToUtf8( szUtf8, 2048, szBuf );

		char* pCur = szUtf8;
		for(;;)
		{
			char* pCurDir = pCur;
			while( pCurDir[0] != '\\' && pCurDir[0] != '/' )
			{
				if( !pCurDir[0] )
				{
					if( pCurDir != pCur )
						mkdir( szUtf8, nAccessMode );
					return;
				}
				pCurDir++;
			}
			pCurDir[0] = 0;
			mkdir( szUtf8, nAccessMode );
			pCurDir[0] = L'/';
			pCur = pCurDir + 1;
		}
#endif
	}

	void CPathMgr::MakeDirectory( const char* szDirectory, uint32_t nAccessMode )
	{
		wchar_t szBuf[2048];
		Utf8ToUcs( szBuf, 2048, szDirectory );
		MakeDirectory( szBuf, nAccessMode );
	}
	
	void CPathMgr::DeleteDirectory( const wchar_t* szDirectory )
	{
		wchar_t szAbsolutePath[2048];
		ToPhysicalPath( szDirectory, szAbsolutePath, ELEM_COUNT( szAbsolutePath ) );
		uint32_t nLen = (uint32_t)wcslen( szAbsolutePath );
		if( szAbsolutePath[ nLen - 1 ] != '/' && szAbsolutePath[ nLen - 1 ] != '\\' )
			szAbsolutePath[ nLen++ ] =  '/';
		szAbsolutePath[nLen] = 0;
		_DeleteDirectory( szAbsolutePath, nLen );
	}

	void CPathMgr::DeleteDirectory( const char* szDirectory )
	{
		char szAbsolutePath[2048];
		ToPhysicalPath( szDirectory, szAbsolutePath, ELEM_COUNT( szAbsolutePath ) );
		uint32_t nLen = (uint32_t)strlen( szAbsolutePath );
		if( szAbsolutePath[ nLen - 1 ] != '/' && szAbsolutePath[ nLen - 1 ] != '\\' )
			szAbsolutePath[ nLen++ ] =  '/';
		szAbsolutePath[nLen] = 0;
		_DeleteDirectory( szAbsolutePath, nLen );
	}

	void CPathMgr::DeleteFile( const wchar_t* szPath )
	{
		wchar_t szBuf[2048];
		szPath = ToPhysicalPath( szPath, szBuf, ELEM_COUNT( szBuf ) );
#ifdef _WIN32
		_wremove( szPath );
#else
		remove( UcsToUtf8( szPath ).c_str() );
#endif
	}

	void CPathMgr::DeleteFile( const char* szPath )
	{
		wchar_t szBuf[2048];
		Utf8ToUcs( szBuf, 2048, szPath );
		DeleteFile( szBuf );
	}

	void CPathMgr::RenamePath( const wchar_t* szOldName, const wchar_t* szNewName )
	{
		wchar_t szBufOld[2048];
		wchar_t szBufNew[2048];
		szOldName = ToPhysicalPath( szOldName, szBufOld, ELEM_COUNT( szBufOld ) );
		szNewName = ToPhysicalPath( szNewName, szBufNew, ELEM_COUNT( szBufNew ) );
#ifdef _WIN32
		_wrename( szOldName, szNewName );
#else
		rename( UcsToUtf8( szOldName ).c_str(), UcsToUtf8( szNewName ).c_str() );
#endif
	}

	void CPathMgr::RenamePath( const char* szOldName, const char* szNewName )
	{
		wchar_t szBufOld[2048];
		wchar_t szBufNew[2048];
		Utf8ToUcs( szBufOld, 2048, szOldName );
		Utf8ToUcs( szBufNew, 2048, szNewName );
		RenamePath( szBufOld, szBufNew );
	}

	uint32_t CPathMgr::ShortPath( char* szPath )
	{
		return GammaShortPath( szPath );
	}

	uint32_t CPathMgr::ShortPath(std::string& szPath)
	{
		uint32_t nret = GammaShortPath(&szPath[0]);
		szPath = szPath.c_str();
		return nret;
	}

	uint32_t CPathMgr::ShortPath( wchar_t* szPath )
	{
		return GammaShortPath( szPath );
	}

	const char* CPathMgr::GetRelativePath( const char* szBaseDir, const char* szCheckDir, char* szRelativePath, size_t nMaxSize )
	{
		char szAbsoluteBaseDir[2048];
		char szAbsoluteCheckDir[2048];
		ToPhysicalPath( szBaseDir, szAbsoluteBaseDir, 2048 );
		ToPhysicalPath( szCheckDir, szAbsoluteCheckDir, 2048 );
		GammaString( &szAbsoluteBaseDir[0] );
		GammaString( &szAbsoluteCheckDir[0] );

		uint32_t nLenBase = (uint32_t)strlen( szAbsoluteBaseDir );
		uint32_t nLenCheck = (uint32_t)strlen( szAbsoluteCheckDir );
		if( szAbsoluteBaseDir[nLenBase - 1] != '/' )
			szAbsoluteBaseDir[nLenBase++] = '/';

		if( szAbsoluteBaseDir[0] != szAbsoluteCheckDir[0] )
			return NULL;

		nLenBase = ShortPath( &szAbsoluteBaseDir[0] );
		nLenCheck = ShortPath( &szAbsoluteCheckDir[0] );

		size_t i = 0;
		const char* szCurBaseDirPos = szAbsoluteBaseDir;
		const char* szCurCheckDirPos = szAbsoluteCheckDir;
		for( size_t j = i; szCurBaseDirPos[j] == szCurCheckDirPos[j] && szCurCheckDirPos[j]; j++ )
			if( szCurBaseDirPos[j] == '/' )
				i = j + 1;

		string szRelative = szCurCheckDirPos + i;
		for( size_t j = i; j < nLenBase; j++ )
			if( szAbsoluteBaseDir[j] == '/' )
				szRelative = "../" + szRelative;
		if( szRelative.empty() )
			szRelative = "./";

		strcpy_safe( szRelativePath, szRelative.c_str(), (uint32_t)nMaxSize, INVALID_32BITID );
		return szRelativePath;
	}

	bool CPathMgr::CheckDirectory( const char* szDirectory )
	{
		char szBuf[2048];
		szDirectory = ToPhysicalPath( szDirectory, szBuf, ELEM_COUNT( szBuf ) );

		bool ret = false;  
#ifdef _WIN32
		WIN32_FIND_DATAW fd;    
		  
		HANDLE hFind = FindFirstFileW( Utf8ToUcs( szDirectory ).c_str(), &fd );    
		if ( (hFind != INVALID_HANDLE_VALUE) &&
			( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
			ret = true;     
		FindClose(hFind);    
#else

		struct stat buf;
		stat( szDirectory, &buf );
		if( S_ISDIR( buf.st_mode ) != 0 )
			ret = true;
#endif		
		return ret;
	}
}
