//===============================================
// GammaPlatform.h 
// 定义平台函数
// 柯达昭
// 2007-09-07
//===============================================

#ifndef __GAMMA_PLATFORM_H__
#define __GAMMA_PLATFORM_H__

#ifdef _WIN32
#include <direct.h>
#include <winerror.h>
#include <Windows.h>
#elif defined _ANDROID
#include <dirent.h>
#include <unistd.h>
#include <android/sensor.h>
#include <android/input.h>
#include <android/configuration.h>
#include <android/looper.h>
#include <android/native_activity.h>
#else
#include <dirent.h>
#endif

#include "GammaCommon/GammaCommonType.h"
#include <string.h>

namespace Gamma
{
	typedef int32_t (*AppEntryFunction)( int32_t, const char** );

	#define CPUTYPE_LEN	64
	#define DEVICEDESC_LEN 64
	#define OSDESC_LEN 64
	#define UUID_LEN 64
	#define LANGUAGE_LEN 8

	struct SHardwareDesc 
	{
		SHardwareDesc() { memset( this, 0, sizeof(SHardwareDesc) ); }
		char	m_szDeviceDesc[DEVICEDESC_LEN];
		char	m_szCpuType[CPUTYPE_LEN];
		char	m_szOSDesc[OSDESC_LEN];
		char	m_szUUID[UUID_LEN];
		char	m_szLanguage[LANGUAGE_LEN];
		uint64_t	m_nMac;
		uint32_t	m_nCpuFrequery;
		uint32_t	m_nCpuCount;
		uint32_t	m_nMemSize;
		uint16_t  m_nScreen_X;
		uint16_t	m_nScreen_Y;
	};

	GAMMA_COMMON_API uint64_t GetNativeModuleVersion();
	GAMMA_COMMON_API void*  GetApplicationHandle();
	GAMMA_COMMON_API void	GetApplicationSignature( char szMd5[33] );
	GAMMA_COMMON_API void	GetHardwareDesc( SHardwareDesc& HardwareDesc );
	GAMMA_COMMON_API bool	IsWifiConnect();
	GAMMA_COMMON_API void	OpenURL( const char* szUrl );
	
	GAMMA_COMMON_API void	StartLocation( uint32_t nLocationInterval );
	GAMMA_COMMON_API bool	GetLocation( double& fLongitude, double& fLatitude, double& fAltitude );

	#define CONTENT_TYPE_TEXT	0
	#define CONTENT_TYPE_IMAGE	1
	#define CONTENT_TYPE_MUSIC	2
	#define CONTENT_TYPE_VIDEO	3
	#define CONTENT_TYPE_COUNT	4

	typedef void (*SystemFileCallback)( void* pContext, const char* szPhysicPath );
	typedef void (*SystemFileListCallback)( void* pContext, const char** szPhysicPath, uint32_t nCount );
	GAMMA_COMMON_API void	SetClipboardContent( int32_t nType, const void* pContent, uint32_t nSize );
	GAMMA_COMMON_API void	GetClipboardContent( int32_t nType, const void*& pContent, uint32_t& nSize );
	GAMMA_COMMON_API bool	GetSystemFile( int32_t nType, void* pContext, SystemFileCallback funCallback );
	GAMMA_COMMON_API bool	GetSystemFileList( int32_t nType, void* pContext, SystemFileListCallback funCallback );
	GAMMA_COMMON_API void*	LoadDynamicLib( const char* szName );
	GAMMA_COMMON_API void*	GetFunctionAddress( void* pLibContext, const char* szName );

#ifdef _WIN32
	GAMMA_COMMON_API void	SetPackagePath( const char* szPackagePath );
	GAMMA_COMMON_API int32_t	StartApp( AppEntryFunction funEntry, int nArg, const wchar_t* szArg[] );
	GAMMA_COMMON_API int32_t	StartApp( AppEntryFunction funEntry, void* pApp, wchar_t* szCmdLine );
	#define main gamma_main( int, const char** );\
		int32_t wmain( int nArg, const wchar_t* szArg[] )\
		{ return Gamma::StartApp( gamma_main, nArg, szArg ); } \
		int32_t WINAPI wWinMain( HINSTANCE hInst, HINSTANCE, wchar_t* szCmd, int )\
		{ return Gamma::StartApp( gamma_main, hInst, szCmd ); } \
		int gamma_main
#else
	#ifdef _ANDROID 
	GAMMA_COMMON_API JavaVM* GetJavaVM();
	GAMMA_COMMON_API void* GetMainActivity();
	#endif
	GAMMA_COMMON_API int32_t	StartApp( AppEntryFunction funEntry, int nArg, const char* szArg[] );
	#define main gamma_main( int, const char** );\
		int32_t main( int nArg, const char* szArg[] ) \
		{ return Gamma::StartApp( gamma_main, nArg, szArg ); } \
		int gamma_main
#endif
}

#endif
