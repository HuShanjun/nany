
#ifdef _WIN32
#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/GammaCodeCvs.h"
#include "GammaCommon/TGammaStrStream.h"
#include "GammaCommon/IGammaFileMgr.h"
#include "GammaCommon/CVersion.h"
#include "GammaCommon/CPathMgr.h"
#include "Win32.h"
#include <string>
#include <vector>
#include <Iphlpapi.h>
#include <winsock2.h>
#include <shellapi.h>
#include <commdlg.h>
#include <tchar.h>
#include <Shlobj.h>
#include <Shobjidl.h>
#include <dlgs.h>
#include <intrin.h>

#pragma comment( lib, "iphlpapi.lib" )
#pragma comment( lib, "version.lib" )
#pragma comment( lib, "Shell32.lib" )

using namespace std;

namespace Gamma
{
	CWin32App::CWin32App()
	{
		CoInitializeEx( NULL, COINIT_MULTITHREADED );
		FetchHardwareInfo();
	}

	CWin32App::~CWin32App()
	{
		CoUninitialize();
	}

	CWin32App& CWin32App::GetInstance()
	{
		static CWin32App s_instance;
		return s_instance;
	}

	void* CWin32App::GetModuleHandle()
	{
		return ::GetModuleHandle( NULL );
	}

	uint64 CWin32App::GetVersion()
	{
		wchar_t szBuffer[2048];
		::GetModuleFileNameW( NULL, szBuffer, ELEM_COUNT( szBuffer ) );
		DWORD dwHandle;  
		DWORD dwVersionInfoSize;  
		dwVersionInfoSize = ::GetFileVersionInfoSizeW( szBuffer, &dwHandle );  

		if( 0 == dwVersionInfoSize )  
			return 0;  

		char* pVersionInfo = new char[dwVersionInfoSize];  
		if( !::GetFileVersionInfoW( szBuffer, 0, dwVersionInfoSize, pVersionInfo ) )  
		{  
			delete[] pVersionInfo;  
			return 0;  
		}  

		VS_FIXEDFILEINFO* pVersion = NULL;  
		uint32 nLen;  
		if( !VerQueryValueW( pVersionInfo, L"\\", (void**)&pVersion, &nLen ) )  
		{  
			delete[] pVersionInfo;  
			return 0;  
		}  

		CVersion Version( 
			(uint8)( pVersion->dwFileVersionMS >> 24 ),
			(uint8)( pVersion->dwFileVersionMS >> 16 ),
			(uint16)( pVersion->dwFileVersionMS ),
			(uint16)( pVersion->dwFileVersionLS >> 16 ),
			(uint16)( pVersion->dwFileVersionLS ) );  
		delete[] pVersionInfo;
		return Version;
	}

	void CWin32App::FetchHardwareInfo()
	{
		/*
		char	m_szDeviceDesc[DEVICEDESC_LEN]*;
		char	m_szCpuType[CPUTYPE_LEN]*;
		char	m_szOSDesc[OSDESC_LEN]*;
		char	m_szLanguage[LANGUAGE_LEN]*;
		uint64	m_nMac*;
		uint32	m_nCpuFrequery*;
		uint32	m_nCpuCount*;
		uint32	m_nMemSize;
		uint16  m_nScreen_X;
		uint16	m_nScreen_Y;
		*/
		DWORD nLen = DEVICEDESC_LEN;
		wchar_t szComputerName[DEVICEDESC_LEN];
		GetComputerNameW( szComputerName, &nLen );
		UcsToUtf8( m_HardwareDesc.m_szDeviceDesc, DEVICEDESC_LEN, szComputerName );
 
		gammasstream ssOSDesc( m_HardwareDesc.m_szOSDesc, sizeof( m_HardwareDesc.m_szOSDesc ) );
		OSVERSIONINFO osvi; 
		ZeroMemory(&osvi, sizeof(OSVERSIONINFO) );
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx ( (OSVERSIONINFO*)&osvi ); 
		if( osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 3 )
			ssOSDesc << "Windows 8.1"; 
		if( osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 2 )
			ssOSDesc << "Windows 8"; 
		if( osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1 )
			ssOSDesc << "Windows 7"; 
		if( osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0 )
			ssOSDesc << "Windows Vista"; 
		if( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
			ssOSDesc << "Windows Server 2003"; 
		if( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
			ssOSDesc << "Windows XP"; 
		if( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
			ssOSDesc << "Windows 2000"; 

		char* szCpuType = m_HardwareDesc.m_szCpuType;
		__cpuid( (int32*)&szCpuType[0x00], 0x80000002 );
		__cpuid( (int32*)&szCpuType[0x10], 0x80000003 );
		__cpuid( (int32*)&szCpuType[0x20], 0x80000004 );
		int32 nStart = 0;
		while( IsBlank( szCpuType[nStart] ) )
			nStart++;
		memmove( szCpuType, szCpuType + nStart, CPUTYPE_LEN - nStart );

		string strBuffer;
		strBuffer.resize( sizeof(IP_ADAPTER_INFO) );
		ULONG nBufLen = sizeof(IP_ADAPTER_INFO);
		if( GetAdaptersInfo( (IP_ADAPTER_INFO*)&strBuffer[0], &nBufLen ) == ERROR_BUFFER_OVERFLOW )
			strBuffer.resize( nBufLen );
		if ( GetAdaptersInfo( (IP_ADAPTER_INFO*)&strBuffer[0], &nBufLen ) == NO_ERROR )
			m_HardwareDesc.m_nMac = *(uint64*)( (IP_ADAPTER_INFO*)&strBuffer[0] )->Address;
		uint8* nMac = (uint8*)&m_HardwareDesc.m_nMac;
		sprintf( m_HardwareDesc.m_szUUID, "%02x-%02x-%02x-%02x-%02x-%02x", 
			nMac[0], nMac[1], nMac[2], nMac[3], nMac[4], nMac[5] );

		SYSTEM_INFO systeminfo;
		GetSystemInfo( &systeminfo );
		m_HardwareDesc.m_nCpuCount = systeminfo.dwNumberOfProcessors;
		
		const char* szLanguageName = "Unknown";
		LANGID nLanguageId = GetSystemDefaultLangID();

		if( nLanguageId == 0x0436 ) szLanguageName = "af";
		else if( nLanguageId == 0x041c ) szLanguageName = "sq";
		else if( nLanguageId == 0x0401 ) szLanguageName = "ar";
		else if( nLanguageId == 0x0801 ) szLanguageName = "ar-IQ";
		else if( nLanguageId == 0x0c01 ) szLanguageName = "ar-EG";
		else if( nLanguageId == 0x1001 ) szLanguageName = "ar-LY";
		else if( nLanguageId == 0x1401 ) szLanguageName = "ar-DZ";
		else if( nLanguageId == 0x1801 ) szLanguageName = "ar-MA";
		else if( nLanguageId == 0x1c01 ) szLanguageName = "ar-TN";
		else if( nLanguageId == 0x2001 ) szLanguageName = "ar-OM";
		else if( nLanguageId == 0x2401 ) szLanguageName = "ar-YE";
		else if( nLanguageId == 0x2801 ) szLanguageName = "ar-SY";
		else if( nLanguageId == 0x2c01 ) szLanguageName = "ar-JO";
		else if( nLanguageId == 0x3001 ) szLanguageName = "ar-LB";
		else if( nLanguageId == 0x3401 ) szLanguageName = "ar-KW";
		else if( nLanguageId == 0x3801 ) szLanguageName = "ar";
		else if( nLanguageId == 0x3c01 ) szLanguageName = "ar-BH";
		else if( nLanguageId == 0x4001 ) szLanguageName = "ar-QA";
		else if( nLanguageId == 0x042d ) szLanguageName = "eu";
		else if( nLanguageId == 0x0423 ) szLanguageName = "be";
		else if( nLanguageId == 0x0402 ) szLanguageName = "bg";
		else if( nLanguageId == 0x0455 ) szLanguageName = "my";
		else if( nLanguageId == 0x0403 ) szLanguageName = "ca";
		else if( nLanguageId == 0x0404 ) szLanguageName = "zh-TW";
		else if( nLanguageId == 0x0804 ) szLanguageName = "zh-CN";
		else if( nLanguageId == 0x0c04 ) szLanguageName = "zh-HK";
		else if( nLanguageId == 0x1004 ) szLanguageName = "zh-SG";
		else if( nLanguageId == 0x041a ) szLanguageName = "hr";
		else if( nLanguageId == 0x0405 ) szLanguageName = "cs";
		else if( nLanguageId == 0x0406 ) szLanguageName = "da";
		else if( nLanguageId == 0x0413 ) szLanguageName = "nl-NL";
		else if( nLanguageId == 0x0813 ) szLanguageName = "nl-BE";
		else if( nLanguageId == 0x0409 ) szLanguageName = "en-US";
		else if( nLanguageId == 0x0809 ) szLanguageName = "en-GB";
		else if( nLanguageId == 0x0c09 ) szLanguageName = "en-AU";
		else if( nLanguageId == 0x1009 ) szLanguageName = "en-CA";
		else if( nLanguageId == 0x1409 ) szLanguageName = "en-NZ";
		else if( nLanguageId == 0x1809 ) szLanguageName = "en-IE";
		else if( nLanguageId == 0x1c09 ) szLanguageName = "en-ZA";
		else if( nLanguageId == 0x2009 ) szLanguageName = "en-JM";
		else if( nLanguageId == 0x2409 ) szLanguageName = "en-BQ";
		else if( nLanguageId == 0x2809 ) szLanguageName = "en-BZ";
		else if( nLanguageId == 0x2c09 ) szLanguageName = "en-TT";
		else if( nLanguageId == 0x0425 ) szLanguageName = "et";
		else if( nLanguageId == 0x040b ) szLanguageName = "fi";
		else if( nLanguageId == 0x040c ) szLanguageName = "fr";
		else if( nLanguageId == 0x080c ) szLanguageName = "fr";
		else if( nLanguageId == 0x0c0c ) szLanguageName = "fr";
		else if( nLanguageId == 0x100c ) szLanguageName = "fr-CH";
		else if( nLanguageId == 0x140c ) szLanguageName = "fr-LU";
		else if( nLanguageId == 0x0407 ) szLanguageName = "de";
		else if( nLanguageId == 0x0807 ) szLanguageName = "de-CH";
		else if( nLanguageId == 0x0c07 ) szLanguageName = "de-AT";
		else if( nLanguageId == 0x1007 ) szLanguageName = "de-LU";
		else if( nLanguageId == 0x1407 ) szLanguageName = "de-LI";
		else if( nLanguageId == 0x0408 ) szLanguageName = "el";
		else if( nLanguageId == 0x040d ) szLanguageName = "he";
		else if( nLanguageId == 0x040e ) szLanguageName = "hu";
		else if( nLanguageId == 0x040f ) szLanguageName = "is";
		else if( nLanguageId == 0x0421 ) szLanguageName = "id";
		else if( nLanguageId == 0x0410 ) szLanguageName = "it";
		else if( nLanguageId == 0x0810 ) szLanguageName = "it-CH";
		else if( nLanguageId == 0x0411 ) szLanguageName = "ja";
		else if( nLanguageId == 0x0412 ) szLanguageName = "ko";
		else if( nLanguageId == 0x0426 ) szLanguageName = "lv";
		else if( nLanguageId == 0x0427 ) szLanguageName = "lt";
		else if( nLanguageId == 0x043e ) szLanguageName = "ms";
		else if( nLanguageId == 0x083e ) szLanguageName = "ms-BN";
		else if( nLanguageId == 0x0414 ) szLanguageName = "no";
		else if( nLanguageId == 0x0814 ) szLanguageName = "no";
		else if( nLanguageId == 0x0415 ) szLanguageName = "pl";
		else if( nLanguageId == 0x0416 ) szLanguageName = "pt-BR";
		else if( nLanguageId == 0x0816 ) szLanguageName = "pt-PT";
		else if( nLanguageId == 0x0418 ) szLanguageName = "ro";
		else if( nLanguageId == 0x0419 ) szLanguageName = "ru";
		else if( nLanguageId == 0x0c1a ) szLanguageName = "sr";
		else if( nLanguageId == 0x081a ) szLanguageName = "sr";
		else if( nLanguageId == 0x041b ) szLanguageName = "sk";
		else if( nLanguageId == 0x0424 ) szLanguageName = "sl";
		else if( nLanguageId == 0x040a ) szLanguageName = "es";
		else if( nLanguageId == 0x080a ) szLanguageName = "es";
		else if( nLanguageId == 0x0c0a ) szLanguageName = "es";
		else if( nLanguageId == 0x100a ) szLanguageName = "es-GT";
		else if( nLanguageId == 0x140a ) szLanguageName = "es";
		else if( nLanguageId == 0x180a ) szLanguageName = "es-PA";
		else if( nLanguageId == 0x1c0a ) szLanguageName = "es";
		else if( nLanguageId == 0x200a ) szLanguageName = "es-VE";
		else if( nLanguageId == 0x240a ) szLanguageName = "es-CO";
		else if( nLanguageId == 0x280a ) szLanguageName = "es-PE";
		else if( nLanguageId == 0x2c0a ) szLanguageName = "es-AR";
		else if( nLanguageId == 0x300a ) szLanguageName = "es";
		else if( nLanguageId == 0x340a ) szLanguageName = "es-CL";
		else if( nLanguageId == 0x380a ) szLanguageName = "es-UY";
		else if( nLanguageId == 0x3c0a ) szLanguageName = "es-PY";
		else if( nLanguageId == 0x400a ) szLanguageName = "es-BO";
		else if( nLanguageId == 0x440a ) szLanguageName = "es";
		else if( nLanguageId == 0x480a ) szLanguageName = "es-HN";
		else if( nLanguageId == 0x4c0a ) szLanguageName = "es-NI";
		else if( nLanguageId == 0x500a ) szLanguageName = "es";
		else if( nLanguageId == 0x0441 ) szLanguageName = "sw-KE";
		else if( nLanguageId == 0x041d ) szLanguageName = "sv";
		else if( nLanguageId == 0x081d ) szLanguageName = "sv-FI";
		else if( nLanguageId == 0x0444 ) szLanguageName = "tt";
		else if( nLanguageId == 0x041e ) szLanguageName = "th";
		else if( nLanguageId == 0x041f ) szLanguageName = "tr";
		else if( nLanguageId == 0x0422 ) szLanguageName = "uk";
		else if( nLanguageId == 0x0820 ) szLanguageName = "ur-IN";
		else if( nLanguageId == 0x0443 ) szLanguageName = "uz";
		else if( nLanguageId == 0x0843 ) szLanguageName = "uz";

		gammasstream( m_HardwareDesc.m_szLanguage, sizeof( m_HardwareDesc.m_szLanguage ) )
			<< szLanguageName;

		LARGE_INTEGER freq;
		QueryPerformanceFrequency( &freq );	///< cpu 主频
		m_HardwareDesc.m_nCpuFrequery = (uint32)( freq.QuadPart / ( 1000 )  );

		MEMORYSTATUS mem;
		GlobalMemoryStatus( &mem );			///< 内存容量
		m_HardwareDesc.m_nMemSize = (uint32)mem.dwTotalPhys / ( 1024 * 1024 );

		m_HardwareDesc.m_nScreen_X = (uint16)GetSystemMetrics(SM_CXSCREEN);
		m_HardwareDesc.m_nScreen_Y = (uint16)GetSystemMetrics(SM_CYSCREEN);
	}

	void CWin32App::GetHardwareDesc( SHardwareDesc& HardwareDesc )
	{
		HardwareDesc = m_HardwareDesc;
	}

	void CWin32App::SetPackagePath( const char* szPath )
	{
		m_strPackagePath = szPath;
	}
	
	const char* CWin32App::GetPackagePath()
	{
		if( m_strPackagePath.empty() )
			return GetGammaFileMgr().GetLocalCachePath();
		return m_strPackagePath.c_str();
	}

	int32 CWin32App::WindowMessagePump()
	{
		int32 nMsgCount = 0;
#if !( defined _DEBUG )
		__try
		{
#endif
			MSG msg;
			while( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) )
			{
				nMsgCount++;
				if( !GetMessage( &msg, NULL, 0, 0 ) )
					return -1;
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}

			CheckFileCallback();
#if !( defined _DEBUG )
		}
		__except( EXCEPTION_EXECUTE_HANDLER )
		{
			GammaLog << "system msg error" << endl;				
		}
#endif
		return nMsgCount;
	}

	void CWin32App::SetClipboardContent( int32 nType, const void* pContent, uint32 nSize )
	{
		if( !pContent )
			return;

		if( !OpenClipboard( NULL ) )
			return;
		EmptyClipboard();
		uint32 nBufferSize = (uint32)( ( nSize + 1 ) * sizeof(wchar_t) );
		HANDLE hMem = GlobalAlloc( GMEM_MOVEABLE|GMEM_DDESHARE, nBufferSize );
		if( !hMem )
			return;
		wchar_t* szBuf = (wchar_t*)GlobalLock( hMem );
		if( !szBuf )
			return;
		szBuf[nSize] = 0;
		UINT uFormat = CF_MAX;

		if( nType == CONTENT_TYPE_TEXT )
		{
			wstring strValue = Utf8ToUcs( (const char*)pContent );
			memcpy( szBuf, strValue.c_str(), nSize );
			uFormat = CF_UNICODETEXT;
		}

		GlobalUnlock( hMem );
		if( uFormat != CF_MAX )
			SetClipboardData( uFormat, hMem );
		CloseClipboard();
		GlobalFree( hMem );	
	}

	void CWin32App::GetClipboardContent( int32 nType, const void*& pContent, uint32& nSize )
	{
		if( !OpenClipboard( NULL ) )
			return;

		if( nType == CONTENT_TYPE_TEXT )
		{
			static string s_ClipString;
			s_ClipString = UcsToUtf8( (const wchar_t*)GetClipboardData( CF_UNICODETEXT ) );
			nSize = (uint32)s_ClipString.size();
			pContent = s_ClipString.c_str();
		}

		CloseClipboard();		
	}

	bool CWin32App::GetSystemFile( bool bList, int32 nType, void* pContext, void* funCallback )
	{
		if( funCallback == NULL )
			return false;

		SFileOpenContext* pFileContext = new SFileOpenContext;
		pFileContext->m_bList = bList;
		pFileContext->m_nType = nType;
		pFileContext->m_pContext = pContext;
		pFileContext->m_funCallback = funCallback;
		struct SFileOpen
		{
			static void Open( SFileOpenContext* pContext )
			{
				int32 arySysType[CONTENT_TYPE_COUNT] =
				{
					CSIDL_COMMON_DOCUMENTS,
					CSIDL_COMMON_PICTURES,
					CSIDL_COMMON_MUSIC,
					CSIDL_COMMON_VIDEO
				};

				WCHAR szDirectory[260];	 // InitPath
				SHGetSpecialFolderPathW( NULL, szDirectory, arySysType[pContext->m_nType], false );

				if( !pContext->m_bList )
				{
					WCHAR szFile[260];       // buffer for file name

					COMDLG_FILTERSPEC aryText[CONTENT_TYPE_COUNT][2] =
					{ 
						{ { L"Text", L"*.txt;*.log;*.inf;*.ini" },		{ L"All", L"*.*" } },
						{ { L"Image", L"*.bmp;*.jpg;*.jpeg;*.png" },	{ L"All", L"*.*" } },
						{ { L"Audio", L"*.wav;*.mp3" },					{ L"All", L"*.*" } },
						{ { L"Video", L"*.avi;*.mp4;*.264" },			{ L"All", L"*.*" } },
					};

					const WCHAR* szFilter[CONTENT_TYPE_COUNT] =
					{
						L"Text\0*.txt;*.log;*.inf;*.ini\0All\0*.*\0",
						L"Image\0*.bmp;*.jpg;*.jpeg;*.png\0All\0*.*\0",
						L"Audio\0*.wav;*.mp3\0All\0*.*\0",
						L"Video\0*.avi;*.mp4;*.264\0All\0*.*\0",
					};

					OSVERSIONINFO vi;
					ZeroMemory(&vi, sizeof(OSVERSIONINFO));
					vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
					::GetVersionEx(&vi);

					BOOL bOpenSuceeded = FALSE;
					// if running under Vista
					if( vi.dwMajorVersion >= 6 )
					{
						HRESULT hr;
						IFileDialog* pIFileDialog;
						hr = CoCreateInstance(CLSID_FileOpenDialog, NULL,
							CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pIFileDialog) );

						pIFileDialog->SetFileTypes( 1, aryText[pContext->m_nType] );

						IShellItem* psiInitialDir = NULL;
						typedef HRESULT ( __stdcall *Fun )( PCWSTR, IBindCtx*, _In_ REFIID, void** );
						HMODULE hShell = LoadLibraryW( L"Shell32.dll" );
						Fun funSHCreateItem = (Fun)GetProcAddress( hShell, "SHCreateItemFromParsingName" );
						hr = funSHCreateItem( szDirectory, NULL, IID_PPV_ARGS(&psiInitialDir) );
						FreeLibrary( hShell );
						if( SUCCEEDED( hr ) )
						{
							pIFileDialog->SetFolder( psiInitialDir );
							psiInitialDir->Release();
						}

						pIFileDialog->Show( NULL );
						IShellItem* psiResult;
						if( SUCCEEDED( pIFileDialog->GetResult( &psiResult ) ) )
						{
							LPWSTR wcPathName = NULL;
							hr = psiResult->GetDisplayName( SIGDN_FILESYSPATH, &wcPathName );
							bOpenSuceeded = SUCCEEDED( hr );
							if( bOpenSuceeded )
								strcpy2array_safe( szFile, wcPathName );
						}
					}
					else
					{
						OPENFILENAMEW ofn;       // common dialog box structure
						// Initialize OPENFILENAME
						ZeroMemory(&ofn, sizeof(ofn));
						ofn.lStructSize = sizeof(ofn);
						ofn.hwndOwner = NULL;
						ofn.lpstrFile = szFile;
						// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
						// use the contents of szFile to initialize itself.
						ofn.lpstrFile[0] = '\0';
						ofn.nMaxFile = sizeof(szFile);
						ofn.lpstrFilter = szFilter[pContext->m_nType];
						ofn.nFilterIndex = 1;
						ofn.lpstrFileTitle = NULL;
						ofn.nMaxFileTitle = 0;
						ofn.lpstrInitialDir = szDirectory;
						ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
						bOpenSuceeded = GetOpenFileNameW( &ofn );
					}

					// Display the Open dialog box. 
					if( bOpenSuceeded )
					{
						GammaString( szFile );
						pContext->m_vecFileName.push_back( UcsToUtf8( szFile ) );
					}
				}
				else
				{
					CPathMgr::FileTreeWalk( szDirectory, (CPathMgr::FILE_PROC)&Walk, pContext );
				}
				CWin32App::GetInstance().m_Lock.Lock();
				CWin32App::GetInstance().m_listContext.PushBack( *pContext );
				CWin32App::GetInstance().m_Lock.Unlock();
			}

			static CPathMgr::FTW_RESULT Walk( const wchar_t* szDirectory, 
				CPathMgr::FTW_FLAG eFlag, SFileOpenContext* pContext )
			{
				if( eFlag == CPathMgr::eFTW_FLAG_FILE )
				{
					string strPath = UcsToUtf8( szDirectory );
					GammaString( strPath );
					pContext->m_vecFileName.push_back( strPath );
				}
				return CPathMgr::eFTW_RESULT_CONTINUNE;
			}
		};

		HTHREAD hThread;
		GammaCreateThread( &hThread, 0, (THREADPROC)&SFileOpen::Open, pFileContext );
		return true;
	}

	void CWin32App::CheckFileCallback()
	{
		while( m_listContext.GetFirst() )
		{
			SFileOpenContext* pContext = m_listContext.GetFirst();
			m_Lock.Lock();
			pContext->Remove();
			m_Lock.Unlock();
			if( pContext->m_funCallback )
			{
				if( pContext->m_bList )
				{
					vector<const char*> vecPath;
					for( uint32 i = 0; i < pContext->m_vecFileName.size(); i++ )
						vecPath.push_back( pContext->m_vecFileName[i].c_str() );
					const char** aryPath = vecPath.empty() ? NULL : &vecPath[0];
					( (SystemFileListCallback)pContext->m_funCallback )( 
						pContext->m_pContext, aryPath, (uint32)vecPath.size() );
				}
				else
				{
					const char* szPath = pContext->m_vecFileName.size() ?
						pContext->m_vecFileName[0].c_str() : NULL;
					( (SystemFileCallback)pContext->m_funCallback )
						( pContext->m_pContext, szPath );
				}
			}
			delete pContext;
		}
	}

	void CWin32App::OpenURL(const char* szUrl)
	{
		wstring strValue = Utf8ToUcs( szUrl );
		wstring strOp = Utf8ToUcs( "open" );
		ShellExecuteW( NULL, strOp.c_str(), strValue.c_str(), NULL, NULL, SW_SHOWNORMAL  );
	}

	void* CWin32App::LoadDynamicLib( const char* szName )
	{
		wstring strName = Utf8ToUcs( szName );
		if( strName.find( L".dll" ) == wstring::npos )
			strName += L".dll";
		return ::LoadLibraryW( strName.c_str() );
	}

	void* CWin32App::GetFunctionAddress( void* pLibContext, const char* szName )
	{
		return GetProcAddress( (HMODULE)pLibContext, szName );
	}

	uint32 AnsiToUcs( wchar_t* pUcs, uint32 nSize, const char* pAnsi, uint32 nLen )
	{
		return MultiByteToWideChar( CP_ACP, 0, pAnsi, nSize, pUcs, nLen );
	}

	const std::wstring AnsiToUcs( const char* pUtf8, uint32 nLen /*= -1 */ )
	{
		std::wstring sTemp;
		if( !pUtf8 )
			return sTemp;
		if( nLen == -1 )
			nLen = (uint32)strlen( pUtf8 );
		sTemp.resize( nLen );
		AnsiToUcs( &sTemp[0], (uint32)sTemp.size(), pUtf8, nLen );
		return sTemp.c_str();
	}

	uint32 UcsToAnsi( char* pAnsi, uint32 nSize, const wchar_t* pUnicode, uint32 nLen /*= -1 */ )
	{
		return WideCharToMultiByte( CP_ACP, 0, pUnicode, nLen, pAnsi, nSize, NULL, NULL );
	}

	const std::string UcsToAnsi( const wchar_t* pUnicode, uint32 nLen /*= -1 */ )
	{
		std::string sTemp;
		if( !pUnicode )
			return sTemp;
		if( nLen == -1 )
			nLen = (uint32)wcslen( pUnicode );
		sTemp.resize( nLen * 3 );
		UcsToAnsi( &sTemp[0], (uint32)sTemp.size(), pUnicode, nLen );
		return sTemp.c_str();
	}

}

#endif