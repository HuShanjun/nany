#pragma once
#include "GammaCommon/IGammaFileMgr.h"
#include "GammaCommon/TGammaRBTree.h"
#include "GammaCommon/CPathMgr.h"
#include "CReadFileThread.h"
#include <string>
#include <vector>
#include <list>
#include <map>

namespace Gamma
{
	class CPackage;
	class CAddressHttp;
	class CGammaFileMgr;
	struct SPackagePart;

	enum ELoadPriority
	{ 
		eLP_Sync,
		eLP_High,
		eLP_Low,
	};

	typedef std::vector<SPackagePart*> CPackageInfoList;
	typedef bool  (*EnumFileHandler)( const char* szFileName, void* pContext );
	typedef void  (*ReadFileHandler)( const char* szFileName, void* pContext, const tbyte* pBuffer, uint32 nSize );

	//============================================
	// 一个分包
	// 当 m_nPackageIndex == 0时
	//		CPackage* m_pPackage 有效
	// 否则
	//		C7ZPackageNode* m_pPrePart 有效
	//============================================
	struct SPackagePart
	{
		std::string			m_strFilePath;

		union
		{
			CPackage*		m_pPackage;
			SPackagePart*	m_pPrePart;
		};

		// m_nOrgSize：
		// 0--------------非压缩文件
		// INVALID_32BITID--未知长度压缩文件
		// 其他-----------压缩文件原始长度
		uint32				m_nOrgSize;	

		// 文件压缩后的大小
		uint32				m_nPackSize;

		// 一个完整包的分包索引，当
		uint16				m_nPackageIndex;
		
		// 曾经成功加载过
		bool				m_bLoaded;

		// 已经在本地存储位置
		bool				m_bCached;
	};

	class CFileContext
	{
		SPackagePart*		m_pPackageNode;
		uint32				m_nIndex;

	public:
		friend class CPackage;
		friend class CPackageMgr;
		CFileContext();
		~CFileContext();

		bool				IsInFileList() const;
		uint32				Size() const;
		const tbyte*		GetBuffer() const;
		CPackage*			GetPackage() const;
	};
	
	struct SAddr
	{
		int32				nAiFamily;
		std::string			strAddBuff;
	};

	class CAddressHttp 
		: public TGammaRBTree<CAddressHttp>::CGammaRBTreeNode
	{
		std::string			m_strOrg;
		std::string			m_strHost;
		HLOCK				m_hLock;
		uint16				m_nPort;
		std::vector<SAddr>	m_vecAddrHttp;
		std::string			m_strMirror;
	public:
		CAddressHttp( const std::string& strKey, const std::string& strHost, uint16 nPort );
		~CAddressHttp();	
		const SAddr*		GetAddr( uint32 nIndex );
		const std::string&	GetOrg() const { return m_strOrg; }
		const std::string&	GetHost() const { return m_strHost; }
		const std::string&	GetMirror() const { return m_strMirror; }
		void				SetMirror( const std::string& strMirror );
		operator			const std::string&() const { return m_strOrg; }
	};

	class CPackageMgr : public IResListener
	{
		enum
		{ 
			eLoadContent_Version, 
			eLoadContent_FileList,
			eLoadContent_ExtractPackage,
			eLoadContent_DownloadPackage,
			eLoadContent_AllPackageUpdated,
			eLoadContent_Nothing
		};	

		typedef CPathMgr::FTW_RESULT FTW_RESULT;
		typedef std::list<SPackagePart> ListPackageType;
		typedef std::map<std::string, CFileContext> MapFileContextType;
		typedef TGammaRBTree<CAddressHttp> MapHttpAddress;
		typedef ListPackageType::iterator PackageIterator;
		typedef std::set<gammacstring> FileListSet;

		CGammaFileMgr*			m_pFileMgr;
		CVersion				m_Version;
		MapFileContextType		m_mapFileContext;
		ListPackageType			m_listPackageParts;
		PackageIterator			m_itPackageCheck;
		CPackageInfoList		m_vecNeedExtractPackage;
		CPackageInfoList		m_vecNeedDownloadPackage;
		MapHttpAddress			m_mapHost;

#ifdef _ANDROID
		mutable bool			m_bFromFileList;
		mutable CRefStringPtr	m_strFileBuffer;
		mutable FileListSet		m_setPkgFileList;
#endif

		std::string				m_strBaseWebPath;
		IGammaFileMgrListener*	m_pFileMgrListener;

		uint32					m_nPreCheckTime;
		uint32					m_nPreDownloadTime;
		bool					m_bWifiConnected;

		bool					m_bReleaseCache;
		bool					m_bLocalFiles;

		uint8					m_eLoadContent;
		uint32					m_nNeedExtractCount;
		uint32					m_nNeedDownloadCount;

		bool					HasFileList() const{ return !m_mapFileContext.empty(); }					
		void					DownLoadPackages();

		void					OnLoadFileList( const char* szFileName, const tbyte* pBuffer, uint32 nSize );
		void					OnLoadVersionFile( const char* szFileName, const tbyte* pBuffer, uint32 nSize );
		void					OnExtractPackage( const char* szFileName, bool bFailed );
		void					OnDownloadPackage( const char* szFileName, bool bFailed );	

		CAddressHttp*			AddHost( std::string strKey, std::string strHost );
		void					AddMirror( const char* szMirrorInfo );
		void					ReadFileList( std::string& strBuffer, FileListSet& setPkgFileList ) const;

	public:
		CPackageMgr( CGammaFileMgr* pFileMgr );
		virtual ~CPackageMgr();
		friend class CGammaFileMgr;

		CGammaFileMgr*			GetFileMgr() const { return m_pFileMgr; }
		void					SetBaseWebPath( const char* szPath, const char* szParam, bool bHttpAsLocal );
		const char*				GetBaseWebPath() const { return m_strBaseWebPath.c_str(); }
		const std::string&		GetBaseWebPathString() const { return m_strBaseWebPath; }
		const CVersion&			GetVersion() const { return m_Version; }
		bool					IsInDownLoad() const { return m_eLoadContent == eLoadContent_DownloadPackage; };
		void					DumpCurrentPackage();
							
		void					ConvertToAbsolutePath( const char* szPathName, char szAbsolutePath[2048] ) const;
		const char*				RevertToShortPath( const char* szAbsolutePath ) const;
		uint32					GetBasePathEnd( const char* szPathName );
		CAddressHttp*			GetHost( const std::string& strUrl );

		CPackage*				CreatePackage( const char* szFullPathName );
		CFileContext*			GetFileBuff( const char* szFilename );
		CFileContext*			GetFileContext( const char* strPathName );

		bool					FileTreeWalk( const char* szDirName, CPathMgr::FILE_PROC_UTF8 pfnFileProc,
									void* pContext, uint32 nDepth = -1, bool bFullFilePath = true );
		bool					HasFile( const char* szFileName ) const;		

		void					UpdateAllPackage( bool bExtract, bool bDownload );
		void					EnableCheckPackageWithWifiOpen( bool bEnable );
		void					CheckPackage();

		void					ReleaseCache( bool bReleaseCache );
		bool					IsReleaseCacheEnable() const;
		IGammaFileMgrListener*	GetFileMgrListener() const { return m_pFileMgrListener; }
		void					SetFileMgrListener( IGammaFileMgrListener* pFileMgrListener );

		void					OnLoadedEnd( const char* szFileName, const tbyte* pBuffer, uint32 nSize );
		bool					OnDownloadPackage( CPackage* pPackage );

		bool					IsFileInCurrentPackage( const char* szNameInPackage ) const;
		bool					ReadResourcePackageFile( std::string& strBuffer, const char* szNameInPackage ) const;
		bool					ExtractPackageFile( EnumFileHandler funOnEnum, ReadFileHandler funOnRead, void* pContext ) const;
	};
}
