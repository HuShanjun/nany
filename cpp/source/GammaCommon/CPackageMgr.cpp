
#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/CIniFile.h"
#include "GammaCommon/CTableFile.h"
#include "GammaCommon/TGammaStrStream.h"
#include "GammaCommon/GammaTime.h"
#include "GammaCommon/IGammaFileMgr.h"
#include "GammaCommon/GammaCodeCvs.h"
#include "GammaCommon/CPathMgr.h"
#include "CReadFileThread.h"
#include "CGammaFileMgr.h"
#include "CPackageMgr.h"
#include "CPackage.h"
#include "PathHelp.h"

#ifdef _WIN32
#include "Win32.h"
#elif defined _ANDROID
#include "Android.h"
#include "limits.h"
#elif defined _IOS
#include "IOS.h"
#endif

namespace Gamma
{
	//=============================================================
	// CPackageMgr
	//=============================================================
	CFileContext::CFileContext()
	{
	}

	CFileContext::~CFileContext()
	{
	}

	uint32 CFileContext::Size() const
	{
		CPackage* pPackage = GetPackage();
		if( !pPackage )
			return 0;
		return pPackage->GetFileBuffer( m_nIndex ).m_nBufferSize;
	}

	const tbyte* CFileContext::GetBuffer() const
	{
		CPackage* pPackage = GetPackage();
		if( !pPackage )
			return NULL;
		SFileBuffer FileBuffer = pPackage->GetFileBuffer( m_nIndex );
		if( !FileBuffer.m_BufferPtr )
			return (const tbyte*)"";
		return (const tbyte*)FileBuffer.m_BufferPtr->c_str() + FileBuffer.m_nOffset;
	}

	CPackage* CFileContext::GetPackage() const
	{
		if( !m_pPackageNode )
			return NULL;
		SPackagePart* pPackageNode = m_pPackageNode;
		while( pPackageNode->m_nPackageIndex )
			pPackageNode = pPackageNode->m_pPrePart;
		return pPackageNode->m_pPackage;
	}

	bool CFileContext::IsInFileList() const
	{
		return m_nIndex != INVALID_32BITID;
	}

	//=============================================================
	// CAddressHttp
	//=============================================================
	CAddressHttp::CAddressHttp( const string& strKey, const string& strHost, uint16 nPort )
		: m_strOrg( strKey )
		, m_strHost( strHost )
		, m_nPort( nPort )
		, m_hLock( GammaCreateLock() )
	{
	}

	CAddressHttp::~CAddressHttp()
	{
		GammaDestroyLock( m_hLock );
	}

	const SAddr* CAddressHttp::GetAddr( uint32 nIndex )
	{
		const SAddr* pAddr = NULL;
		GammaLock( m_hLock );
		if( m_vecAddrHttp.empty() )
		{
			addrinfo* pResult;
			addrinfo Info;
			memset( &Info, 0, sizeof(Info) );
			Info.ai_family = AF_UNSPEC;
			Info.ai_socktype = SOCK_STREAM;

			const char* szHost = m_strHost.c_str();
			int ret = getaddrinfo( szHost, NULL, &Info, &pResult );
			if( ret == 0 )
			{
				while( pResult )
				{
					if( pResult->ai_family == AF_INET )
					{
						SAddr AddrInfo;
						AddrInfo.nAiFamily = AF_INET;
						AddrInfo.strAddBuff.assign( (char*)pResult->ai_addr, sizeof(sockaddr_in) );
						( (sockaddr_in*)( &AddrInfo.strAddBuff[0] ) )->sin_port = htons( (u_short)m_nPort );
						m_vecAddrHttp.push_back( AddrInfo );
					}
					else if( pResult->ai_family == AF_INET6 )
					{
						SAddr AddrInfo;
						AddrInfo.nAiFamily = AF_INET6;
						AddrInfo.strAddBuff.assign( (char*)pResult->ai_addr, sizeof(sockaddr_in6) );
						( (sockaddr_in6*)( &AddrInfo.strAddBuff[0] ) )->sin6_port = htons( (u_short)m_nPort );
						m_vecAddrHttp.push_back( AddrInfo );
					}
					pResult = pResult->ai_next;
				}
			}
		}

		pAddr = nIndex < m_vecAddrHttp.size() ? &m_vecAddrHttp[nIndex] : NULL;
		GammaUnlock( m_hLock );

		return pAddr;
	}

	void CAddressHttp::SetMirror( const string& strMirror )
	{
		m_strMirror = strMirror;
	}

	inline bool operator < ( const string& v1, const CAddressHttp& v2 ) 
	{ return v1 < (const string&)v2; }

	inline bool operator < ( const CAddressHttp& v1, const string& v2 ) 
	{ return (const string&)v1 < v2; }

	inline bool operator < ( const CAddressHttp& v1, const CAddressHttp& v2 )
	{ return (const string&)v1 < (const string&)v2; }
	//=============================================================
	// CPackageMgr
	//=============================================================
	CPackageMgr::CPackageMgr( CGammaFileMgr* pFileMgr )
		: m_pFileMgr( pFileMgr )
		, m_bReleaseCache( true )
		, m_pFileMgrListener( NULL )
		, m_bLocalFiles( true )
		, m_Version( INVALID_64BITID )
		, m_eLoadContent( eLoadContent_Nothing )
		, m_nNeedExtractCount( 0 )
		, m_nNeedDownloadCount( 0 )
		, m_nPreCheckTime( 0 )
		, m_nPreDownloadTime( INVALID_32BITID )
		, m_bWifiConnected( false )
	{
#ifdef _ANDROID
		m_bFromFileList = true;
#endif
		m_itPackageCheck = m_listPackageParts.end();
	}

	CPackageMgr::~CPackageMgr()
	{
		while( m_mapHost.GetFirst() )
			delete m_mapHost.GetFirst();
#ifdef _DEBUG
		for( ListPackageType::iterator it = m_listPackageParts.begin(); it != m_listPackageParts.end(); it++ )
		{
			if( !it->m_pPackage )
				continue;
			GammaLog << it->m_strFilePath << endl;
		}
#endif
	}

	void CPackageMgr::SetBaseWebPath( const char* szPath, const char* szParam, bool bHttpAsLocal )
	{
		GammaLog << "SetBaseWebPath:" << szPath << "?" << ( szParam ? szParam : "" ) << endl;
		m_strBaseWebPath = szPath; 
		m_eLoadContent = eLoadContent_Version;

		char szVersionFileName[2048];
		if( szPath[1] == ':' || szPath[0] == '/' ||
		    !memcmp( "pkgroot:/", szPath, 9 ) ||
		    !memcmp( "external:/", szPath, 10 ) )
		{
			m_bLocalFiles = true;
			gammasstream( szVersionFileName, 2048 ) << m_strBaseWebPath.c_str();
			if( *m_strBaseWebPath.rbegin() != '/' && 
				m_pFileMgr->SyncLoad( szVersionFileName, false, this ) )
				return;
			OnLoadedEnd( szVersionFileName, NULL, 0 );
		}
		else
		{
			m_bLocalFiles = bHttpAsLocal;
			gammasstream url( szVersionFileName, 2048 );
			url << m_strBaseWebPath.c_str() << ( szParam && szParam[0] ? string( "?" ) + szParam : string() );
			m_pFileMgr->Load( szVersionFileName, false, false, this );
		}
	}

	bool CPackageMgr::ReadResourcePackageFile( string& strBuffer, const char* szNameInPackage ) const
	{
#ifdef _ANDROID
		const char* szPackagePath = CAndroidApp::GetInstance().GetPackagePath();
		if( szPackagePath == NULL )
			return false;

		char szNameInApk[2048] = "assets/";
		strcat( szNameInApk, szNameInPackage );
		unzFile hZip = unzOpen( szPackagePath );
		if( hZip )
		{
			// 定位到文件
			if( UNZ_OK == unzLocateFile( hZip, szNameInApk, true ) &&
				UNZ_OK == unzOpenCurrentFile( hZip ) )
			{
				unz_file_info info;
				// 获取被打开的文件信息
				unzGetCurrentFileInfo( hZip, &info, NULL, 0, NULL, 0, NULL, 0 );
				strBuffer.resize( info.uncompressed_size );
				// 读文件从zip包到内存
				if( unzReadCurrentFile( hZip, &strBuffer[ 0 ], info.uncompressed_size ) == (int)( info.uncompressed_size ) )
				{
					unzCloseCurrentFile( hZip );
					unzClose( hZip );
					return true;
				}
			}
			unzCloseCurrentFile( hZip );
			unzClose( hZip );
		}
		strBuffer.clear();
#else
	#ifdef _WIN32
		string strPackagePath = CWin32App::GetInstance().GetPackagePath();
	#elif defined _IOS
		string strPackagePath = CIOSApp::GetInstance().GetPackagePath();
	#else
		string strPackagePath = "";
	#endif

		strPackagePath += "assets/";
        strPackagePath += szNameInPackage;
        FILE* hFile = fopen( strPackagePath.c_str(), "rb" );
        if( hFile == NULL )
            return false;
        fseek( hFile, 0, SEEK_END );
        uint32 nSize = (uint32)ftell( hFile );
        fseek( hFile, 0, SEEK_SET );
        strBuffer.resize( nSize );
        bool bReadOK = fread( &strBuffer[0], 1, nSize, hFile ) == nSize;
        fclose( hFile );
        if( bReadOK )
            return true;
#endif
		return false;
	}

	bool CPackageMgr::ExtractPackageFile( EnumFileHandler funOnEnum, ReadFileHandler funOnRead, void* pContext ) const
	{
#ifdef _ANDROID
		const char* szPackagePath = CAndroidApp::GetInstance().GetPackagePath();
		if( szPackagePath == NULL )
			return false;

		unzFile hZip = unzOpen( szPackagePath );
		if( NULL == hZip )
			return false;			

		// 定位到第一个文件
		if( UNZ_OK != unzGoToFirstFile( hZip ) )
			return false;

		string strData;
		do
		{
			// 打开包内当前的文件
			if( UNZ_OK != unzOpenCurrentFile( hZip ) )
			{
				unzClose( hZip );
				return false;
			}

			unz_file_info unzFile = { 0 };
			char strInnerName[ 1024 ] = { 0 };   // 被打开的文件名
			char strComment[ 256 ]  = { 0 };    // 注释
			// 获取被打开的文件信息
			unzGetCurrentFileInfo( hZip, &unzFile, strInnerName, 
				sizeof(strInnerName), NULL, 0, strComment, sizeof(strComment) );
			static const char* szAssets = "assets/";
			static const uint32 nLen = strlen( szAssets );
			if( memcmp( strInnerName, szAssets, nLen ) )
				continue;

			if( funOnEnum( strInnerName + nLen, pContext ) )
			{
				strData.resize( unzFile.uncompressed_size );
				if( unzReadCurrentFile( hZip, &strData[ 0 ], 
					unzFile.uncompressed_size ) != (int)( unzFile.uncompressed_size ) )
				{
					unzCloseCurrentFile( hZip );
					unzClose( hZip );
					return false;
				}
				funOnRead( strInnerName + nLen, pContext, 
					(const tbyte*)strData.c_str(), unzFile.uncompressed_size );
				unzCloseCurrentFile( hZip );
			}			
		}
		while( UNZ_OK == unzGoToNextFile( hZip ) );

		unzClose( hZip );
#else
	#ifdef _WIN32
			string strPackagePath = m_pFileMgr->GetLocalCachePath();
	#elif defined _IOS
			string strPackagePath = CIOSApp::GetInstance().GetPackagePath();
	#else
			string strPackagePath = "";
	#endif

		strPackagePath += "assets/";
		struct SDirInfo
		{
			static CPathMgr::FTW_RESULT SearchDir( const char* szFile, CPathMgr::FTW_FLAG eFlag, void* pContext )
			{
				if( eFlag == CPathMgr::eFTW_FLAG_DIR )
					return CPathMgr::eFTW_RESULT_CONTINUNE;
				SDirInfo& Info = *(SDirInfo*)pContext;

				if( Info.m_funOnEnum( szFile + Info.m_nRootPathLen, Info.m_pContext ) )
				{
#ifdef _WIN32
					wchar_t szFileName[2048];
					Utf8ToUcs( szFileName, ELEM_COUNT(szFileName), szFile );
					FILE* hFile = _wfopen( szFileName, L"rb" );
#else
					FILE* hFile = fopen( szFile, "rb" );
#endif
					if( hFile == NULL )
						return CPathMgr::eFTW_RESULT_STOP;
					fseek( hFile, 0, SEEK_END );
					uint32 nSize = (uint32)ftell( hFile );
					fseek( hFile, 0, SEEK_SET );
					Info.m_strData.resize( nSize );
					if( fread( &Info.m_strData[0], 1, nSize, hFile ) != nSize )
					{
						fclose( hFile );
						return CPathMgr::eFTW_RESULT_STOP;
					}
					Info.m_funOnRead( szFile + Info.m_nRootPathLen, Info.m_pContext,
						(const tbyte*)Info.m_strData.c_str(), nSize );
					fclose( hFile );
				}			

				return CPathMgr::eFTW_RESULT_CONTINUNE;
			}

			EnumFileHandler m_funOnEnum;
			ReadFileHandler m_funOnRead;
			void*			m_pContext;
			size_t			m_nRootPathLen;
			string			m_strData;
		};

		SDirInfo DirInfo = { funOnEnum, funOnRead, pContext, strPackagePath.size() };
		if( CPathMgr::FileTreeWalk( strPackagePath.c_str(), 
			&SDirInfo::SearchDir, &DirInfo, INVALID_32BITID, true ) )
			return false;
#endif
		return true;
	}

	bool CPackageMgr::IsFileInCurrentPackage( const char* szNameInPackage ) const
	{
#ifdef _ANDROID		
		if( !m_strFileBuffer )
			ReadFileList( *( m_strFileBuffer = new CRefString ), m_setPkgFileList );
		gammacstring strName( szNameInPackage, true );
		return m_setPkgFileList.find( strName ) != m_setPkgFileList.end();
#else
	#ifdef _WIN32
		string strPackagePath = m_pFileMgr->GetLocalCachePath();
	#elif defined _IOS
		string strPackagePath = CIOSApp::GetInstance().GetPackagePath();
	#else
		string strPackagePath = "";
	#endif

		strPackagePath += "assets/";
        strPackagePath += szNameInPackage;
        if( CPathMgr::IsFileExist( strPackagePath.c_str() ) )
            return true;
#endif
		return false;
	}

	CPackage* CPackageMgr::CreatePackage( const char* szFullPathName )
	{
		if( !szFullPathName )
			return NULL;

		CFileContext* pFileContext = NULL;
		const char* szShortPath = RevertToShortPath( szFullPathName );
		const char* szIndexPath = szShortPath ? szShortPath : szFullPathName;
		pFileContext = GetFileContext( szIndexPath );
		if( !pFileContext )
		{
			pFileContext = &m_mapFileContext[szIndexPath];

#ifdef _DEBUG
			ListPackageType::iterator it = m_listPackageParts.begin();
			while( it != m_listPackageParts.end() && it->m_strFilePath != szIndexPath )
				++it;
			GammaAst( it == m_listPackageParts.end() );
#endif
			m_listPackageParts.push_back( SPackagePart() );
			SPackagePart& PkgPart = *m_listPackageParts.rbegin();
			PkgPart.m_strFilePath = szIndexPath;
			PkgPart.m_pPackage = 0;
			PkgPart.m_nOrgSize = 0;
			PkgPart.m_bLoaded = false;
			PkgPart.m_bCached = false;
			PkgPart.m_nPackageIndex = 0;	

			pFileContext->m_nIndex = 0;	
			pFileContext->m_pPackageNode = &PkgPart;
		}

		SPackagePart* pFirstPartNode = pFileContext->m_pPackageNode;
		while( pFirstPartNode->m_nPackageIndex )
			pFirstPartNode = pFirstPartNode->m_pPrePart;

		if( pFirstPartNode->m_pPackage )
		{
			pFirstPartNode->m_pPackage->AddRef();
			return pFirstPartNode->m_pPackage;
		}		

		// 使用最后一个PartNode来创建CPackage
		pFirstPartNode->m_pPackage = 
			new CPackage( this, pFileContext->m_pPackageNode );
		return pFirstPartNode->m_pPackage;
	}

	CFileContext* CPackageMgr::GetFileContext( const char* szPathName )
	{
		string strPathName = szPathName;
		GammaString( strPathName );
		MapFileContextType::iterator it = m_mapFileContext.find( strPathName );//文件内容无  如何get加载
		if( it != m_mapFileContext.end() )
			return &it->second;
		return NULL;
	}

	CFileContext* CPackageMgr::GetFileBuff( const char* szFileName )
	{
		const char* szIndexName = RevertToShortPath( szFileName );
		CFileContext* pContext = GetFileContext( szIndexName ? szIndexName : szFileName );
		if( !pContext )
			return NULL;

		SPackagePart* pFirstPartNode = pContext->m_pPackageNode;
		while( pFirstPartNode->m_nPackageIndex )
			pFirstPartNode = pFirstPartNode->m_pPrePart;
		CPackage* pPackage = pFirstPartNode->m_pPackage;
		if( !pPackage )
		{
			if( !m_bLocalFiles )
				return NULL;
			pPackage = CreatePackage( szFileName );
			m_pFileMgr->SyncLoad( szFileName, true, NULL );
		}
		else
		{
			pPackage->AddRef();
		}

		if( pPackage->GetLoadState() != eLoadState_Succeeded )
		{
			SAFE_RELEASE( pPackage );
			return NULL;
		}

		return pContext;
	}

	void CPackageMgr::ConvertToAbsolutePath( const char* szPathName, char szAbsolutePath[2048] ) const
	{
		// 有':'或'/'开头的视为绝对路径
		const char* szFind = strchr( szPathName, ':' );
		if( szFind || szPathName[0] == L'/' )
			szAbsolutePath[0] = 0;
		else if( !m_strBaseWebPath.empty() )
			strcpy_safe( szAbsolutePath, m_strBaseWebPath.c_str(), 2048, INVALID_32BITID );
		else
			strcpy_safe( szAbsolutePath, CPathMgr::GetCurPath(), 2048, INVALID_32BITID );

		strcat_safe( szAbsolutePath, szPathName, 2048, INVALID_32BITID );
		CPathMgr::ShortPath( szAbsolutePath );
	}

	const char* CPackageMgr::RevertToShortPath( const char* szAbsolutePath ) const
	{
		if( !memcmp( szAbsolutePath, m_strBaseWebPath.c_str(), sizeof(char)*m_strBaseWebPath.size() ) )
			return szAbsolutePath + m_strBaseWebPath.size();
		if( szAbsolutePath[0] == L'/' || strchr( szAbsolutePath, ':' ) )
			return NULL;
		return szAbsolutePath;
	}

	uint32 CPackageMgr::GetBasePathEnd( const char* szPathName )
	{
		if( !memcmp( szPathName, m_strBaseWebPath.c_str(), m_strBaseWebPath.size() ) )
			return (uint32)m_strBaseWebPath.size();
		return 0;
	}

	CAddressHttp* CPackageMgr::AddHost( string strKey, string strHost )
	{
		CAddressHttp* pAddress = m_mapHost.Find( strKey );
		if( pAddress == NULL )
		{			
			uint16 nPort = 80;
			string::size_type nPos = strHost.find_first_of( ":", 0 );
			if( nPos != string::npos )
			{
				nPort = (uint16)GammaA2I( strHost.c_str() + nPos + 1 );
				strHost.erase( nPos );
			}

			pAddress = new CAddressHttp( strKey, strHost, nPort );
			m_mapHost.Insert( *pAddress );
		}

		return pAddress;
	}

	CAddressHttp* CPackageMgr::GetHost( const string& strUrl )
	{
		CAddressHttp* pAddress = m_mapHost.UpperBound( strUrl );
		pAddress = pAddress ? pAddress->GetPre() : m_mapHost.GetLast();
		while( pAddress )
		{
			const string& strKey = pAddress->GetOrg();
			if( !strKey.compare( 0, strKey.size(), strUrl.c_str(), 0, strKey.size() ) )
				return pAddress;
			pAddress = pAddress->GetPre();
		}

		// skip http://
		const char* szUrl = strUrl.c_str();
		const char* szStartHost = szUrl + 7;
		const char* szEndHost = strchr( szStartHost, '/' );
		string strKey = string( szUrl, szEndHost + 1 - szUrl );
		string strHost = string( szStartHost, szEndHost - szStartHost );
		return AddHost( strKey, strHost );
	}

	void CPackageMgr::AddMirror( const char* szMirrorInfo )
	{
		if( szMirrorInfo == NULL || szMirrorInfo[0] == 0 )
			return;
		const char* szFind = strchr( szMirrorInfo, ',' );
		if( !szFind )
			return;

		const char* szOrgStart = szMirrorInfo;
		const char* szOrgEnd = szFind;
		const char* szMirrorStart = szFind + 1;
		const char* szMirrorEnd = szMirrorStart + strlen( szMirrorStart );

		while( IsBlank( *szOrgStart ) )
			szOrgStart++;
		while( IsBlank( *( szOrgEnd - 1 ) ) )
			szOrgEnd--;
		while( IsBlank( *szMirrorStart ) )
			szMirrorStart++;
		while( IsBlank( *( szMirrorEnd - 1 ) ) )
			szMirrorEnd--;

		string strOrg = string( szOrgStart, szOrgEnd - szOrgStart );
		string strMirror = string( szMirrorStart, szMirrorEnd - szMirrorStart );

		GammaString( strOrg );
		GammaString( strMirror );
		if( strOrg.compare( 0, 7, "http://" ) )
			return;
		if( strMirror.compare( 0, 7, "http://" ) )
			return;
		if( *strOrg.rbegin() != '/' )
			strOrg.push_back( '/' );
		if( *strMirror.rbegin() != '/' )
			strMirror.push_back( '/' );

		// skip http://
		const char* szStartHost = strMirror.c_str() + 7;
		const char* szEndHost = strchr( szStartHost, '/' );
		string strHost = string( szStartHost, szEndHost - szStartHost );
		CAddressHttp* pAddressHttp = AddHost( strOrg, strHost );
		pAddressHttp->SetMirror( strMirror );
	}

	bool CPackageMgr::FileTreeWalk( const char* szDirName, CPathMgr::FILE_PROC_UTF8 pfnFileProc, 
		void* pContext, uint32 nDepth /*= -1*/, bool bFullFilePath /*= true */ )
	{
		typedef map<string, CPathMgr::FTW_FLAG> CFileMap;
		typedef CFileMap::iterator iterator;
		CFileMap setPackFile;
		list<iterator> listPackFile;
		char szAbsolutePath[2048];

		if( !CPathMgr::IsAbsolutePath( szDirName ) )
			strcpy2array_safe( szAbsolutePath, ( m_strBaseWebPath + szDirName ).c_str() );
		else
			strcpy2array_safe( szAbsolutePath, szDirName );
		uint32 nLen = (uint32)strlen( szAbsolutePath );
		if( szAbsolutePath[ nLen - 1 ] != '/' && szAbsolutePath[ nLen - 1 ] != '\\' )
			szAbsolutePath[ nLen++ ] =  '/';		

		if( !GammaStringCompare( szAbsolutePath, 
			m_strBaseWebPath.c_str(), (uint32)m_strBaseWebPath.size() ) )
		{
			string strDirName = szAbsolutePath + m_strBaseWebPath.size();
			MapFileContextType::iterator itBegin = m_mapFileContext.upper_bound( strDirName );

			for( MapFileContextType::iterator it = itBegin;	it != m_mapFileContext.end(); ++it )
			{
				if( memcmp( it->first.c_str(), strDirName.c_str(), strDirName.size() ) )
					break;
				if( it->second.m_nIndex == INVALID_32BITID )
					continue;

				uint32 nCurDepth = 0;
				const char* szRelativePath = it->first.c_str() + strDirName.size();
				for( ; *szRelativePath && nCurDepth <= nDepth; szRelativePath++ )
					nCurDepth += *szRelativePath == '/' || *szRelativePath == '\\';
				if( nCurDepth > nDepth )
					continue;

				string strPath = bFullFilePath ? m_strBaseWebPath + it->first :
					string( it->first.c_str() + strDirName.size() );
				if( setPackFile.find( strPath ) != setPackFile.end() )
					continue;
				pair<string, CPathMgr::FTW_FLAG> pairFile( strPath, CPathMgr::eFTW_FLAG_FILE );
				listPackFile.push_back( setPackFile.insert( pairFile ).first );
			}

			uint32 nLen = (uint32)strlen( szAbsolutePath );
			if( szAbsolutePath[1] == ':' || szAbsolutePath[0] == '/' ||
				!memcmp( "external:/", szAbsolutePath, 10 ) )
			{
				struct SWalker
				{
					CPathMgr::FILE_PROC_UTF8		m_funCallback;
					void*							m_pContext;
					CFileMap*						m_setPackFile;
					list<iterator>*					m_listPackFile;

					static CPathMgr::FTW_RESULT Walk( const char* szFileName,
						CPathMgr::FTW_FLAG eFlag, SWalker* pContext )
					{
						string strPath = szFileName;
						if( pContext->m_setPackFile->find( strPath ) != pContext->m_setPackFile->end() )
							return CPathMgr::eFTW_RESULT_CONTINUNE;
						pair<string, CPathMgr::FTW_FLAG> pairFile( strPath, eFlag );
						pContext->m_listPackFile->push_back( pContext->m_setPackFile->insert( pairFile ).first );
						return CPathMgr::eFTW_RESULT_CONTINUNE;
					}

				};

				SWalker Walker = { pfnFileProc, pContext, &setPackFile, &listPackFile };
				CPathMgr::FILE_PROC_UTF8 funCallback = (CPathMgr::FILE_PROC_UTF8)&SWalker::Walk;
				_FileTreeWalk( szAbsolutePath, nLen, funCallback, &Walker, nDepth, bFullFilePath );	
			}
		}

#ifdef _ANDROID
		// android在包里找到了文件
		if( !GammaStringCompare( szAbsolutePath, "pkgroot:/", 9 ) )
		{
			gammacstring strDirName( szAbsolutePath + 9, true );
			FileListSet::iterator it = m_setPkgFileList.upper_bound( strDirName );
			for( ; it != m_setPkgFileList.end(); ++it )
			{
				if( memcmp( strDirName.c_str(), it->c_str(), strDirName.size() ) )
					break;

				uint32 nCurDepth = 0;
				const char* szRelativePath = it->c_str() + strDirName.size();
				for( ; *szRelativePath && nCurDepth <= nDepth; szRelativePath++ )
					nCurDepth += *szRelativePath == '/' || *szRelativePath == '\\';
				if( nCurDepth > nDepth )
					continue;

				string strPath = bFullFilePath ? "pkgroot:/" + string( (*it).c_str() ) :
					string( it->c_str() + strDirName.size() );
				if( setPackFile.find( strPath ) != setPackFile.end() )
					continue;
				pair<string, CPathMgr::FTW_FLAG> pairFile( strPath, CPathMgr::eFTW_FLAG_FILE );
				listPackFile.push_back( setPackFile.insert( pairFile ).first );
			}
		}
#endif

		for( list<iterator>::iterator it = listPackFile.begin(); it != listPackFile.end(); ++it )
			if( pfnFileProc( (*it)->first.c_str(), (*it)->second, pContext ) != CPathMgr::eFTW_RESULT_CONTINUNE )
				break;
		return !listPackFile.empty();
	}

	bool CPackageMgr::HasFile( const char* szFileName ) const
	{
		if( !szFileName )
			return false;

		string strFileName = GammaString( szFileName );
		if( strFileName.find( L':' ) == string::npos )
			return true;
		return m_mapFileContext.find( strFileName ) != m_mapFileContext.end();
	}

	void CPackageMgr::ReleaseCache( bool bReleaseCache )
	{
		if( m_bReleaseCache == bReleaseCache )
			return;
		m_bReleaseCache = bReleaseCache;
		if( !m_bReleaseCache )
			return;

		for( ListPackageType::iterator it = m_listPackageParts.begin(); it != m_listPackageParts.end(); it++ )
		{
			if( it->m_nPackageIndex || 
				it->m_pPackage == NULL || 
				it->m_pPackage->GetRef() )
				continue;
			SAFE_DELETE( it->m_pPackage );
		}
	}

	bool CPackageMgr::IsReleaseCacheEnable() const
	{
		return m_bReleaseCache || m_eLoadContent < eLoadContent_AllPackageUpdated;
	}

	void CPackageMgr::SetFileMgrListener( IGammaFileMgrListener* pFileMgrListener )
	{
		m_pFileMgrListener = pFileMgrListener;
	}

	void CPackageMgr::OnLoadedEnd( const char* szFileName, const tbyte* pBuffer, uint32 nSize )
	{
		if( m_eLoadContent == eLoadContent_Version )
			return OnLoadVersionFile( szFileName, pBuffer, nSize );
		if( m_eLoadContent == eLoadContent_FileList )
			return OnLoadFileList( szFileName, pBuffer, nSize );
		if( m_eLoadContent == eLoadContent_ExtractPackage )
			return OnExtractPackage( szFileName, !pBuffer );
		if( m_eLoadContent == eLoadContent_DownloadPackage )
			return OnDownloadPackage( szFileName, !pBuffer );
	}

	void CPackageMgr::OnLoadVersionFile( const char* szFileName, const tbyte* pBuffer, uint32 nSize )
	{
		CIniFile VersionInf;

		if( pBuffer == NULL || !VersionInf.Init( pBuffer, nSize ) )
		{
			CTabFile tabFile;
			m_Version = CVersion( (uint64)0 );
			if( !m_bLocalFiles )
			{
				if( m_pFileMgrListener )
					m_pFileMgrListener->OnNewCodeVersionRetrieved( NULL );
				return;
			}

			m_eLoadContent = eLoadContent_Nothing;
			if( m_bLocalFiles && m_pFileMgrListener &&
				m_pFileMgrListener->OnNewCodeVersionRetrieved( &VersionInf ) )
				m_pFileMgrListener->OnPackageInfoRetrieved( &tabFile, 0, 0, 0, 0 );
			return;
		}

		m_Version.FromString( VersionInf.GetString( "Data", "Version" ) );
		const char* szDataURL = VersionInf.GetString( "Data", "URL" ); 
		if( szDataURL && szDataURL[0] )
			m_strBaseWebPath = szDataURL;

		const char* szMirror = VersionInf.GetNextKey( "Mirror", NULL );
		while( szMirror )
		{
			AddMirror( VersionInf.GetString( "Mirror", szMirror ) );
			szMirror = VersionInf.GetNextKey( "Mirror", szMirror );
		}

		m_eLoadContent = eLoadContent_FileList;
		if( m_pFileMgrListener && 
			m_pFileMgrListener->OnNewCodeVersionRetrieved( &VersionInf ) == false )
		{
			if( m_eLoadContent == eLoadContent_FileList )
				m_eLoadContent = eLoadContent_Nothing;
			return;
		}

		char szFileListName[256];
		gammasstream name( szFileListName, 256 );
		name << "filelist";
		if( (uint64)m_Version )
			name << "_" << (string)m_Version;
		name << ( m_bLocalFiles ? ".r" : ".z" );

		CFileContext* pFileContext = &m_mapFileContext[szFileListName];
		m_listPackageParts.push_back( SPackagePart() );
		SPackagePart& PkgNode = *m_listPackageParts.rbegin();
		PkgNode.m_strFilePath = szFileListName;
		PkgNode.m_pPackage = 0;
		PkgNode.m_nOrgSize = INVALID_32BITID;
		PkgNode.m_bLoaded = false;
		PkgNode.m_bCached = false;
		PkgNode.m_nPackageIndex = 0;

		// 在filelist.z包里面只有一个文件
		pFileContext->m_nIndex = 0;
		pFileContext->m_pPackageNode = &PkgNode;

		char szFileListPath[2048];
		gammasstream url( szFileListPath, 2048 );
		url << m_strBaseWebPath.c_str() << szFileListName;
		m_pFileMgr->Load( szFileListPath, false, true, this );
		m_eLoadContent = eLoadContent_FileList;

		GammaLog << "Load filelist:" << szFileListPath << endl;
	}

	void CPackageMgr::OnLoadFileList( const char* szFileListName, const tbyte* pBuffer, uint32 nSize )
	{
		if( !pBuffer )
		{
			CTabFile tabFile;
			if( m_pFileMgrListener )
				m_pFileMgrListener->OnPackageInfoRetrieved( m_bLocalFiles ? &tabFile : NULL, 0, 0, 0, 0 );
			return;
		}

		CTabFile tabFile;
		tabFile.Init( pBuffer, nSize );
		SPackagePart* pCurNode = NULL;
		string strNodePath;
		uint32 nCurIndex = INVALID_32BITID;
		
		for( int32 i = 0; i < tabFile.GetHeight(); i++ )
		{
			int32 nOrgSize = tabFile.GetInteger( i, 1, -1 );
			if( nOrgSize >= 0 )
			{
				// 非压缩文件不需要原始大小 
				const char* szFileName = tabFile.GetString( i, 0 );
				uint32 nLen = (uint32)strlen( szFileName );
				bool bCompressed = szFileName[nLen - 2] == L'.' && szFileName[nLen - 1] == L'z';

				m_listPackageParts.push_back( SPackagePart() );
				SPackagePart& PkgPart = *m_listPackageParts.rbegin();

				PkgPart.m_strFilePath = szFileName;
				PkgPart.m_nPackSize = (uint32)tabFile.GetInteger( i, 2, 0 );
				PkgPart.m_bLoaded = false;
				PkgPart.m_bCached = false;
				PkgPart.m_pPrePart = nCurIndex ? NULL : pCurNode;
				PkgPart.m_nPackageIndex = nCurIndex ? 0 : pCurNode->m_nPackageIndex + 1;
				PkgPart.m_nOrgSize = bCompressed ? (uint32)nOrgSize : 0;

				string::size_type nPos = PkgPart.m_strFilePath.find_last_of( L'/' );
				strNodePath = PkgPart.m_strFilePath.substr( 0, nPos + 1 );
				pCurNode = &PkgPart;
				nCurIndex = 0;

				// 如果之前有一个包和现在的完全一样，只需要下载一个就可以
				if( m_mapFileContext.find( PkgPart.m_strFilePath ) != m_mapFileContext.end() )
					continue;
				CFileContext& file = m_mapFileContext[PkgPart.m_strFilePath];
				file.m_nIndex = INVALID_32BITID;
				file.m_pPackageNode = pCurNode;
				m_vecNeedDownloadPackage.push_back( pCurNode );
			}
			else
			{
				// CFileContext的m_pPackageNode指向使用最后一个PartNode
				CFileContext& file = m_mapFileContext[strNodePath + tabFile.GetString( i, 0 )];
				file.m_nIndex = nCurIndex++;
				file.m_pPackageNode = pCurNode;
			}
		}

		string strCachePath;
		char szFilePathUtf8[2048];
		const string& strLocalCachePath = m_pFileMgr->GetLocalCachePathString();

		memcpy( szFilePathUtf8, m_strBaseWebPath.c_str(), m_strBaseWebPath.size() );
		char* szFileName = szFilePathUtf8 + m_strBaseWebPath.size();
		uint32 nFileNameSize = ELEM_COUNT( szFilePathUtf8 ) - (uint32)m_strBaseWebPath.size();

		// 扫描包里已有的文件，
#ifdef _ANDROID
		FileListSet setEmpty;
		if( !m_strFileBuffer )
			ReadFileList( *( m_strFileBuffer = new CRefString ), m_setPkgFileList );
		FileListSet& setPkgFileList = m_bFromFileList ? m_setPkgFileList : setEmpty;
#else
		CRefStringPtr strBuffer = new CRefString;
		FileListSet setPkgFileList;
		ReadFileList( *strBuffer, setPkgFileList );
#endif

		set<string> setPkgCacheFiles;
		for( FileListSet::iterator it = setPkgFileList.begin(); it != setPkgFileList.end(); ++it )
		{
			const char* szFileInApk = it->c_str();
			strcpy_safe( szFileName, szFileInApk, nFileNameSize, INVALID_32BITID );
			strCachePath = m_pFileMgr->MakeCachePath( szFilePathUtf8 );
			strCachePath.erase( 0, strLocalCachePath.size() );
			uint32 nLen = (uint32)strCachePath.size();
			if( nLen > 2 && strCachePath[nLen - 2] == '.' && strCachePath[nLen - 1] == 'z' )
				strCachePath[nLen - 1] = 'r';
			setPkgCacheFiles.insert( strCachePath );
		}

		struct SCacheInfo
		{
			static CPathMgr::FTW_RESULT SearchCache( const char* szFile, CPathMgr::FTW_FLAG eFlag, void* pContext )
			{
				if( eFlag == CPathMgr::eFTW_FLAG_DIR )
					return CPathMgr::eFTW_RESULT_IGNORE;
				set<string>& setOSCacheFiles = ( (SCacheInfo*)pContext )->setOSCacheFiles;
				setOSCacheFiles.insert( szFile );
				return CPathMgr::eFTW_RESULT_CONTINUNE;
			}
			set<string> setOSCacheFiles;
		};

		SCacheInfo CacheInfo;
		CPathMgr::FileTreeWalk( strLocalCachePath.c_str(), 
			&SCacheInfo::SearchCache, &CacheInfo, INVALID_32BITID, false );

		uint32 nDownLoadSize = 0;
		uint32 nExtractSize = 0;
		m_nNeedDownloadCount = 0;
		m_nNeedExtractCount = 0;

		while( m_nNeedDownloadCount < m_vecNeedDownloadPackage.size() )
		{
			SPackagePart& PackNode = *m_vecNeedDownloadPackage[m_nNeedDownloadCount];
			strcpy_safe( szFileName, PackNode.m_strFilePath.c_str(), nFileNameSize, INVALID_32BITID );
			strCachePath = m_pFileMgr->MakeCachePath( szFilePathUtf8 );
			strCachePath.erase( 0, strLocalCachePath.size() );
			uint32 nLen = (uint32)strCachePath.size();
			if( nLen > 2 && strCachePath[nLen - 2] == '.' && strCachePath[nLen - 1] == 'z' )
				strCachePath[nLen - 1] = 'r';

			if( CacheInfo.setOSCacheFiles.find( strCachePath ) != CacheInfo.setOSCacheFiles.end() )
			{
				PackNode.m_bCached = true;
				m_vecNeedDownloadPackage.erase( m_vecNeedDownloadPackage.begin() + m_nNeedDownloadCount );
				continue;
			}

			if( setPkgCacheFiles.find( strCachePath ) != setPkgCacheFiles.end() )
			{
				m_nNeedExtractCount++;
				nExtractSize += PackNode.m_nPackSize;
				m_vecNeedExtractPackage.push_back( &PackNode );
				PackNode.m_bCached = true;
				m_vecNeedDownloadPackage.erase( m_vecNeedDownloadPackage.begin() + m_nNeedDownloadCount );
				continue;
			}

			nDownLoadSize += PackNode.m_nPackSize;
			m_nNeedDownloadCount++;
		}

		if( m_pFileMgrListener && !m_pFileMgrListener->OnPackageInfoRetrieved(
			&tabFile, m_nNeedExtractCount, nExtractSize, m_nNeedDownloadCount, nDownLoadSize ) )
			return;

		UpdateAllPackage( false, false );
	}

	void CPackageMgr::OnExtractPackage( const char* szFileName, bool bFailed )
	{
		if( bFailed )
			GammaLog << "Extract File Failed:" << szFileName << endl;

		if( szFileName && szFileName[0] )
		{
			if( m_pFileMgrListener )
				m_pFileMgrListener->OnPackageExtracted( szFileName );
			return;
		}

		if( m_nNeedDownloadCount )
		{
			m_eLoadContent = eLoadContent_DownloadPackage;
			DownLoadPackages();
		}
		else
		{
			if( m_pFileMgrListener )
				m_pFileMgrListener->OnAllPackageDownloaded();
			m_eLoadContent = eLoadContent_AllPackageUpdated;
		}
	}
	
	void CPackageMgr::OnDownloadPackage( const char* szFileName, bool bFailed )
	{
		if( m_pFileMgrListener )
			m_pFileMgrListener->OnPackageDownloaded( szFileName, bFailed );

		if( bFailed )
		{
			// 更新包下载失败，必须重新更新
			m_pFileMgr->ParallelLoad( szFileName, true, true, this );
			return;
		}

		if( --m_nNeedDownloadCount )
			return DownLoadPackages();

		if( m_pFileMgrListener )
			m_pFileMgrListener->OnAllPackageDownloaded();
		m_eLoadContent = eLoadContent_AllPackageUpdated;
	}

	void CPackageMgr::UpdateAllPackage( bool bExtract, bool bDownload )
	{
		if( !bExtract )
		{
			m_nNeedExtractCount = 0;
			m_vecNeedExtractPackage.clear();
		}

		if( !bDownload )
		{
			m_nNeedDownloadCount = 0;
			m_vecNeedDownloadPackage.clear();
		}

		if( m_nNeedExtractCount )
		{
			m_eLoadContent = eLoadContent_ExtractPackage;
			m_pFileMgr->CreateExtractThread( m_vecNeedExtractPackage );
			m_nNeedExtractCount = 0;
			m_vecNeedExtractPackage.clear();
		}
		else if( m_nNeedDownloadCount )
		{
			m_eLoadContent = eLoadContent_DownloadPackage;
			DownLoadPackages();
		}
		else if( m_pFileMgrListener )
		{
			m_pFileMgrListener->OnAllPackageDownloaded();
			m_eLoadContent = eLoadContent_AllPackageUpdated;
		}
	}

	void CPackageMgr::EnableCheckPackageWithWifiOpen( bool bEnable )
	{
		m_itPackageCheck = bEnable ? m_listPackageParts.begin() : m_listPackageParts.end();
	}

	void CPackageMgr::CheckPackage()
	{
		if( m_itPackageCheck == m_listPackageParts.end() )
			return;

		int64 nCurTime = GetGammaTime();
		if( nCurTime - m_nPreCheckTime > 10000 )
		{
			m_nPreCheckTime = (uint32)nCurTime;
			m_bWifiConnected = IsWifiConnect();
		}

		if( !m_bWifiConnected )
			return;

		if( m_nPreDownloadTime == INVALID_32BITID )
			m_nPreDownloadTime = (uint32)nCurTime;
		if( m_nPreDownloadTime + 500 > nCurTime )
			return;
		m_nPreDownloadTime = INVALID_32BITID;

		while( m_itPackageCheck != m_listPackageParts.end() )
		{
			if( m_itPackageCheck->m_bLoaded ||
				m_itPackageCheck->m_bCached )
			{
				++m_itPackageCheck;
				continue;
			}

			SPackagePart* pPkgPart = &*m_itPackageCheck;
			while( pPkgPart->m_nPackageIndex )
				pPkgPart = pPkgPart->m_pPrePart;
			CPackage* pPackage = pPkgPart->m_pPackage;
			if( pPackage && pPackage->GetLoadState() != eLoadState_NotLoad )
			{
				++m_itPackageCheck;
				continue;
			}

			break;
		}

		if( m_itPackageCheck == m_listPackageParts.end() )
			return;

		string strPath = m_strBaseWebPath + m_itPackageCheck->m_strFilePath;
		m_pFileMgr->ParallelLoad( strPath.c_str(), true, true, NULL );
		m_itPackageCheck->m_bCached = true;
	}

	void CPackageMgr::DownLoadPackages()
	{
		int32 nCount = (int32)m_vecNeedDownloadPackage.size();
		for( int32 i = nCount - 1, j = 0; i >= 0 && j < 20; i--, j++ )
		{
			SPackagePart& PackNode = *m_vecNeedDownloadPackage[i];
			string strPath = m_strBaseWebPath + PackNode.m_strFilePath;
			m_pFileMgr->ParallelLoad( strPath.c_str(), true, true, this );
			m_vecNeedDownloadPackage.erase( m_vecNeedDownloadPackage.begin() + i );
		}
	}

	void CPackageMgr::DumpCurrentPackage()
	{
		uint32 nTotalSize = 0;
		for( ListPackageType::iterator it = m_listPackageParts.begin(); it != m_listPackageParts.end(); it++ )
		{
			if( it->m_nPackageIndex != 0 || it->m_pPackage == NULL )
				continue;
			nTotalSize += it->m_nOrgSize;
			GammaLog << it->m_pPackage->GetRef() << "\t" << it->m_strFilePath << endl;
		}
		GammaLog << "Total Package Size:" << nTotalSize << endl;
	}

	void CPackageMgr::ReadFileList( string& strBuffer, CPackageMgr::FileListSet& setPkgFileList ) const
	{
		strBuffer.clear();
		string strTemp;
		vector<uint32> vecOffset;

		if( ReadResourcePackageFile( strTemp, "version.inf" ) )
		{
			CIniFile VersionInf;
			VersionInf.Init( (const tbyte*)strTemp.c_str(), (uint32)strTemp.size() );
			const char* szVersion = VersionInf.GetString( "Data", "Version" );
			char szFileListName[256];
			gammasstream( szFileListName, 256 ) << "filelist_" << szVersion << ".r";

			if( ReadResourcePackageFile( strTemp, szFileListName ) )
			{
				CTabFile tabFile;
				tabFile.Init( (const tbyte*)strTemp.c_str() + 4, (uint32)strTemp.size() - 4 );
				strTemp.clear();

				for( int32 i = 0; i < tabFile.GetHeight(); i++ )
				{
					if( tabFile.GetInteger( i, 1, -1 ) < 0 )
						continue;
					const char* szFileName = tabFile.GetString( i, 0 );
					vecOffset.push_back( (uint32)strTemp.size() );
					strTemp.append( szFileName );
					strTemp.push_back( 0 );				
				}
			}
		}

#ifdef _ANDROID
		if( vecOffset.empty() )
		{
			strTemp.clear();
			m_bFromFileList = false;
			const char* szPackagePath = CAndroidApp::GetInstance().GetPackagePath();
			unzFile hZip = szPackagePath ? unzOpen( szPackagePath ) : NULL;

			// 定位到第一个文件
			if( hZip && UNZ_OK == unzGoToFirstFile( hZip ) )
			{
				do
				{
					// 打开包内当前的文件
					if( UNZ_OK != unzOpenCurrentFile( hZip ) )
					{
						unzClose( hZip );
						break;
					}

					unz_file_info unzFile = { 0 };
					char strInnerName[ 1024 ] = { 0 };   // 被打开的文件名
					char strComment[ 256 ]  = { 0 };    // 注释
					// 获取被打开的文件信息
					unzGetCurrentFileInfo( hZip, &unzFile, strInnerName, 
						sizeof(strInnerName), NULL, 0, strComment, sizeof(strComment) );
					static const char* szAssets = "assets/";
					static const uint32 nLen = strlen( szAssets );
					if( memcmp( strInnerName, szAssets, nLen ) )
						continue;

					vecOffset.push_back( strTemp.size() );
					strTemp.append( strInnerName + nLen );
					strTemp.push_back( 0 );				
				}
				while( UNZ_OK == unzGoToNextFile( hZip ) );
				unzClose( hZip );
			}
		}
#endif

		if( vecOffset.empty() )
			return;
		strBuffer.resize( strTemp.size() );
		memcpy( &strBuffer[0], &strTemp[0], strTemp.size() );

		for( size_t i = 0; i < vecOffset.size(); i++ )
		{
			gammacstring strName( strBuffer.c_str() + vecOffset[i], true );
			setPkgFileList.insert( strName );
		}	
	}

}
