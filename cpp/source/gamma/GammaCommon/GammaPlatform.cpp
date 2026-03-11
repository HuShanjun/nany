
#include "GammaCommon/GammaPlatform.h"
#include "GammaCommon/GammaCodeCvs.h"

#ifdef _WIN32
#include "Win32.h"
#elif ( defined _ANDROID )
#include "Android.h"
#elif ( defined _IOS )
#include "IOS.h"
#else
#include "Linux.h"
#endif

namespace Gamma
{
	GAMMA_COMMON_API uint64_t GetNativeModuleVersion()
	{
#ifdef _WIN32 
		return CWin32App::GetInstance().GetVersion();
#elif ( defined _ANDROID )
		return CAndroidApp::GetInstance().GetVersion();
#elif ( defined _IOS )
        return CIOSApp::GetInstance().GetVersion();
#endif
        return 0;
	}

	GAMMA_COMMON_API void* GetApplicationHandle()
	{
#ifdef _WIN32 
		return CWin32App::GetInstance().GetModuleHandle();
#elif ( defined _ANDROID )
        return CAndroidApp::GetInstance().GetApplication();
#elif ( defined _IOS )
		return CIOSApp::GetInstance().GetApplicationHandle();
#endif
        return nullptr;
	}

	GAMMA_COMMON_API void GetApplicationSignature( char szMd5[33] )
	{
#ifdef _WIN32 
		memset( szMd5, 0, 33 );
#elif ( defined _ANDROID )
		CAndroidApp::GetInstance().GetApplicationSignature( szMd5 );
#elif ( defined _IOS )
		memset( szMd5, 0, 33 );
#endif
	}

	GAMMA_COMMON_API void GetHardwareDesc( SHardwareDesc& HardwareDesc )
	{
#ifdef _WIN32 
		CWin32App::GetInstance().GetHardwareDesc( HardwareDesc );
#elif ( defined _ANDROID )
		CAndroidApp::GetInstance().GetHardwareDesc( HardwareDesc );
#elif ( defined _IOS )
		CIOSApp::GetInstance().GetHardwareDesc( HardwareDesc );
#else
		CLinuxApp::GetInstance().GetHardwareDesc( HardwareDesc );
#endif
	}

	GAMMA_COMMON_API bool IsWifiConnect()
	{
#ifdef _WIN32 
		return true;
#elif ( defined _ANDROID )
		return CAndroidApp::GetInstance().IsWifiConnect();
#elif ( defined _IOS )
		return CIOSApp::GetInstance().IsWifiConnect();
#endif
        return true;
	}

	GAMMA_COMMON_API void StartLocation( uint32_t nLocationInterval )
	{
#ifdef _WIN32
		return;
#elif ( defined _ANDROID )
		//return CAndroidApp::GetInstance().StartLocation( nLocationInterval );
#elif ( defined _IOS )
		return CIOSApp::GetInstance().StartLocation( nLocationInterval );
#endif
	}

	GAMMA_COMMON_API bool GetLocation( double& fLongitude, double& fLatitude, double& fAltitude )
	{
#ifdef _WIN32 
		return true;
#elif ( defined _ANDROID )
		return CAndroidApp::GetInstance().GetLocation( fLongitude, fLatitude, fAltitude );
#elif ( defined _IOS )
		return CIOSApp::GetInstance().GetLocation( fLongitude, fLatitude, fAltitude );
#endif
        return true;
	}

	GAMMA_COMMON_API void OpenURL( const char* szUrl )
	{
#ifdef _WIN32 
		CWin32App::GetInstance().OpenURL(szUrl);
#elif ( defined _IOS )
		CIOSApp::GetInstance().OpenURL( szUrl );
#elif ( defined _ANDROID )
		CAndroidApp::GetInstance().OpenURL( szUrl );
#endif
	}

	GAMMA_COMMON_API void SetClipboardContent( int32_t nType, const void* pContent, uint32_t nSize )
	{
		if( !pContent )
			return;
#ifdef _WIN32 
		CWin32App::GetInstance().SetClipboardContent( nType, pContent, nSize );
#elif ( defined _ANDROID )
		CAndroidApp::GetInstance().SetClipboardContent( nType, pContent, nSize );
#elif ( defined _IOS )
		CIOSApp::GetInstance().SetClipboardContent( nType, pContent, nSize );
#endif
	}

	GAMMA_COMMON_API void GetClipboardContent( int32_t nType, const void*& pContent, uint32_t& nSize )
	{
#ifdef _WIN32 
		CWin32App::GetInstance().GetClipboardContent( nType, pContent, nSize );
#elif ( defined _ANDROID )
		CAndroidApp::GetInstance().GetClipboardContent( nType, pContent, nSize );
#elif ( defined _IOS )
		CIOSApp::GetInstance().GetClipboardContent( nType, pContent, nSize );
#endif
	}

	GAMMA_COMMON_API bool GetSystemFile( int32_t nType, void* pContext, SystemFileCallback funCallback )
	{
		if( nType >= CONTENT_TYPE_COUNT )
			return false;
#ifdef _WIN32 
		return CWin32App::GetInstance().GetSystemFile( false, nType, pContext, funCallback );
#elif ( defined _ANDROID )
		return CAndroidApp::GetInstance().GetSystemFile( false, nType, pContext, (void*)funCallback );
#elif ( defined _IOS )
		return CIOSApp::GetInstance().GetSystemFile( nType, pContext, funCallback );
#endif
        return true;
	}

	GAMMA_COMMON_API bool GetSystemFileList( int32_t nType, void* pContext, SystemFileListCallback funCallback )
	{
		if( nType >= CONTENT_TYPE_COUNT )
			return false;
#ifdef _WIN32 
		return CWin32App::GetInstance().GetSystemFile( true, nType, pContext, funCallback );
#elif ( defined _ANDROID )
		return CAndroidApp::GetInstance().GetSystemFile( true, nType, pContext, (void*)funCallback );
#elif ( defined _IOS )
		return CIOSApp::GetInstance().GetSystemFileList( nType, pContext, funCallback );
#endif
        return true;
	}

	GAMMA_COMMON_API void* LoadDynamicLib( const char* szName )
	{
#ifdef _WIN32 
		return CWin32App::GetInstance().LoadDynamicLib( szName );
#elif ( defined _ANDROID )
		return CAndroidApp::GetInstance().LoadDynamicLib( szName );
#elif ( defined _IOS )
		return CIOSApp::GetInstance().LoadDynamicLib( szName );
#else
		return CLinuxApp::GetInstance().LoadDynamicLib( szName );
#endif
	}

	GAMMA_COMMON_API void* GetFunctionAddress( void* pLibContext, const char* szName )
	{
#ifdef _WIN32 
		return CWin32App::GetInstance().GetFunctionAddress( pLibContext, szName );
#elif ( defined _ANDROID )
		return CAndroidApp::GetInstance().GetFunctionAddress( pLibContext, szName );
#elif ( defined _IOS )
		return CIOSApp::GetInstance().GetFunctionAddress( pLibContext, szName );
#else
		return CLinuxApp::GetInstance().GetFunctionAddress( pLibContext, szName );
#endif
	}

#ifdef _WIN32 
	GAMMA_COMMON_API void SetPackagePath( const char* szPackagePath )
	{
		CWin32App::GetInstance().SetPackagePath( szPackagePath );
	}

	static void ConvertArg( const wchar_t** szArg, int32_t nArg, const char**& aryArg )
	{
		aryArg = new const char*[nArg];
		for( int32_t i = 0; i < nArg; i++ )
		{
			std::string strArg = UcsToUtf8( szArg[i] );
			char* szArg = new char[ strArg.size() + 1 ];
			memcpy( szArg, strArg.c_str(), strArg.size() + 1 );
			aryArg[i] = szArg;
		}
	}

	GAMMA_COMMON_API int32_t StartApp( AppEntryFunction funEntry, int nArg, const wchar_t* szArg[] )
	{
		const char** aryArg = NULL;
		ConvertArg( szArg, nArg, aryArg );
		int32_t nRet = funEntry( nArg, aryArg );
		for( int32_t i = 0; i < nArg; i++ )
			SAFE_DEL_GROUP( aryArg[i] );
		SAFE_DEL_GROUP( aryArg );
		return nRet;
	}

	static void ConvertArg( wchar_t* szCmdLine, int32_t& nArg, const char**& aryArg )
	{
		std::vector<std::wstring> vecArg = CmdParser( szCmdLine ); 
		nArg = (int32_t)vecArg.size() + 1;
		aryArg = new const char*[nArg];
		aryArg[0] = new char(0);

		for( uint32_t i = 0; i < vecArg.size(); i++ )
		{
			std::string strArg = UcsToUtf8( vecArg[i] );
			char* szArg = new char[ strArg.size() + 1 ];
			memcpy( szArg, strArg.c_str(), strArg.size() + 1 );
			aryArg[i+1] = szArg;
		}
	}

	GAMMA_COMMON_API int32_t StartApp( AppEntryFunction funEntry, void* pApp, wchar_t* szCmdLine )
	{
		int32_t nArg = 0;
		const char** aryArg = NULL;
		ConvertArg( szCmdLine, nArg, aryArg );
		int32_t nRet = funEntry( nArg, aryArg );
		for( int32_t i = 0; i < nArg; i++ )
			SAFE_DEL_GROUP( aryArg[i] );
		SAFE_DEL_GROUP( aryArg );
		return nRet;
	}
#else
	GAMMA_COMMON_API int32_t StartApp( AppEntryFunction funEntry, int nArg, const char* szArg[] )
	{
#ifdef _IOS
        return CIOSApp::GetInstance().StartApp( funEntry, nArg, szArg );
#else
		return funEntry( nArg, szArg );
#endif		
	}

#ifdef _ANDROID 
	GAMMA_COMMON_API JavaVM* GetJavaVM()
	{
		return CAndroidApp::GetInstance().GetJavaVM();
	}
	
	GAMMA_COMMON_API void* GetMainActivity()
	{
		return CAndroidApp::GetInstance().GetActivity();
	}

#endif
#endif

	GAMMA_COMMON_API void	SetConsoleOutputUTF8()
	{
#ifdef _WIN32
		SetConsoleOutputCP(65001);
		SetConsoleCP(65001);
#endif
	}
}
