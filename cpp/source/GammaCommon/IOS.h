//
//  IOS.h
//  TestOpenGL
//
//  Created by daphnis on 15-1-8.
//  Copyright (c) 2015年 daphnis. All rights reserved.
//

#ifndef _GAMMA_IOS_H_
#define _GAMMA_IOS_H_

#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/GammaPlatform.h"
#include "GammaCommon/IGammaFileMgr.h"
#include "GammaCommon/TGammaList.h"
#include "GammaCommon/CThread.h"

#ifdef _IOS
namespace Gamma
{
	class CFileReader;
	typedef TGammaList<CFileReader>	CAsynReadList;
	
    //==================================================
    // IOS 窗口类，用于连接GammaWindow和ios窗口的中间层
    //==================================================
    class SIOSWnd;
    typedef int32_t (*InputHandler)( void*, uint32_t, uint32_t, uint32_t, uint32_t );
	#define WM_LOW_MEMORY ( WM_USER + 0x504 )
    
    //==================================================
    // IOS 全局类，用于管理全局数据
    //==================================================
    class CIOSApp
    {
		typedef std::map<uint32_t, SIOSWnd> CWindowsMap;
		
		uint64_t				m_nVersion;
		SHardwareDesc		m_HardwareDesc;
        AppEntryFunction    m_funEntry;
        int32_t               m_nArg;
        const char**        m_szArg;
        
        uint32_t              m_nWindowID;
        CWindowsMap         m_mapWindows;
        std::string			m_strPackage;
        std::string			m_strCache;
		
		int64_t				m_nPreLocationTime;
		uint32_t				m_nLocationInterval;
		double				m_fLongitude;
		double				m_fLatitude;
		double				m_fAltitude;
        
        CIOSApp();
        ~CIOSApp();
		
		void*				Init( AppEntryFunction funEntry, int nArg, const char* szArg[] );
		void				FetchHardwareInfo();
        
    public:
        static CIOSApp&     GetInstance();
        static void*        GetNativeHandler( SIOSWnd* pWnd );
		
		uint64_t				GetVersion();
		void*				GetApplicationHandle();
		void				GetHardwareDesc( SHardwareDesc& HardwareDesc );
		void				OpenURL( const char* szUrl );
		bool				IsWifiConnect();
		void 				SetClipboardContent( int32_t nType, const void* pContent, uint32_t nSize );
		void 				GetClipboardContent( int32_t nType, const void*& pContent, uint32_t& nSize );
		bool 				GetSystemFile( int32_t nType, void* pContext, SystemFileCallback funCallback );
		bool 				GetSystemFileList( int32_t nType, void* pContext, SystemFileListCallback funCallback );
		
		void 				StartLocation( uint32_t nLocationInterval );
		void				SetLocation( double fLongitude, double fLatitude, double fAltitude );
		bool 				GetLocation( double& fLongitude, double& fLatitude, double& fAltitude );
		
        int32_t               MainThread();
        int32_t               StartApp( AppEntryFunction funEntry, int nArg, const char* szArg[] );
        
        SIOSWnd*            CreateIOSGLView( InputHandler funHandler, void* pContext );
        void                DestroyIOSGLView( SIOSWnd* pWnd );
		void                EnableInput( SIOSWnd* pWnd, bool bEnable, bool bFullScreen, const wchar_t* szInitText );
        
        const char*         GetPackagePath(){ return m_strPackage.c_str(); }
        const char*         GetCachePath(){ return m_strCache.c_str(); }
		bool				ReadSystemFile( CFileReader* pReader, HLOCK hLock, CAsynReadList* listFinished );
		void*				LoadDynamicLib( const char* szName ) { return NULL; }
		void*				GetFunctionAddress( void* pLibContext, const char* szName ) { return NULL; }
        
        uint32_t              IOSMessagePump();
        void                OnIOSMessage( void* msg );
    };
}
#endif

#endif
