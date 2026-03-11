
#include "GammaCommon/CThread.h"
#include "GammaCommon/GammaCodeCvs.h"
#include "GammaCommon/GammaTime.h"
#include "GammaCommon/GammaMd5.h"
#include "GammaCommon/CPathMgr.h"
#include "CReadFileThread.h"
#include "CGammaFileMgr.h"
#include "CResObject.h"
#include "zlib.h"
#include <string>
#include <fcntl.h>

#ifdef _ANDROID
#include "Android.h"
#endif

using namespace std;

//#define MAX_LOAD_SPEED	(100*1024)	//50K每秒

namespace Gamma
{
	class CFileReader;
	typedef TGammaList<CFileReader>	CAsynReadList;

	//===================================================================================
	// 文件异步读取信息
	//===================================================================================
	CFileReader::CFileReader( CPackage* pPackage, const char* szFileName, 
		uint32_t nOrgSize, const uint8_t szMd5[16], bool bCache )
		: m_pPackage( pPackage )
		, m_strFileName( szFileName )
		, m_nOrgSize( nOrgSize )
		, m_strMd5( szMd5 ? (const char*)szMd5 : "", szMd5 ? 16 : 0 )
		, m_addrHttp( NULL )
		, m_eLoadState( eLoadState_NotLoad )
		, m_ptrFileBuff( new CRefString )
		, m_nCurSize( 0 )
	{
		CGammaFileMgr& FileMgr = CGammaFileMgr::Instance();
		CPackageMgr& PackageMgr = FileMgr.GetFilePackageManager();

		if( m_pPackage )
		{
			if( m_pPackage->IsHttpRes() )
				m_addrHttp = PackageMgr.GetHost( m_strFileName );
			m_pPackage->AddRef();
		}

		if( !m_addrHttp )
			m_strLocalizePath = CGammaFileMgr::Instance().MakeLocalizePath( szFileName );
		else if( bCache )
			m_strCachePath = CGammaFileMgr::Instance().MakeCachePath( szFileName );

		if( nOrgSize != INVALID_32BITID )
			m_ptrFileBuff->reserve( nOrgSize );
	}

	CFileReader::~CFileReader()
	{
		SAFE_RELEASE( m_pPackage );
	}

	bool CFileReader::Flush( uint32_t nIdleTime )
	{
		GammaAst( m_pPackage );
		return m_pPackage->OnLoaded( nIdleTime );
	}

	//=============================================================================
	// 以下代码在线程中执行
	//=============================================================================
	int32_t CFileReader::Read( string& strReadingBuffer )
	{
		return m_addrHttp ? ReadFromHttp( m_strFileName, strReadingBuffer ) :
			ReadFromDisk( m_strFileName, "CFileReader::Read" );
	}

	int32_t CFileReader::ReadFromDisk( const string& szPath, const char* szContext )
	{
		if( szPath.empty() )
			return eReadError;

		CGammaFileMgr& FileMgr = CGammaFileMgr::Instance();
		CPackageMgr& PackageMgr = FileMgr.GetFilePackageManager();

		const char* szLocalizePath[2];
		size_t nLen[2];

		if( !m_addrHttp )
		{
			szLocalizePath[0] = m_strLocalizePath.c_str();
			szLocalizePath[1] = m_strLocalizePath != szPath ? szPath.c_str() : "";
			nLen[0] = m_strLocalizePath.size();
			nLen[1] = m_strLocalizePath != szPath ? szPath.size() : 0;
		}
		else
		{
			szLocalizePath[0] = szPath.c_str();
			szLocalizePath[1] = "";
			nLen[0] = szPath.size();
			nLen[1] = 0;
		}

		FILE* hFile = NULL;
		for( size_t i = 0; i < 2 && hFile == NULL; i++ )
		{
			if( !szLocalizePath[i][0] )
				continue;

			uint32_t nPkgRootLen = (uint32_t)strlen( "pkgroot:/" );
			if( !memcmp( szLocalizePath[i], "pkgroot:/", nPkgRootLen ) )
			{
				char szFileNameInPkg[1024];
				uint32_t nLen = strcpy2array_safe( szFileNameInPkg, szLocalizePath[i] + nPkgRootLen );
				if( szFileNameInPkg[nLen - 1] == 'z' && szFileNameInPkg[nLen - 2] == '.' )
					szFileNameInPkg[nLen - 1] = 'r';
				if( PackageMgr.ReadResourcePackageFile( *m_ptrFileBuff, szFileNameInPkg ) )
					return eReadOK;
			}

#ifndef _WIN32
			char szPhysicalPath[2048];
			CPathMgr::ToPhysicalPath( szLocalizePath[i], szPhysicalPath, ELEM_COUNT( szPhysicalPath ) );
			hFile = fopen( szPhysicalPath, "rb" );
#else
			wchar_t szFileName[2048];
			wchar_t szPhysicalPath[2048];
			Utf8ToUcs( szFileName, 2000, szLocalizePath[i], (uint32_t)nLen[i] );
			CPathMgr::ToPhysicalPath( szFileName, szPhysicalPath, ELEM_COUNT( szPhysicalPath ) );
			hFile = _wfopen( szFileName, L"rb" );
#endif
		}

		if( hFile )
		{
			fseek( hFile, 0, SEEK_END );
			uint32_t nSize = (uint32_t)ftell( hFile );

			if( nSize == m_nOrgSize || m_nOrgSize == INVALID_32BITID || m_nOrgSize == 0 )
			{
				fseek( hFile, 0, SEEK_SET );
				m_ptrFileBuff->resize( nSize );
#ifdef MAX_LOAD_SPEED
				for( size_t i = 0; i < nSize; i += MAX_LOAD_SPEED )
				{
					CTimer Timer;
					uint32_t nRead = Min<uint32_t>( MAX_LOAD_SPEED, nSize - i );
					m_bReadOK = fread( &( (*m_ptrOrgBuff)[i] ), 1, nRead, hFile ) == nRead;
					int64_t nSleep = nRead*1000/MAX_LOAD_SPEED - Timer.GetElapse();
					if( nSleep < 0 )
						continue;
					GammaSleep( (uint32_t)nSleep );
				}
#else
				if( fread( &( (*m_ptrFileBuff)[0] ), 1, nSize, hFile ) != nSize )
					GammaLog << "Read file: " << szPath << " size( " << nSize << " ) error!!" << endl;
#endif
				fclose( hFile );
				return eReadOK;
			}
			else
			{
				GammaLog << szContext << ": read error size!!!" << endl;
				fclose( hFile );
			}
		}

		return eReadError;
	}

	int32_t CFileReader::ReadFromHttp( const string& szPath, string& strReadingBuffer )
	{	
		// 尝试从本地读取
		if( CheckLocalBuffer() == eReadOK )
			return eReadOK;		

		int32_t nResult = SOCKET_ERROR;
		SOCKET sk = INVALID_SOCKET;
		
		const SAddr* pAddrInfo = m_addrHttp->GetAddr( 0 );
		for( uint32_t i = 0; pAddrInfo; pAddrInfo = m_addrHttp->GetAddr( ++i ) )
		{
			const SAddr& AddrInfo = *pAddrInfo;
			sk = socket( AddrInfo.nAiFamily, SOCK_STREAM, IPPROTO_TCP );
			if( sk == INVALID_SOCKET )
				continue;
			
#ifdef _WIN32
			u_long nParam = 1;
			if( SOCKET_ERROR == ioctlsocket( sk, FIONBIO, &nParam ) )
			{
				closesocket( sk );
				sk = INVALID_SOCKET;
				continue;
			}
#else
			int32_t opts;
			opts = fcntl( sk, F_GETFL );
			if( opts < 0 )
			{
				closesocket( sk );
				sk = INVALID_SOCKET;
				continue;
			}
			
			opts |= O_NONBLOCK;
			if( fcntl( sk, F_SETFL, opts ) < 0 )
			{
				closesocket( sk );
				sk = INVALID_SOCKET;
				continue;
			}
#endif
			const string& strAddr = AddrInfo.strAddBuff;
			nResult = connect( sk, (struct sockaddr*)strAddr.c_str(), (uint32_t)strAddr.size() );
			if( SOCKET_ERROR == nResult )
			{
				uint32_t nError = GNWGetLastError();
#ifdef _WIN32
				if( nError != NE_EWOULDBLOCK )
#else
				if( nError != NE_EWOULDBLOCK && nError != NE_EINPROGRESS )
#endif
				{
					closesocket(sk);
					sk = INVALID_SOCKET;
					continue;
				}
			}
			break;
		}
		
		if( sk == INVALID_SOCKET )
			return eReadError;
		
		fd_set fdValid;
		FD_ZERO( &fdValid );
		FD_SET( sk, &fdValid );
		
		struct timeval tv;
		tv.tv_sec = 10;
		tv.tv_usec = 0;
		
		if( !select( (int32_t)(sk + 1), NULL, &fdValid, NULL, &tv ) )
		{
			closesocket(sk);
			return eReadTimeOut;
		}

#ifdef MAX_LOAD_SPEED
		const int32_t MAX_BUFF = MAX_LOAD_SPEED;
		CTimer Timer;
#else
		const int32_t MAX_BUFF = 4 * 1024;
#endif

		int32_t nRet = 0;
		uint32_t nCurPos = 0;
		uint32_t nCurCapacity = (uint32_t)strReadingBuffer.size();
		if( nCurCapacity < MAX_BUFF )
		{
			strReadingBuffer.resize( MAX_BUFF );
			nCurCapacity = MAX_BUFF;
		}

		m_HttpState.Reset();
		m_nCurSize = 0;

		//  http文件请求格式。改动格式有可能导致接受的数据不正常
		// const char* szFormat = "GET %s HTTP/1.1\r\n"
		//	"HOST:%s\r\n"
		//	"Accept:*/*\r\n"
		//	"Content-Length: 0\r\n"
		//	"Connection: Keep-Alive\r\n\r\n";

		char* buff = &strReadingBuffer[0];
		*(uint32_t*)buff = *(uint32_t*)"GET ";

		// 跳过 "http://Host:Port"
		uint32_t nSkipCount = 7 + (uint32_t)m_addrHttp->GetHost().size();

		if( m_addrHttp->GetMirror().empty() )
		{
			const char* szFilePath = szPath.c_str() + nSkipCount;
            while( szFilePath[0] != '/' && szFilePath[0] != '\\' )
				++szFilePath;
			URLEncode( (const uint8_t*)szFilePath, buff + 4, MAX_BUFF - 1024 );
		}
		else
		{
			// 用strValue替代strKey
			uint32_t nReplaceCount = (uint32_t)m_addrHttp->GetMirror().size() - nSkipCount;
			memcpy( buff + 4, m_addrHttp->GetMirror().c_str() + nSkipCount, nReplaceCount );
			
			// 跳过 strKey 的长度
			const char* szFilePath = szPath.c_str() + m_addrHttp->GetOrg().size();
			URLEncode( (const uint8_t*)szFilePath, buff + 4 + nReplaceCount, MAX_BUFF - 1024 );
		}

		uint32_t nLen = (uint32_t)strlen( buff );
		gammasstream ssURL( buff + nLen, MAX_BUFF - nLen );
		ssURL << " HTTP/1.1\r\n"
			"HOST:"<< m_addrHttp->GetHost() <<"\r\n"
			"Accept:*/*\r\n"
			"Content-Length: 0\r\n"
			"Connection: Keep-Alive\r\n\r\n";
		send( sk, buff, nLen + (uint32_t)ssURL.length(), 0 );

		tv.tv_sec = 10;
		tv.tv_usec = 0;

		while( 1 == ( nRet = select( (int32_t)(sk + 1), &fdValid, NULL, NULL, &tv ) ) )
		{
			if( nCurCapacity < nCurPos + MAX_BUFF )
			{
				strReadingBuffer.resize( nCurPos + MAX_BUFF );
				nCurCapacity = nCurPos + MAX_BUFF;
			}

			int32_t nLen = (int32_t)recv( sk, &strReadingBuffer[nCurPos], MAX_BUFF, 0 );
			if( SOCKET_ERROR == nLen )
			{
				uint32_t nError = GNWGetLastError();
#ifdef _WIN32
				if( nError != NE_EWOULDBLOCK )
#else
				if( nError != NE_EWOULDBLOCK && nError != NE_EINPROGRESS )
#endif
				{
					bool bTimeOut = nError == NE_ETIMEDOUT;
					m_ptrFileBuff->clear();
					closesocket( sk );
					return bTimeOut ? eReadTimeOut : eReadError;
				}
			}

			if( nLen == 0 )
				break;

			nCurPos += nLen;
			EHttpReadState eState = m_HttpState.
				CheckHttpBuffer( &strReadingBuffer[0], nCurPos );
			m_nCurSize = nCurPos;

			if( eState == eHRS_NeedMore )
			{
#ifdef MAX_LOAD_SPEED
				int64_t nSleep = nLen*1000/MAX_LOAD_SPEED - Timer.GetElapse();
				if( nSleep > 0 )
					GammaSleep( (uint32_t)nSleep );
				Timer.Restart();
#endif
				tv.tv_sec = 10;
				tv.tv_usec = 0;
				continue;
			}

			if( eState == eHRS_Ok )
				break;
			m_ptrFileBuff->clear();
			closesocket( sk );
			return eReadError;
		}

		if( nRet == 0 )
		{
			m_ptrFileBuff->clear();
			closesocket( sk );
			return eReadTimeOut;
		}

		closesocket( sk );	

		if( !m_strMd5.empty() )
		{
			uint8_t nMD5[16];
			MD5( nMD5, strReadingBuffer.c_str(),m_nCurSize );
			if( memcmp( nMD5, m_strMd5.c_str(), 16 ) )
				return eReadError;
		}

		if( m_nOrgSize )
		{			
			z_stream zip;
			memset( &zip, 0, sizeof(zip) );
			inflateInit( &zip );
			zip.next_in = (Bytef*)&strReadingBuffer[0];
			zip.avail_in = m_nCurSize;

			if( m_nOrgSize != INVALID_32BITID )
			{
				m_ptrFileBuff->resize( m_nOrgSize );
				zip.next_out = (Bytef*)&( ( *m_ptrFileBuff )[0] );
				zip.avail_out = m_nOrgSize;
				inflate( &zip, Z_FINISH );
				inflateEnd( &zip );
			}
			else
			{
				uint32_t nReadSize = 0;
				string& strBuffer = *m_ptrFileBuff;
				while( true )
				{
					uint32_t nRemain = zip.avail_in;
					strBuffer.resize( nReadSize + 4096 );
					zip.next_out = (Bytef*)&strBuffer[nReadSize];
					zip.avail_out = 4096;
					zip.total_out = 0;

					if( !nRemain )
					{
						inflate( &zip, Z_FINISH );
						strBuffer.resize( nReadSize + zip.total_out );
						inflateEnd( &zip );
						break;
					}

					inflate( &zip, Z_SYNC_FLUSH );
					nReadSize += zip.total_out;
				}
			}
		}
		else
		{
			m_ptrFileBuff->assign( strReadingBuffer.c_str(), m_nCurSize );
		}

		SaveLocalBuffer( "CFileReader::ReadFromHttp" );
		return eReadOK;
	}
	
	int32_t CFileReader::CheckLocalBuffer()
	{
		if( ReadFromDisk( m_strCachePath, "CFileReader::CheckLocalBuffer" ) == eReadOK )
			return eReadOK;		

		CGammaFileMgr& FileMgr = CGammaFileMgr::Instance();
		CPackageMgr& PackageMgr = FileMgr.GetFilePackageManager();
		uint32_t nLen = (uint32_t)m_strFileName.size();
		char cExtern = 0;
		if( nLen >= 2 && m_strFileName[nLen-1] == 'z' && m_strFileName[nLen-2] == '.' )
			m_strFileName[nLen-1] = cExtern = 'r';

		uint32_t nBasePathEnd = PackageMgr.GetBasePathEnd( m_strFileName.c_str() );
		const char* szNameInPackage = m_strFileName.c_str() + nBasePathEnd;
		bool bReadOK = PackageMgr.ReadResourcePackageFile( *m_ptrFileBuff, szNameInPackage );
		if( cExtern )
			m_strFileName[nLen-1] = 'z';

		if( bReadOK )
		{
#ifdef _ANDROID
			if( !m_strCachePath.empty())
				SaveLocalBuffer( "CFileReader::CheckLocalBuffer(save)" );
#endif
			return eReadOK;
		}

		m_ptrFileBuff->clear();
		return eReadError;
	}

	void CFileReader::SaveLocalBuffer( const char* szContext )
	{
		if( m_ptrFileBuff->size() != m_nOrgSize && 
			m_nOrgSize != INVALID_32BITID && m_nOrgSize != 0 )
		{
			GammaLog << szContext << "( error size, " << this << " )" << m_strFileName 
				<< " OrgSize:" << m_nOrgSize << ", CurSize:" << m_ptrFileBuff->size() << endl;
			return;
		}

		const char* szBuffer = m_ptrFileBuff ? m_ptrFileBuff->c_str() : "";
		uint32_t nSize = m_ptrFileBuff ? (uint32_t)m_ptrFileBuff->size() : 0;
		SaveLocalBuffer( szBuffer, nSize, m_strCachePath, szContext );
	}

	void CFileReader::SaveLocalBuffer( const void* szBuffer, uint32_t nSize, 
					const string& strCachePath, const char* szContext )
	{
		if( strCachePath.empty() )
			return;

		string::size_type nPos = strCachePath.rfind( '/' );
		if( string::npos == nPos )
			return;

#ifdef _WIN32
		wchar_t szPathName[2048];
		wchar_t szPhysicalPath[2048];
		wchar_t szPhysicalPathTemp[2048];
		Utf8ToUcs( szPathName, 2000, strCachePath.c_str(), (uint32_t)strCachePath.size() );
		CPathMgr::ToPhysicalPath( szPathName, szPhysicalPath, ELEM_COUNT( szPhysicalPath ) );
		wcscpy( szPhysicalPathTemp, szPhysicalPath );
		wcscat( szPhysicalPathTemp, L".tmp" );
		FILE* hFile = _wfopen( szPhysicalPathTemp, L"wb" );
#else
		char szPhysicalPath[2048];
		char szPhysicalPathTemp[2048];
		CPathMgr::ToPhysicalPath( strCachePath.c_str(), szPhysicalPath, ELEM_COUNT( szPhysicalPath ) );
		strcpy2array_safe( szPhysicalPathTemp, szPhysicalPath );
		strcat2array_safe( szPhysicalPathTemp, ".tmp" );
		FILE* hFile = fopen( szPhysicalPathTemp, "wb" );
#endif

		if( !hFile )
			return;

		size_t nWriteCount = fwrite( szBuffer, 1, nSize, hFile );
		fclose( hFile );

		if( nWriteCount != nSize )
		{
#ifdef _WIN32
			_wremove( szPhysicalPathTemp );
#else
			remove( szPhysicalPathTemp );
#endif
			GammaLog << szContext << ": write error size!!!" << endl;
			return;
		}

#ifdef _WIN32
		_wremove( szPhysicalPath );
		_wrename( szPhysicalPathTemp, szPhysicalPath );
#else
		remove( szPhysicalPath );
		rename( szPhysicalPathTemp, szPhysicalPath );
#endif
	}

	//----------------------------------------------------------------------------------------
	// CReadFileThread
	//----------------------------------------------------------------------------------------
	CReadFileThread::CReadFileThread( 
		SReadListContext& WaitingReader, 
		HSEMAPHORE hSemReader, HLOCK hLockReader,
		SFinishListContext& listFinised, HLOCK hLockFinised )
		: m_listReader( WaitingReader )
		, m_listFinised( listFinised )
		, m_hLockReader( hLockReader )
		, m_hLockFinised( hLockFinised )
		, m_hSemReader( hSemReader )
		, m_hReadThread( NULL )
		, m_bQuit( false )
	{
		GammaVer( GammaCreateThread( &m_hReadThread, 2048, (THREADPROC)&CReadFileThread::ThreadProc, this ) );
		GammaSetThreadPriority( m_hReadThread, GAMMA_THREAD_PRIORITY_BELOW_NORMAL );
	}

	CReadFileThread::~CReadFileThread()
	{
		GammaJoinThread( m_hReadThread );
		m_hReadThread = NULL;
	}

	//-------------------------------
	// 线程运行函数
	//-------------------------------
	uint32_t CReadFileThread::ThreadProc( void* pContext )
	{
		GammaSetThreadName( "ReadFileThread" );
		return ( (CReadFileThread*)pContext )->Run();
	}

	uint32_t CReadFileThread::Run()
	{
		while( !m_bQuit )
		{
			GammaGetSemaphore( m_hSemReader );

			// 轮流读取并行加载和串行加载
			CFileReader* pInfo = NULL;
			SReadCache* pReadCache = NULL;

			GammaLock( m_hLockReader );
			uint32_t nStartIndex = m_listReader.m_bReadParallel;
			m_listReader.m_bReadParallel = !m_listReader.m_bReadParallel;
			uint32_t nIndex = 0;
			for( int i = 0; !pInfo && i < eLT_Count; i++ )
			{
				nIndex = ( i + nStartIndex ) %eLT_Count;
				pInfo = m_listReader.m_listReader[nIndex].GetFirst();
			}

			if( pInfo )
			{
				pReadCache = m_listReader.m_listReadCache.GetFirst();
				if( pReadCache )
					pReadCache->Remove();
				pInfo->Remove();
			}

			GammaUnlock( m_hLockReader );

			// 如果为空，表示CGammaFileMgr在不读文件的时候也发了信号，
			// 也就是程序退出了，这里要跳出循环
			if( !pInfo )
				break;
			
			if( !pReadCache )
				pReadCache = new SReadCache;

			pInfo->SetLoadState( eLoadState_Loading );
			int32_t nResult = pInfo->Read( *pReadCache );

			GammaLock( m_hLockFinised );
			m_listFinised.m_listFinish[nIndex != eLT_Serial].PushBack( *pInfo );
			pInfo->SetLoadState( eReadOK == nResult ? eLoadState_Succeeded : eLoadState_Failed );
			GammaUnlock( m_hLockFinised );

			GammaLock( m_hLockReader );
			m_listReader.m_listReadCache.PushFront( *pReadCache );
			bool bEmpty = true;
			for( int i = 0; i < eLT_Count && bEmpty; i++ )
				bEmpty = m_listReader.m_listReader[i].GetFirst() == NULL;
			while( bEmpty && m_listReader.m_listReadCache.GetFirst() )
				delete m_listReader.m_listReadCache.GetFirst();
			GammaUnlock( m_hLockReader );
		}

		for( uint32_t i = 0; i < eLT_Count; i++)
		{
			while( m_listReader.m_listReader[i].GetFirst() )
			{
				CFileReader* pInfo = m_listReader.m_listReader[i].GetFirst();
				pInfo->Remove();
				m_listFinised.m_listFinish[i != eLT_Serial].PushBack( *pInfo );
			}
		}

		return 0;
	}

	//----------------------------------------------------------------------------------------
	// CExtractThread
	//----------------------------------------------------------------------------------------
	CExtractThread::CExtractThread( const vector<const char*>& aryPackage,
		SFinishListContext& listFinised, HLOCK hLockFinised )
		: m_bAllPackageCommited( false )
		, m_listFinised( listFinised )
		, m_hLockFinised( hLockFinised )
		, m_hReadThread( NULL )
	{
		string strFileName;
		for( uint32_t i = 0; i < aryPackage.size(); i++ )
		{
			strFileName = aryPackage[i];
			uint32_t nLen = (uint32_t)strFileName.size();
			if( nLen >= 2 && strFileName[nLen-1] == 'z' && strFileName[nLen-2] == '.' )
				strFileName[nLen-1] = 'r';
			m_setExtract.insert( gammacstring( strFileName.c_str() ) );
		}
		GammaVer( GammaCreateThread( &m_hReadThread, 2048, (THREADPROC)&CExtractThread::ThreadProc, this ) );
		GammaSetThreadPriority( m_hReadThread, GAMMA_THREAD_PRIORITY_BELOW_NORMAL );
	}

	CExtractThread::~CExtractThread()
	{
		m_bAllPackageCommited = true;
		GammaJoinThread( m_hReadThread );
	}

	//-------------------------------
	// 线程运行函数
	//-------------------------------
	uint32_t CExtractThread::ThreadProc( void* pContext )
	{
		GammaSetThreadName( "CExtractThread" );
		return ( (CExtractThread*)pContext )->Run();
	}

	uint32_t CExtractThread::Run()
	{
		CGammaFileMgr& FileMgr = CGammaFileMgr::Instance();
		CPackageMgr& PackMgr = FileMgr.GetFilePackageManager();
		PackMgr.ExtractPackageFile( &OnEnumFileHandler, &OnReadFileHandler, this );
		GammaLock( m_hLockFinised );
		CFileReader* pEndRead = new CFileReader( NULL, "", INVALID_32BITID, NULL, false );
		m_listFinised.m_listFinish[!eLT_Serial].PushBack( *pEndRead );
		GammaUnlock( m_hLockFinised );
		return 0;
	}

	bool CExtractThread::OnEnumFileHandler( const char* szFileName, void* pContext )
	{
		CExtractThread* pThis = static_cast<CExtractThread*>( pContext );
		return pThis->m_setExtract.find( gammacstring( szFileName, true ) ) != pThis->m_setExtract.end();
	}

	void CExtractThread::OnReadFileHandler( const char* szFileName, void* pContext, const tbyte* pBuffer, uint32_t nSize )
	{
		( (CExtractThread*)pContext )->OnRead( szFileName, pBuffer, nSize );
	}

	void CExtractThread::OnRead( const char* szFileName, const tbyte* pBuffer, uint32_t nSize )
	{
		CExtractSet::iterator it = m_setExtract.find( gammacstring( szFileName, true ) );
		GammaAst( it != m_setExtract.end() );
		CGammaFileMgr& FileMgr = CGammaFileMgr::Instance();
		CPackageMgr& PackMgr = FileMgr.GetFilePackageManager();
		string strAbsolutePath = PackMgr.GetBaseWebPathString() + szFileName;
		string strCachePath = CGammaFileMgr::Instance().MakeCachePath( strAbsolutePath.c_str() );
		CFileReader::SaveLocalBuffer( pBuffer, nSize, strCachePath, "CFileReader::Extract" );
		GammaLock( m_hLockFinised );
		CFileReader* pExtract = new CFileReader( NULL, strAbsolutePath.c_str(), INVALID_32BITID, NULL, false );
		m_listFinised.m_listFinish[!eLT_Serial].PushBack( *pExtract );
		GammaUnlock( m_hLockFinised );
	}
}
