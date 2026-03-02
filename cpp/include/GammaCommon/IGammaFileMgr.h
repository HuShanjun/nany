//=====================================================================
// IGammaFileMgr.h
// 定义统一管理的文件管理器
// 柯达昭
// 2009-04-08
//=======================================================================

#ifndef __IGAMMAFILE_MGR_H__
#define __IGAMMAFILE_MGR_H__

#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/IGammaUnknow.h"
#include "GammaCommon/CVersion.h"

namespace Gamma
{	
	enum ELoadState
	{
		eLoadState_NotLoad,
		eLoadState_Loading,
		eLoadState_Failed,
		eLoadState_Succeeded,
	};

	enum ETextFileType
	{
		eTFT_Ucs2,
		eTFT_Utf8,
		eTFT_Utf8NoneFlag,
		eTFT_Default,
	};

	class CIniFile;
	class CTabFile;

	class IGammFileInterface
	{
	protected:
		~IGammFileInterface(){};
	public:
		virtual	int32				Size() = 0;
		virtual int32				Read( void* pBuf, uint32 nSize ) = 0;
		virtual int32				Write( const void* pBuf, uint32 nSize ) = 0;
		virtual void				Seek( uint32 nPos ) = 0;
	};
	
	class IResListener
	{
	public:
		virtual void OnLoadedEnd( const char* szFileName, const tbyte* pBuffer, uint32 nSize ) = 0;
	};	
	
	class IGammaFileMgrListener
	{
	protected:
		~IGammaFileMgrListener(){};
	public:
		/** 版本信息到达
		 *  参数：
		 *	pVersionFile格式：
		 *		[Data]
		 *		Version = 0.0.7
		 *		URL = http://192.168.1.7/lieyao/data/
		 *		[WinCode]
		 *		Version = 0.0.7
		 *		Size = 123456
		 *		URL = http://192.168.1.7/lieyao/data/ShellClient_0.0.7.z
		 *		[AndroidCode]
		 *		Version = 0.0.7
		 *		Size = 123456
		 *		URL = http://192.168.1.7/lieyao/data/com.joyegame.lieyao_0.0.7.apk
		 *		
		 *	返回值：是否要继续下载PackageInfo
		 */
		virtual bool OnNewCodeVersionRetrieved( CIniFile* pVersionFile ) = 0;

		/** 包信息到达
		 *  参数：
		 *	nNewPackageCount：新的包的数量
		 *	nTotalNewSize：新的包的数量总大小
		 *	返回值：是否要马上更新新的包
		 */		
		virtual bool OnPackageInfoRetrieved( CTabFile* pTableFile, uint32 nExtractPackageCount, 
			uint32 nExtractTotalSize, uint32 nDownLoadCount, uint32 nTotalDownLoadSize ) = 0;

		/**
		 * 一个包解压完毕
		 */
		virtual void OnPackageExtracted( const char* szPackageName ) = 0;

		/**
		 * 一个包更新完毕
		 */
		virtual void OnPackageDownloaded( const char* szPackageName, bool bFailed ) = 0;

		/**
		 * 所有的包的更新请求都已经提交
		 */
		virtual void OnAllPackageDownloaded() = 0;

		/**
		 * 一个包正在更新
		 */
		virtual void OnFileProgress( const char* szFileName, uint32 nCurBytes, uint32 nTotalBytes ) = 0;

		/**
		 * 一个包更新完毕
		 */
		virtual void OnFileLoaded( const char* szFileName, bool bSucceeded ) = 0;

		/**
		 * 一个包更新完毕
		 */
		virtual void PostFileLoaded( const char* szFileName, bool bSucceeded ) = 0;

		/**
		 * 所有等待的数据下载完毕
		 */
		virtual void OnAllLoadedEnd() = 0;
	};

	class IGammaFileMgr
	{	
	public:
		virtual void				SetBaseWebPath( const char* szPath, const char* szParam = "", bool bHttpAsLocal = false ) = 0;
		virtual const char*			GetBaseWebPath() const = 0;
		virtual const CVersion&		GetVersion() const = 0;
		virtual void				DumpCurrentPackage() = 0;

		virtual void				UpdateAllPackage( bool bExtract, bool bDownload ) = 0;
		virtual void				EnableCheckPackageWithWifiOpen( bool bEnable ) = 0;

		virtual void				ReleaseCache( bool bReleaseCache ) = 0;
		virtual bool				GetCacheInfo( const char* szFileName, char* szCacheName, size_t nMaxCount, uint32& offset, uint32& size ) = 0;
		virtual void				CheckFileCacheState( const char* szFileName, bool& bInCache, bool& bInPackage ) = 0;
		virtual bool				IsInFileList( const char* szFileName, char* szAbsolutePath = NULL, size_t nMaxSize = 0 ) = 0;

		virtual void				SetLocalizeDir( const char* szOrgPathName, const char* szLocalizePathName ) = 0;								
		virtual void				SetLocalCachePath( const char* szLocalCachePath ) = 0;
		virtual const char*			GetLocalCachePath() = 0;
		virtual void				Flush( uint32 nElapseTime ) = 0;
								
		virtual uint32				SyncLoad( const char* szPathName, bool bSave2Cache, IResListener* pListener ) = 0;
		virtual uint32				ParallelLoad( const char* szPathName, bool bHighPriority, bool bSave2Cache, IResListener* pListener ) = 0;
		virtual uint32				Load( const char* szPathName, bool bHighPriority, bool bSave2Cache, IResListener* pListener ) = 0;

		virtual void				RemoveListener( IResListener* pListener ) = 0;
		virtual void				EnableFileProgress( bool bEnable ) = 0;
		virtual uint32				GetResWaitingCount() = 0;
		virtual uint32				GetObjectCountInHistory() = 0;
		virtual void				SetListener( IGammaFileMgrListener* pMgrListener ) = 0;
		virtual void				Exit() = 0;
	};

	GAMMA_COMMON_API IGammaFileMgr& GetGammaFileMgr();
}

#endif
