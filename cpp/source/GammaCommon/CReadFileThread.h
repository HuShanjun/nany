#pragma once
#include "GammaCommon/CThread.h"
#include "GammaCommon/TGammaList.h"
#include "GammaCommon/TTinyList.h"
#include "GammaCommon/TRefString.h"
#include "GammaCommon/TConstString.h"
#include "GammaCommon/GammaHttp.h"
#include "GammaCommon/IGammaFileMgr.h"

#include <string>
#include <set>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define NE_ETIMEDOUT			WSAETIMEDOUT
#define NE_EWOULDBLOCK			WSAEWOULDBLOCK
#define GNWGetLastError			WSAGetLastError
#pragma comment( linker, "/defaultlib:ws2_32.lib" )
#else
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cerrno>
#include <netdb.h>
#define INVALID_SOCKET			-1
#define closesocket				close
#define ioctlsocket				ioctl
typedef int32					SOCKET;
typedef struct linger			LINGER;
#define SOCKET_ERROR			-1
#define SD_SEND					SHUT_WR
#define NE_ETIMEDOUT			ETIMEDOUT
#define NE_EWOULDBLOCK			EWOULDBLOCK
#define NE_EINPROGRESS			EINPROGRESS
inline int GNWGetLastError()	{ return errno; }
#endif

namespace Gamma
{
	class CPackage;
	class CResObject;
	class CFileReader;
	class CGammaFileMgr;

	class CAddressHttp;
	typedef TGammaList<CFileReader>	CAsynReadList;

	enum EReadResult
	{
		eReadOK = 0,
		eReadTimeOut,
		eReadContinue,
		eReadError,
	};

	enum ELoadType
	{ 
		eLT_Serial,
		eLT_High,
		eLT_Low,
		eLT_Count,
	};

	enum ELocation
	{ 
		eLC_Local,
		eLC_Http,
		eLC_Count,
	};

	struct SReadCache
		: public TTinyList<SReadCache>::CTinyListNode
		, public std::string {};
	typedef TTinyList<SReadCache> CReadCacheList;

	struct SReadListContext
	{
		SReadListContext() : m_bReadParallel(false){}
		CAsynReadList	m_listReader[eLT_Count];
		CReadCacheList	m_listReadCache;
		bool			m_bReadParallel;
	};

	struct SFinishListContext
	{
		SFinishListContext() : m_bReadParallel(false){}
		CAsynReadList	m_listFinish[2];
		bool			m_bReadParallel;
	};

	class CFileReader : public CAsynReadList::CGammaListNode
	{
		CPackage*				m_pPackage;
		std::string				m_strFileName;
		std::string				m_strLocalizePath;
		std::string				m_strCachePath;
		std::string				m_strMd5;
		uint32					m_nOrgSize;
		ELoadState				m_eLoadState;
		CRefStringPtr			m_ptrFileBuff;
		CAddressHttp*			m_addrHttp;
		CHttpRecvState			m_HttpState;
		uint32					m_nCurSize;

	public:
		CFileReader( CPackage* pPackage, const char* szFileName, 
			uint32 nOrgSize, const uint8 szMd5[16], bool bCache );
		~CFileReader();

		int32					Read( std::string& strReadingBuffer );
		bool					Flush( uint32 nIdleTime );
		static void				SaveLocalBuffer( const void* szBuffer, uint32 nSize, 
									const std::string& strCachePath, const char* szContext );

		CPackage*				GetPackage() const { return m_pPackage; }
		const CRefStringPtr&	GetFileBuffer() const { return m_ptrFileBuff; }
		CRefStringPtr&			GetFileBuffer() { return m_ptrFileBuff; }
		const std::string&		GetFileName() const { return m_strFileName; }
		uint32					GetTotalSize() const { return m_HttpState.GetDataSize(); }
		uint32					GetCurSize() const { return m_nCurSize; }
		const std::string&		GetCachePath() const { return m_strCachePath; }
		Gamma::ELoadState		GetLoadState() const { return m_eLoadState; }
		void					SetLoadState( Gamma::ELoadState eState ) { m_eLoadState = eState; }

	private:
		int32					ReadFromDisk( const std::string& szPath, const char* szContext );
		int32					ReadFromHttp( const std::string& szPath, std::string& strReadingBuffer );
		int32					CheckLocalBuffer();
		void					SaveLocalBuffer( const char* szContext );
	};

	class CReadFileThread
	{
        const CReadFileThread& operator= ( const CReadFileThread& );
	public:
		CReadFileThread( SReadListContext& WaitingReader, 
			HSEMAPHORE hSemReader, HLOCK hLockReader,
			SFinishListContext& listFinised, HLOCK hLockFinised );
		~CReadFileThread();

		static uint32			ThreadProc( void* );
		uint32					Run();

	private:
		HLOCK					m_hLockFinised;
		HLOCK					m_hLockReader;
		HSEMAPHORE				m_hSemReader;
		HTHREAD					m_hReadThread;
		bool					m_bQuit;

		SReadListContext&		m_listReader;
		SFinishListContext&		m_listFinised;
	};

	class CExtractThread
	{
		typedef std::set<gammacstring> CExtractSet;
        const CExtractThread& operator= ( const CExtractThread& );
	public:
		CExtractThread( const std::vector<const char*>& aryPackage,
			SFinishListContext& listFinised, HLOCK hLockFinised );
		~CExtractThread();

	private:
		static bool				OnEnumFileHandler( const char* szFileName, void* pContext );
		static void				OnReadFileHandler( const char* szFileName, void* pContext, const tbyte* pBuffer, uint32 nSize );
		void					OnRead( const char* szFileName, const tbyte* pBuffer, uint32 nSize );
		static uint32			ThreadProc( void* );
		uint32					Run();

		bool					m_bAllPackageCommited;
		CExtractSet				m_setExtract;
		HLOCK					m_hLockFinised;
		SFinishListContext&		m_listFinised;
		HTHREAD					m_hReadThread;
	};
}
