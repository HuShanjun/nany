//=====================================================================
// CGammaFileMgr.h
// 定义统一管理的文件管理器
// 柯达昭
// 2009-04-08
//=======================================================================

#ifndef __GAMMAFILE_MGR_H__
#define __GAMMAFILE_MGR_H__

#include "GammaCommon/IGammaFileMgr.h"
#include "GammaCommon/TGammaList.h"
#include "GammaCommon/CThread.h"
#include "GammaCommon/CBinaryDiff.h"
#include "GammaCommon/GammaTime.h"
#include "CReadFileThread.h"
#include "CPackageMgr.h"
#include <set>
#include <map>

namespace Gamma
{
	#define READ_THREAD_COUNT 5
	typedef std::multimap<IResListener*, CResObject*> CMapListener;
	typedef std::set<CResObject*> CListNeedProgress;
	class CQueueBelongedNode;

	//===================================================================================
	// 文件读取管理器
	//===================================================================================
	class CGammaFileMgr : public IGammaFileMgr
	{	
		struct SFileFailed;
		typedef TGammaList<CQueueBelongedNode> CResObjectList;
		typedef std::map<std::string, std::string> CLocalizeDirMap;
		typedef std::map<std::string, SFileFailed> CExcludeFileMap;

		struct SFileFailed
		{
			SFileFailed() : m_nFailedTime( 0 ), m_nFailedCount( 0 ){}
			uint32_t					m_nFailedTime;
			uint32_t					m_nFailedCount;
		};

		CReadFileThread*			m_vecReadThreadList[READ_THREAD_COUNT];
		CExtractThread*				m_pExtractThread;

		HLOCK						m_hLockLoaded;
		HLOCK						m_hLockHttpWaiting;
		HLOCK						m_hLockLocalWaiting;
		HSEMAPHORE					m_hSemLocalWaiting;
		HSEMAPHORE					m_hSemHttpWaiting;
		SReadListContext			m_listWaiting[eLC_Count];
		SFinishListContext			m_listLoaded;

		std::string					m_strLocalCachePath;
		CLocalizeDirMap				m_mapLocalizeDir;
		bool						m_bQuit;
		bool						m_bEnableFileProgress;

		CPackageMgr					m_FilePackageManager;
		uint32_t						m_objectCountInHistory;
		uint32_t						m_nWaitingObjectCount;
		CResObjectList				m_arrSerialWaitingObject;
		CResObjectList				m_arrParallelFinishObject;
		CResObjectList				m_arrParallelWaitingObject;
		uint32_t						m_nCurWaitingSerialID;

		CExcludeFileMap				m_mapExcludeFile;
		CMapListener				m_mapListener;
		CListNeedProgress			m_listNeedProgress;
		bool						m_bLoadPackageFirst;
		std::string					m_strTempBuffer;

		void						OnResObjectLoadedEnd( CResObject* pResObject );
		void						ApplyAllLoaded( uint32_t nIdleTime );

		void						ProcessObject( CResObject* pResObject );
		void						RemoveObject( CResObject* pResObject );
		bool						AddObject( const char* szPathName, bool bCache, bool bSerial, 
										ELoadPriority eLoadPriority, IResListener* pListener );

		CPackage*					CreatePackage( const char* szPathName );
		uint32_t						LoadDir( const char* szDirName, bool bCache, bool bSerial,
										ELoadPriority eLoadPriority, IResListener* pListener );

		CGammaFileMgr();
		virtual ~CGammaFileMgr();
	public:
		friend class CPackage;

		static CGammaFileMgr&		Instance();

		bool						IsQuit() { return m_bQuit; }
		std::string					MakeCachePath( const char* szDataPath ) const;
		std::string					MakeLocalizePath( const char* szDataPath ) const;
		CPackageMgr&				GetFilePackageManager();
		std::string&				GetTempBuffer() { return m_strTempBuffer; }
		const std::string&			GetLocalCachePathString() { return m_strLocalCachePath; }

		virtual void				SetBaseWebPath( const char* szPath, const char* szParam, bool bHttpAsLocal );
		virtual const char*			GetBaseWebPath() const;
		virtual const CVersion&		GetVersion() const;
		virtual void				DumpCurrentPackage();

		virtual void				UpdateAllPackage( bool bExtract, bool bDownload );
		virtual void				EnableCheckPackageWithWifiOpen( bool bEnable );

		virtual void				ReleaseCache( bool bReleaseCache );
		virtual bool				GetCacheInfo( const char* szFileName, char* szCacheName, size_t nMaxCount, uint32_t& offset, uint32_t& size );		
		virtual void				CheckFileCacheState( const char* szFileName, bool& bInCache, bool& bInPackage );
		virtual bool				IsInFileList( const char* szFileName, char* szAbsolutePath = NULL, size_t nMaxSize = 0 );

		virtual void				SetLocalizeDir( const char* szOrgPathName, const char* szLocalizePathName );
		virtual void				SetLocalCachePath( const char* szLocalCachePath );
		virtual const char*			GetLocalCachePath(){ return m_strLocalCachePath.c_str(); }
		virtual void				Flush( uint32_t nElapseTime );

		virtual uint32_t				GetResWaitingCount();
		virtual uint32_t				GetObjectCountInHistory();
		virtual void				SetListener( IGammaFileMgrListener* pMgrListener );
		virtual void				RemoveListener( IResListener* pListener );
		virtual void				EnableFileProgress( bool bEnable );
		virtual void				Exit();

		virtual void				CreateExtractThread( const CPackageInfoList& listPackage );
		virtual uint32_t				SyncLoad( const char* szPathName, bool bSave2Cache, IResListener* pListener );
		virtual uint32_t				ParallelLoad( const char* szPathName, bool bHighPriority, bool bSave2Cache, IResListener* pListener );
		virtual uint32_t				Load( const char* szPathName, bool bHighPriority, bool bSave2Cache, IResListener* pListener );
	};
}

#endif
