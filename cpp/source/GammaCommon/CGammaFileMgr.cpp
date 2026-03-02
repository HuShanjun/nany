
#include "GammaCommon/CPkgFile.h"
#include "GammaCommon/GammaTime.h"
#include "GammaCommon/GammaCodeCvs.h"
#include "GammaCommon/GammaProfile.h"
#include "GammaCommon/GammaFileMap.h"
#include "GammaCommon/GammaMath.h"
#include "GammaCommon/CGammaRand.h"
#include "GammaCommon/CPathMgr.h"
#include "CReadFileThread.h"
#include "CGammaFileMgr.h"
#include "CResObject.h"
#include "CPackageMgr.h"

#ifdef _IOS
#include "IOS.h"
#endif

namespace Gamma
{

	//===============================================================
	// 文件管理器
	//===============================================================
	GAMMA_COMMON_API IGammaFileMgr& GetGammaFileMgr()
	{
		return CGammaFileMgr::Instance();
	}

	CGammaFileMgr& CGammaFileMgr::Instance()
	{
		static CGammaFileMgr s_instance;
		return s_instance;
	}

	CGammaFileMgr::CGammaFileMgr()
		: m_bQuit( false )
		, m_FilePackageManager( this )
		, m_hLockLoaded( GammaCreateLock() )
		, m_hLockHttpWaiting( GammaCreateLock() )
		, m_hLockLocalWaiting( GammaCreateLock() )
		, m_hSemLocalWaiting( GammaCreateSemaphore() )
		, m_hSemHttpWaiting( GammaCreateSemaphore() )
		, m_bLoadPackageFirst( true )
		, m_nCurWaitingSerialID( 0 )
		, m_bEnableFileProgress( false )
		, m_pExtractThread( NULL )
	{
#ifdef _WIN32
		WORD wVersion;
		WSADATA wsaData;

		wVersion = MAKEWORD( 2, 2 );
		int32 nResult = WSAStartup( wVersion, &wsaData );
		if( nResult )
		{
			std::stringstream strm;
			strm << "WSAStartup failed with error code:" << nResult << std::ends;
			GammaThrow( strm.str() );
		}
#endif
		m_vecReadThreadList[0] = new CReadFileThread( m_listWaiting[eLC_Local], 
			m_hSemLocalWaiting, m_hLockLocalWaiting, m_listLoaded, m_hLockLoaded );

		for( size_t i = 1; i < READ_THREAD_COUNT; i++  )
			m_vecReadThreadList[i] = new CReadFileThread( m_listWaiting[eLC_Http],
			m_hSemHttpWaiting, m_hLockHttpWaiting, m_listLoaded, m_hLockLoaded );
	}

	CGammaFileMgr::~CGammaFileMgr()
	{
		if( !m_bQuit )
			Exit();

		GammaDestroyLock( m_hLockLoaded );
		GammaDestroyLock( m_hLockHttpWaiting );
		GammaDestroyLock( m_hLockLocalWaiting );
		GammaDestroySemaphore( m_hSemHttpWaiting );
		GammaDestroySemaphore( m_hSemLocalWaiting );

#ifdef _WIN32
		int32 nResult = WSACleanup();
		if( nResult )
		{
			std::stringstream strm;
			strm << "WSACleanup failed with error code:" << nResult << std::ends;
			GammaThrow( strm.str() );
		}
#endif
		GammaAst( !m_arrSerialWaitingObject.GetFirst() );
		GammaAst( !m_arrParallelFinishObject.GetFirst() );
	}

	CPackageMgr& CGammaFileMgr::GetFilePackageManager()
	{
		return m_FilePackageManager;
	}

	void CGammaFileMgr::SetBaseWebPath( const char* szPath, const char* szParam, bool bHttpAsLocal )
	{
		m_FilePackageManager.SetBaseWebPath( szPath, szParam, bHttpAsLocal );
		CLocalizeDirMap mapLocalizeDir;
		for( CLocalizeDirMap::iterator iter = m_mapLocalizeDir.begin(); iter != m_mapLocalizeDir.end(); ++iter )
		{
			char szAbsoluteOrgPath[2048];
			char szAbsoluteLocalizePath[2048];
			m_FilePackageManager.ConvertToAbsolutePath( iter->first.c_str(), szAbsoluteOrgPath );
			m_FilePackageManager.ConvertToAbsolutePath( iter->second.c_str(), szAbsoluteLocalizePath );
			mapLocalizeDir[ GammaString( szAbsoluteOrgPath ) ] = szAbsoluteLocalizePath;
		}
		m_mapLocalizeDir = mapLocalizeDir;
	}

	const char* CGammaFileMgr::GetBaseWebPath() const
	{
		return m_FilePackageManager.GetBaseWebPath();
	}

	const CVersion& CGammaFileMgr::GetVersion() const
	{
		return m_FilePackageManager.GetVersion();
	}

	void CGammaFileMgr::DumpCurrentPackage()
	{
		m_FilePackageManager.DumpCurrentPackage();
	}

	void CGammaFileMgr::UpdateAllPackage( bool bExtract, bool bDownload )
	{
		m_FilePackageManager.UpdateAllPackage( bExtract, bDownload );
	}

	void CGammaFileMgr::EnableCheckPackageWithWifiOpen( bool bEnable )
	{
		m_FilePackageManager.EnableCheckPackageWithWifiOpen( bEnable );
	}

	void CGammaFileMgr::ReleaseCache( bool bReleaseCache )
	{
		m_FilePackageManager.ReleaseCache( bReleaseCache );
	}

	CPackage* CGammaFileMgr::CreatePackage( const char* szPathName )
	{
		if( m_bQuit || szPathName == NULL || szPathName[0] == 0 )
			return NULL;

		if( strnicmp( szPathName, "memory:", 7 ) == 0 )
			return NULL;

		size_t nPathLen = strlen( szPathName );
		if( szPathName[nPathLen - 1] == '/' || szPathName[nPathLen - 1] == '\\' )
			return NULL;

		char szAbsPathName[2048];
		m_FilePackageManager.ConvertToAbsolutePath( szPathName, szAbsPathName );
		return m_FilePackageManager.CreatePackage( szAbsPathName );
	}

	bool CGammaFileMgr::GetCacheInfo( const char* szPathName, char* szCacheName, size_t nMaxCount, uint32& offset, uint32& size )
	{
		CPackage* pPackage = CreatePackage( szPathName );
		if( !pPackage )
			return false;

		if( pPackage->GetLoadState() != eLoadState_Succeeded )
		{
			SAFE_RELEASE( pPackage );
			return false;
		}

		const char* szBufferStart = pPackage->GetFileBuffer();
		if( szBufferStart == NULL )
		{
			SAFE_RELEASE( pPackage );
			return false;
		}

		SFileBuffer Buffer = pPackage->GetFileBuffer( szPathName );
		if( !Buffer.m_BufferPtr )
		{
			SAFE_RELEASE( pPackage );
			return false;
		}

		offset = Buffer.m_nOffset;
		size = Buffer.m_nBufferSize;
		SAFE_RELEASE( pPackage );
		return true;
	}

	void CGammaFileMgr::CheckFileCacheState( const char* szFileName, bool& bInCache, bool& bInPackage )
	{
		CPackage* pPackage = CreatePackage( szFileName );
		bInCache = bInPackage = false;
		if( !pPackage )
			return;

		std::string strUrl;
		bInCache = true;
		bInPackage = true;

		const std::string& strBaseWebPath = m_FilePackageManager.GetBaseWebPathString();
		SPackagePart* pPkgNode = pPackage->GetLastPkgNode();
		for( int32 i = pPkgNode->m_nPackageIndex; i >= 0; --i )
		{
			const std::string& strName = pPkgNode->m_strFilePath;
			const char* szUrl = strName.c_str();
			if( !CPathMgr::IsAbsolutePath( szUrl ) )
			{
				strUrl = strBaseWebPath;
				strUrl.append( strName );
				szUrl = strUrl.c_str();
			}

			if( !CPathMgr::IsFileExist( MakeCachePath( szUrl ).c_str() ) )
				bInCache = false;
			if( !m_FilePackageManager.IsFileInCurrentPackage( szUrl + strBaseWebPath.size() ) )
				bInPackage = false;
			pPkgNode = pPkgNode->m_pPrePart;
		}
	}

	bool CGammaFileMgr::IsInFileList( const char* szFileName, char* szAbsolutePath, size_t nMaxSize )
	{
		char szPath[2048];
		// 检查在包里是否存在
		m_FilePackageManager.ConvertToAbsolutePath( szFileName, szPath );
		if( szAbsolutePath )
			strcpy_safe( szAbsolutePath, szPath, (uint32)nMaxSize, INVALID_32BITID );

		const char* szIndexName = m_FilePackageManager.RevertToShortPath( szPath );
		if( !szIndexName )
			return false;
		CFileContext* pFileContext = m_FilePackageManager.GetFileContext( szIndexName );
		return pFileContext && pFileContext->IsInFileList();
	}

	void CGammaFileMgr::OnResObjectLoadedEnd( CResObject* pResObject )
	{
		m_listNeedProgress.erase( pResObject );

		if( pResObject->GetSerialID() == INVALID_32BITID )
		{
			pResObject->CQueueBelongedNode::Remove();
			m_arrParallelFinishObject.PushBack( *pResObject );
		}

		// 如果是m_FilePackageManager的回调要立即处理
		CMapListener::iterator it = pResObject->GetListener();
		if( it != m_mapListener.end() && it->first == &m_FilePackageManager )
		{
			ProcessObject( pResObject );
		}
		else if( pResObject->GetLoadState() == eLoadState_Failed )
		{
			SFileFailed& FileFailed = m_mapExcludeFile[*pResObject->GetPathName()]; 
			FileFailed.m_nFailedTime = (uint32)( GetNatureTime()/1000 );
			FileFailed.m_nFailedCount++;
		}
	}

	void CGammaFileMgr::ApplyAllLoaded( uint32 nIdleTime )
	{
		STATCK_LOG( this );
		uint32 nWaitingObjectCount = m_nWaitingObjectCount;

		CTimer timer;
		while( timer.GetElapse() < nIdleTime )
		{
			PROCESS_LOG;
			// 串行队列可能只有部分加载，并且要保证回调顺序
			CQueueBelongedNode* pSerialNode = m_arrSerialWaitingObject.GetFirst();
			if( pSerialNode && static_cast<CResObject*>( pSerialNode )->GetLoadState() == eLoadState_Loading )
				pSerialNode = NULL;
			else if( pSerialNode )
				ProcessObject( static_cast<CResObject*>( pSerialNode ) );

			PROCESS_LOG;
			// 并行队列一定是全部加载了的
			CQueueBelongedNode* pParalleNode = m_arrParallelFinishObject.GetFirst();
			if( pParalleNode )
				ProcessObject( static_cast<CResObject*>( pParalleNode ) );

			if( !pSerialNode && !pParalleNode )
				break;
		}

		PROCESS_LOG;
		if( nWaitingObjectCount && m_nWaitingObjectCount == 0 && m_FilePackageManager.GetFileMgrListener() )
			m_FilePackageManager.GetFileMgrListener()->OnAllLoadedEnd();
	}

	// 处理资源
	void CGammaFileMgr::ProcessObject( CResObject* pResObject )
	{
		STATCK_LOG( this );
		IGammaFileMgrListener* pMgrListener = m_FilePackageManager.GetFileMgrListener();
		pResObject->CQueueBelongedNode::Remove();
		CRefStringPtr strPathName = pResObject->GetPathName();
		SFileBuffer FileBuffer = pResObject->GetBuffer();
		ELoadState eLoadeState = pResObject->GetLoadState();
		const tbyte* szFileBuffer = NULL;
		if( eLoadeState == eLoadState_Succeeded )
		{
            CRefStringPtr& BufferPtr = FileBuffer.m_BufferPtr;
			const char* pBuffer = BufferPtr ? FileBuffer.m_BufferPtr->c_str() : NULL;
			szFileBuffer = (const tbyte*)( pBuffer ? pBuffer + FileBuffer.m_nOffset : "" );
		}

		PROCESS_LOG;
		if( pMgrListener )
			pMgrListener->OnFileLoaded( strPathName->c_str(), szFileBuffer!= NULL );

		// 注：
		// 如果pPackage需要保留引用直到OnLoadedEnd之后，
		// 否则OnLoadedEnd再次加载同一个文件pPackage要重新new，或者在GetCacheInfo时失败( CMusicSL.cpp )
		CPackage* pPackage = pResObject->GetFilePackage();
		if( pPackage )
			pPackage->AddRef();

		PROCESS_LOG;
		CMapListener::iterator it = pResObject->GetListener();
		IResListener* pListener = it != m_mapListener.end() ? it->first : NULL;
		RemoveObject( pResObject );

		PROCESS_LOG;
		if( pListener )
			pListener->OnLoadedEnd( strPathName->c_str(), szFileBuffer, FileBuffer.m_nBufferSize );

		if( pMgrListener )
			pMgrListener->PostFileLoaded( strPathName->c_str(), szFileBuffer!= NULL );

		SAFE_RELEASE( pPackage );
		PROCESS_LOG;
	}

	bool CGammaFileMgr::AddObject( const char* szPathName, bool bCache, 
		bool bSerial, ELoadPriority eLoadPriority, IResListener* pListener )
	{
		if( m_bQuit || szPathName == NULL || szPathName[0] == 0 )
			return false;

		// 内存文件直接处理
		if( strnicmp( szPathName, "memory:", 7 ) == 0 )
		{
			if( pListener == NULL )
				return true;
			if( szPathName[7] != '/' && szPathName[7] != '\\' )
				return false;
			const char* szSize = GetFileNameFromPath( szPathName );
			if( szSize == NULL || szSize == szPathName + 8 )
				return false;
			uint64 nMemoryAddr = GammaA2I64( szPathName + 8 );
			uint32 nMemorySize = GammaA2I( szSize );
			pListener->OnLoadedEnd( szPathName, (const tbyte*)(ptrdiff_t)nMemoryAddr, nMemorySize );
			return true;
		}

		// 不能加载目录
		size_t nPathLen = strlen( szPathName );
		if( (szPathName[nPathLen - 1] == '/' || szPathName[nPathLen - 1] == '\\') && GammaStringCompare( szPathName, "http://", 7 ) )
			return false;

		// 同步加载，支持本地和网络读取，优先本地
		if( eLoadPriority == eLP_Sync )
		{
			char szAbsPathName[2048];
			CPathMgr::ToAbsolutePath( szPathName, szAbsPathName, ELEM_COUNT( szAbsPathName ) );
			GammaString( szAbsPathName );

			// 非网络同步读取
			if( GammaStringCompare( szAbsPathName, "http://", 7 ) )
			{
				CFileReader TempReader( NULL, szAbsPathName, 0, NULL, bCache );
				if( TempReader.Read( m_strTempBuffer ) == eReadOK )
				{
					if( pListener )
					{
						const CRefStringPtr& ptrBuffer = TempReader.GetFileBuffer();
						const tbyte* szBuffer = (const tbyte*)( ptrBuffer->empty() ? "" : ptrBuffer->c_str() );
						pListener->OnLoadedEnd( szAbsPathName, szBuffer, (uint32)ptrBuffer->size() );
					}
					return true;
				}
			}

			// 同步读取，支持本地和网络读取
			m_FilePackageManager.ConvertToAbsolutePath( szPathName, szAbsPathName );
			CPackage* pPackage = m_FilePackageManager.CreatePackage( szAbsPathName );
			if( !pPackage )
				return false; 

			if( pPackage->GetLoadState() != eLoadState_Failed ||
				pPackage->GetLoadState() != eLoadState_Succeeded )
			{
				CReaderList& listReader = pPackage->GetReaderList();
				if( listReader.empty() )
				{
					pPackage->CreateReaders( bCache );
					for( uint32 i = 0; i < listReader.size(); i++ )
					{
						listReader[i]->SetLoadState( eLoadState_Failed );
						if( listReader[i]->Read( m_strTempBuffer ) != eReadOK )
							continue;
						listReader[i]->SetLoadState( eLoadState_Succeeded );
					}
				}

				bool bLoading = true;
				while( listReader.size() && bLoading )
				{ 
					bLoading = false;
					for( uint32 i = 0; i < listReader.size(); i++ )
					{
						if( listReader[i]->GetLoadState() >= eLoadState_Failed )
							continue;
						bLoading = true;
					}

					if( !bLoading )
						break;
					GammaSleep( 300 );
				}

				GammaLock( m_hLockLoaded );
				for( uint32 i = 0; i < listReader.size(); i++ )
					listReader[i]->Remove();
				GammaUnlock( m_hLockLoaded );	
				pPackage->OnLoaded( INVALID_32BITID );
			}

			if( pPackage->GetLoadState() == eLoadState_Failed )
			{
				SAFE_RELEASE( pPackage );
				return false;
			}

			if( pListener )
			{
				SFileBuffer FileBuffer = pPackage->GetFileBuffer( szAbsPathName );
				const tbyte* szFileBuffer =(const tbyte*)FileBuffer.m_BufferPtr->c_str();
				szFileBuffer += szFileBuffer ? FileBuffer.m_nOffset : 0;
				pListener->OnLoadedEnd( szAbsPathName, szFileBuffer, FileBuffer.m_nBufferSize );
			}
			SAFE_RELEASE( pPackage );
			return true;
		}

		// 异步加载，支持本地和网络读取
		char szAbsPathName[2048];
		m_FilePackageManager.ConvertToAbsolutePath( szPathName, szAbsPathName );
		CPackage* pPackage = m_FilePackageManager.CreatePackage( szAbsPathName );
		if( !pPackage )
			return false; 

		// 没有监听器不需要创建CResObject
		if( pListener )
		{
			// 允许m_FilePackageManager重新加载
			if( pListener == &m_FilePackageManager && pPackage->GetLoadState() == eLoadState_Failed )
				pPackage->MarkNotLoad();

			GammaString( szAbsPathName );
			uint32 nSerialID = bSerial ? m_nCurWaitingSerialID++ : INVALID_32BITID;
			CResObject* pResObject = new CResObject( nSerialID, szAbsPathName, pPackage );
			pResObject->SetListener( m_mapListener.insert( std::make_pair( pListener, pResObject ) ) );
			if( m_bEnableFileProgress )
				m_listNeedProgress.insert( pResObject );
			m_nWaitingObjectCount++;
			pPackage->AddResObject( *pResObject );

			if( bSerial )
				m_arrSerialWaitingObject.PushBack( *pResObject );
			else if( pResObject->GetLoadState() <= eLoadState_Loading )
				m_arrParallelWaitingObject.PushBack( *pResObject );
			else
				m_arrParallelFinishObject.PushBack( *pResObject );
		}

		// 异步装载
		// 只有明确处于加载完毕状态的数据才能返回，
		// 因为已经在上一行代码中加入了m_arrParallelFinishObject队列
		if( pPackage->GetLoadState() == eLoadState_Succeeded ||
			pPackage->GetLoadState() == eLoadState_Failed )
		{
			SAFE_RELEASE( pPackage );
			return true;
		}

		// 处于loading或者notload状态的一律重新加载。
		// 因为有可能调用者在处于loading状态时取消了所有监听，
		// 导致数据无法正确传递给调用者，以致调用者一直处于loading状态
		// 而底层却因为加载完成删除了相关的SFileReader
		// 因此loading状态不能作为数据状态的判断标准，必须重新加载
		pPackage->MarkLoading();
		if( !pPackage->GetReaderList().empty() )
		{
			SAFE_RELEASE( pPackage );
			return true;
		}

		CReaderList& listReader = pPackage->CreateReaders( bCache );
		// 曾经加载成功的文件会在并行加载中被优先加载
		if( !bSerial && pPackage->HasLoadedSucceed() )
			eLoadPriority = eLP_High;

		// 是否网络资源
		bool bHttpRes = pPackage->IsHttpRes();
		SPackagePart* pPkgNode = pPackage->GetLastPkgNode();
		for( int32 i = pPkgNode->m_nPackageIndex; i >= 0; --i )
		{
#ifdef _IOS
			if( CIOSApp::GetInstance().ReadSystemFile( listReader[0],
				m_hLockLoaded, &m_listLoaded.m_listFinish[!bSerial] ) )
			{
				GammaAst( i == 0 );
				break;
			}
#endif
			bool bInCached = pPkgNode->m_bCached || pPkgNode->m_bLoaded;
			bool bLoadFromHttp = bHttpRes && !bInCached;
			HLOCK hLock = bLoadFromHttp ? m_hLockHttpWaiting : m_hLockLocalWaiting;
			HSEMAPHORE hSemWait = bLoadFromHttp ? m_hSemHttpWaiting : m_hSemLocalWaiting;
			ELocation eLocation = bLoadFromHttp ? eLC_Http : eLC_Local;
			ELoadType eLoadType = bSerial ? eLT_Serial : ( eLoadPriority == eLP_High ? eLT_High : eLT_Low );
			CAsynReadList& listWaiting = m_listWaiting[eLocation].m_listReader[eLoadType];

			GammaLock( hLock );
			listWaiting.PushBack( *listReader[i] );
			GammaUnlock( hLock );
			GammaPutSemaphore( hSemWait );
			pPkgNode = pPkgNode->m_pPrePart;
		}

		SAFE_RELEASE( pPackage );
		return true;
	}

	void CGammaFileMgr::RemoveObject( CResObject* pResObject )
	{
		m_listNeedProgress.erase( pResObject );
		CMapListener::iterator it = pResObject->GetListener();
		if( it != m_mapListener.end() )
			m_mapListener.erase( it );
		delete pResObject;
		m_nWaitingObjectCount--;
	}

	void CGammaFileMgr::RemoveListener( IResListener* pListener )
	{
		CMapListener::iterator it = m_mapListener.lower_bound( pListener );
		while( it != m_mapListener.end() && it->first == pListener )
			RemoveObject( ( it++ )->second );
	}

	void CGammaFileMgr::CreateExtractThread( const CPackageInfoList& listPackage )
	{
		std::vector<const char*> listPackageName( listPackage.size() );
		for( uint32 i = 0; i < listPackageName.size(); i++ )
			listPackageName[i] = listPackage[i]->m_strFilePath.c_str();
		m_pExtractThread = new CExtractThread( listPackageName, m_listLoaded, m_hLockLoaded );
	}

	uint32 CGammaFileMgr::LoadDir( const char* szDirName, bool bCache, 
		bool bSerial, ELoadPriority eLoadPriority, IResListener* pListener )
	{
		struct SDirInfo
		{
			static CPathMgr::FTW_RESULT SearchDir( const char* szFile, CPathMgr::FTW_FLAG eFlag, void* pContext )
			{
				if( eFlag == CPathMgr::eFTW_FLAG_DIR )
					return CPathMgr::eFTW_RESULT_CONTINUNE;
				SDirInfo& Info = *(SDirInfo*)pContext;
				if( !Info.m_bSerial )
					Info.m_pFileMgr->ParallelLoad( szFile, Info.m_eLoadPriority == eLP_High, Info.m_bCache, Info.m_pListener );
				else if( Info.m_eLoadPriority == eLP_Sync )
					Info.m_pFileMgr->SyncLoad( szFile, Info.m_bCache, Info.m_pListener );
				else
					Info.m_pFileMgr->Load( szFile, Info.m_eLoadPriority == eLP_High, Info.m_bCache, Info.m_pListener );
				Info.m_nCount++;
				return CPathMgr::eFTW_RESULT_CONTINUNE;
			}

			CGammaFileMgr*	m_pFileMgr;
			ELoadPriority	m_eLoadPriority;
			bool			m_bCache;
			bool			m_bSerial;
			IResListener*	m_pListener;
			uint32			m_nCount;
		};

		SDirInfo DirInfo = { this, eLoadPriority, bCache, bSerial, pListener, 0 };
		CPathMgr::FileTreeWalk( szDirName, &SDirInfo::SearchDir, &DirInfo, INVALID_32BITID, true );
		return DirInfo.m_nCount;
	}

	uint32 CGammaFileMgr::SyncLoad( const char* szPathName, bool bSave2Cache, IResListener* pListener )
	{
		if( *(szPathName + strlen( szPathName ) - 1 ) == L'/' )
			return LoadDir( szPathName, bSave2Cache, true, eLP_Sync, pListener );

		++m_objectCountInHistory;
		bool bSuceeded = AddObject( szPathName, bSave2Cache, false, eLP_Sync, pListener );
		return bSuceeded;
	}

	uint32 CGammaFileMgr::ParallelLoad( const char* szPathName, bool bHighPriority, bool bSave2Cache, IResListener* pListener )
	{
		if( *(szPathName + strlen( szPathName ) - 1 ) == L'/' )
			return LoadDir( szPathName, bSave2Cache, false, bHighPriority ? eLP_High : eLP_Low, pListener );

		++m_objectCountInHistory;
		return AddObject( szPathName, bSave2Cache, false, bHighPriority ? eLP_High : eLP_Low, pListener );
	}

	uint32 CGammaFileMgr::Load( const char* szPathName, bool bHighPriority, bool bSave2Cache, IResListener* pListener )
	{
		if( szPathName && *(szPathName + strlen( szPathName ) - 1 ) == L'/' && GammaStringCompare( szPathName, "http://", 7 ) )
			return LoadDir( szPathName, bSave2Cache, true, bHighPriority ? eLP_High : eLP_Low, pListener );

		++m_objectCountInHistory;	
		return AddObject( szPathName, bSave2Cache, true, bHighPriority ? eLP_High : eLP_Low, pListener );
	}

	void CGammaFileMgr::EnableFileProgress( bool bEnable )
	{
		m_bEnableFileProgress = bEnable;
	}

	uint32 CGammaFileMgr::GetResWaitingCount()
	{
		return m_nWaitingObjectCount;
	}

	uint32 CGammaFileMgr::GetObjectCountInHistory()
	{
		return m_objectCountInHistory;
	}

	void CGammaFileMgr::SetListener( IGammaFileMgrListener* pListener )
	{
		m_FilePackageManager.SetFileMgrListener( pListener );
	}

	void CGammaFileMgr::Exit()
	{
		m_bQuit = true;
		SAFE_DELETE( m_pExtractThread );
		for( uint32 i = 0; i < READ_THREAD_COUNT; ++i )
			GammaPutSemaphore( i ? m_hSemHttpWaiting : m_hSemLocalWaiting );
		for( uint32 i = 0; i < READ_THREAD_COUNT; ++i )
			SAFE_DELETE( m_vecReadThreadList[i] );
		Flush( INVALID_32BITID );
	}

	///< 将读好的文件回调出去
	void CGammaFileMgr::Flush( uint32 nElapseTime )
	{
		STATCK_LOG( this );
		CTimer TimeCheck;
		// 使用一半的时间来应用已加载资源
		ApplyAllLoaded( nElapseTime/2 );

		PROCESS_LOG;
		while( TimeCheck.GetElapse() < nElapseTime )
		{
			GammaLock( m_hLockLoaded );
			uint32 nIndex = m_listLoaded.m_bReadParallel;
			m_listLoaded.m_bReadParallel = !m_listLoaded.m_bReadParallel;
			CFileReader* pReader = m_listLoaded.m_listFinish[nIndex].GetFirst();
			if( !pReader )
			{
				nIndex = m_listLoaded.m_bReadParallel;
				m_listLoaded.m_bReadParallel = !m_listLoaded.m_bReadParallel;
				pReader = m_listLoaded.m_listFinish[nIndex].GetFirst();
			}

			GammaUnlock( m_hLockLoaded );

			if( !pReader )
				break;

			uint64 nElapse = TimeCheck.GetElapse();
			if( nElapse >= nElapseTime )
				break;

			GammaLock( m_hLockLoaded );
			pReader->Remove();
			GammaUnlock( m_hLockLoaded );

			// 只有一种情况，就是解压完毕的终止标志
			if( pReader->GetPackage() == NULL )
			{
				const char* szFileName = pReader->GetFileName().c_str();
				m_FilePackageManager.OnLoadedEnd( szFileName, (const tbyte*)"", 1 );
				delete pReader;
				continue;
			}

			if( pReader->Flush( uint32( nElapseTime - nElapse ) ) )
				continue;

			// Flush超时，需要加回原来的队列
			GammaLock( m_hLockLoaded );
			m_listLoaded.m_listFinish[nIndex].PushFront( *pReader );
			GammaUnlock( m_hLockLoaded );
		}

		PROCESS_LOG;
		uint64 nElapse = TimeCheck.GetElapse();
		if( TimeCheck.GetElapse() < nElapseTime )
			ApplyAllLoaded( (uint32)( nElapseTime - nElapse ) );

		PROCESS_LOG;
		IGammaFileMgrListener* pListener = m_FilePackageManager.GetFileMgrListener();
		if( pListener )
		{
			for( CListNeedProgress::iterator it = m_listNeedProgress.begin(); it != m_listNeedProgress.end(); ++it )
			{
				CResObject* pResObject = *it;
				CPackage* pPackage = pResObject->GetFilePackage();
				CReaderList& listReader = pPackage->GetReaderList();
				GammaAst( !listReader.empty() );
				uint32 nTotalSize = 0;
				uint32 nCurSize = 0;
				for( uint32 i = 0; i < listReader.size(); i++ )
				{
					nTotalSize += listReader[i]->GetTotalSize();
					nCurSize += listReader[i]->GetCurSize();
				}
				pListener->OnFileProgress( pResObject->GetPathName()->c_str(), nCurSize, nTotalSize );
			}
		}

		for( uint32 i = 0; i < ELEM_COUNT( m_listWaiting ); i++ )
		{
			for( uint32 j = 0; j < ELEM_COUNT( m_listWaiting[i].m_listReader ); j++ )
			{
				if( !m_listWaiting[i].m_listReader[j].IsEmpty() )
					return;
			}
		}

		m_FilePackageManager.CheckPackage();
	}

	void CGammaFileMgr::SetLocalCachePath( const char* szLocalCachePath )
	{
		if( szLocalCachePath == NULL )
			m_strLocalCachePath.clear();
		else if( szLocalCachePath[0] == 0 )
			m_strLocalCachePath = "./";
		else
			m_strLocalCachePath = szLocalCachePath;
		GammaString( &m_strLocalCachePath[0] );
		CPathMgr::MakeDirectory( m_strLocalCachePath.c_str() );
	}

	std::string CGammaFileMgr::MakeCachePath( const char* szDataPath ) const
	{
		if( m_strLocalCachePath.empty() )
			return m_strLocalCachePath;

		std::string strFileName = m_strLocalCachePath;
		static unsigned char hexchars[] = "0123456789ABCDEF";

		while( *szDataPath )
		{
			uint8 c = (uint8)*szDataPath++;

			if( IsNumber( c ) || IsLetter( c ) || c == '.' || c == '_' )
				strFileName.push_back( c );
			else
			{
				strFileName.push_back( '%' );
				strFileName.push_back( hexchars[ c & 15 ] );
				strFileName.push_back( hexchars[ c >> 4 ] );
			}
		}

		// 如果是压缩文件则保存为原始文件
		uint32 nLen = (uint32)strFileName.size();
		if( nLen < 2 )
			return strFileName;
		if( strFileName[nLen-1] == 'z' && strFileName[nLen-2] == '.' )
			strFileName[nLen-1] = 'r';
		return strFileName;
	}

	std::string CGammaFileMgr::MakeLocalizePath( const char* szDataPath ) const
	{
		char szAbsoluteDataPath[2048];
		m_FilePackageManager.ConvertToAbsolutePath( szDataPath, szAbsoluteDataPath );
		std::string szLowcaseDir = GammaString( szAbsoluteDataPath );
		CLocalizeDirMap::const_iterator it = m_mapLocalizeDir.upper_bound( szLowcaseDir );
		if( it == m_mapLocalizeDir.begin() )
			return szDataPath;
		it--;
		if( !memcmp( it->first.c_str(), szLowcaseDir.c_str(), it->first.size()*sizeof(char) ) )
			return it->second + ( szDataPath + it->first.size() );
		if( m_mapLocalizeDir.begin()->first.empty() )
			return m_mapLocalizeDir.begin()->second + ( szDataPath + m_mapLocalizeDir.begin()->first.size() );
		return szDataPath;
	}

	void CGammaFileMgr::SetLocalizeDir( const char* szOrgPathName, const char* szLocalizePathName )
	{
		char szAbsoluteOrgPath[2048];
		char szAbsoluteLocalizePath[2048];
		m_FilePackageManager.ConvertToAbsolutePath( szOrgPathName, szAbsoluteOrgPath );
		m_FilePackageManager.ConvertToAbsolutePath( szLocalizePathName, szAbsoluteLocalizePath );
		m_mapLocalizeDir[ GammaString( szAbsoluteOrgPath ) ] = szAbsoluteLocalizePath;
	}
}
