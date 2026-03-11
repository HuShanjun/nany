
#ifdef __linux__
#include "Linux.h"
#include <unistd.h>
#include <stdio.h> 
#include <sys/ioctl.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <net/if.h> 
#include <stdlib.h>
#include <dlfcn.h>

namespace Gamma
{ 
	CLinuxApp::CLinuxApp(void)
	{
		FetchHardwareInfo();
	}
	
	CLinuxApp::~CLinuxApp(void)
	{
	}

	CLinuxApp& CLinuxApp::GetInstance()
	{
		static CLinuxApp s_instance;
		return s_instance;
	}

	void CLinuxApp::FetchHardwareInfo()
	{
		/*
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
		uint16_t	m_nScreen_Y; */

		m_HardwareDesc.m_nScreen_X = 0;
		m_HardwareDesc.m_nScreen_Y = 0;

		int nLen = DEVICEDESC_LEN;
		char szComputerName[DEVICEDESC_LEN];
		gethostname( szComputerName, nLen );
		strcpy2array_safe( m_HardwareDesc.m_szDeviceDesc, szComputerName );

		FILE* fp;
		char szTempBuffer[2048];

		if( NULL != ( fp = fopen( "/proc/version", "r" ) ) )
		{
			//Linux version 3.10.0-123.el7.x86_64 (builder@kbuilder.dev.centos.org) (gcc version 4.8.2 20140120 (Red Hat 4.8.2-16) (GCC) ) #1 SMP Mon Jun 30 12:09:22 UTC 2014
			if( fgets( szTempBuffer, 2048, fp ) )
				strcpy2array_safe( m_HardwareDesc.m_szOSDesc, szTempBuffer );
			fclose( fp );
		}

		struct ifreq ifr;  
		int			nSock = socket( AF_INET, SOCK_STREAM, 0 );
		if( nSock >= 0 ) 
		{ 
			 ifr.ifr_addr.sa_family = AF_INET;  
			 strncpy( ifr.ifr_name, "eth0", IFNAMSIZ - 1 );
			 ioctl( nSock, SIOCGIFHWADDR, &ifr );
			 close( nSock );
			 uint8_t* nMac = (uint8_t*)&m_HardwareDesc.m_nMac;
			 nMac[0] = ValueFromHexNumber( (unsigned char)ifr.ifr_hwaddr.sa_data[0] );
			 nMac[1] = ValueFromHexNumber( (unsigned char)ifr.ifr_hwaddr.sa_data[1] );
			 nMac[2] = ValueFromHexNumber( (unsigned char)ifr.ifr_hwaddr.sa_data[2] );
			 nMac[3] = ValueFromHexNumber( (unsigned char)ifr.ifr_hwaddr.sa_data[3] );
			 nMac[4] = ValueFromHexNumber( (unsigned char)ifr.ifr_hwaddr.sa_data[4] );
			 nMac[5] = ValueFromHexNumber( (unsigned char)ifr.ifr_hwaddr.sa_data[5] );
			 sprintf( m_HardwareDesc.m_szUUID, "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",  
				 (unsigned char)ifr.ifr_hwaddr.sa_data[0],
				 (unsigned char)ifr.ifr_hwaddr.sa_data[1],
				 (unsigned char)ifr.ifr_hwaddr.sa_data[2],
				 (unsigned char)ifr.ifr_hwaddr.sa_data[3],
				 (unsigned char)ifr.ifr_hwaddr.sa_data[4],
				 (unsigned char)ifr.ifr_hwaddr.sa_data[5] );
		}

		const char* szLanguage = getenv( "LANG" );
		if ( szLanguage )
			strcpy2array_safe( m_HardwareDesc.m_szLanguage, szLanguage );

		char szBuffer[1024];
		if( NULL != ( fp = fopen( "/proc/cpuinfo", "r" ) ) )
		{
			while ( fgets( szTempBuffer, 2048, fp ) )
			{
				size_t n = strlen( szTempBuffer );
				while( IsBlank( szTempBuffer[n-1] ) )
					szTempBuffer[--n] = 0;
				const char* szFind = strchr( szTempBuffer, ':' );
				if ( !szFind )
					continue;
				while( ++szFind && IsBlank( szFind[0] ) && szFind[0] != '\0' );
				if( strstr( szTempBuffer, "model name" ) )
				{
					strcpy2array_safe( m_HardwareDesc.m_szCpuType, szFind );
				}
				else if ( strstr( szTempBuffer, "cpu MHz" ) )
				{
					strcpy2array_safe( szBuffer, szFind );
					m_HardwareDesc.m_nCpuFrequery = (uint32_t)atof( szBuffer );
				}
				else if ( strstr( szTempBuffer, "cpu cores" ) )
				{
					strcpy2array_safe( szBuffer, szFind );
					m_HardwareDesc.m_nCpuCount = (uint32_t)atof( szBuffer );
				}
			}
			fclose( fp );
		}

		if( NULL != ( fp = fopen( "/proc/meminfo", "r" ) ) )
		{
			while( fgets( szTempBuffer, 2048, fp ) )
			{
				size_t n = strlen( szTempBuffer );
				while( IsBlank( szTempBuffer[n-1] ) )
					szTempBuffer[--n] = 0;
				const char* szFind = strchr( szTempBuffer, ':' );
				if ( !szFind )
					continue;
				while( ++szFind && IsBlank( szFind[0] ) && szFind[0] != '\0' );
				if( strstr( szTempBuffer, "MemTotal" ) )
				{
					strcpy2array_safe( szBuffer, szFind );
					m_HardwareDesc.m_nMemSize = (uint32_t)atof( szBuffer );
				}
				fclose( fp );
				break;
			}
		}
	}

	void* CLinuxApp::LoadDynamicLib( const char* szName )
	{		
		std::string strName = szName;
		if( strName.find( ".so" ) == std::wstring::npos )
			strName += ".so";
		void* pHandle = dlopen( strName.c_str(), RTLD_NOW|RTLD_GLOBAL ); // 在全局空间加载
		if( pHandle )
			return pHandle;
		GammaLog << dlerror() << std::endl;
		return NULL;
	}

	void* CLinuxApp::GetFunctionAddress( void* pLibContext, const char* szName )
	{
		return dlsym( RTLD_DEFAULT, szName );
	}
}
#endif