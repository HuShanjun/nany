//===============================================
// Android.h 
// 定义平台函数
// 柯达昭
// 2007-09-07
//===============================================

#ifndef __GAMMA_ANDROID_H__
#define __GAMMA_ANDROID_H__

#ifdef _ANDROID
#include "GammaCommon/CThread.h"
#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/TGammaList.h"
#include "GammaCommon/TGammaRBTree.h"
#include <string>
#include <map>

using namespace std;

#define WM_LOW_MEMORY	( WM_USER + 0x0504 )

namespace Gamma
{
	typedef int32_t (*InputHandler)( void*, uint32_t, uint32_t, uint32_t, uint32_t );

	class CAndroidApp
	{
		enum
		{
			LOOPER_ID_MAIN 	= 1,
			LOOPER_ID_INPUT = 2,
		};

		enum EActivityState
		{
			APP_CMD_INPUT_CHANGED			= 0,
			APP_CMD_INIT_WINDOW				= 1,
			APP_CMD_TERM_WINDOW				= 2,
			APP_CMD_WINDOW_RESIZED			= 3,
			APP_CMD_WINDOW_REDRAW_NEEDED	= 4,
			APP_CMD_CONTENT_RECT_CHANGED	= 5,
			APP_CMD_GAINED_FOCUS			= 6,
			APP_CMD_LOST_FOCUS				= 7,
			APP_CMD_CONFIG_CHANGED			= 8,
			APP_CMD_LOW_MEMORY				= 9,
			APP_CMD_START					= 10,
			APP_CMD_RESUME					= 11,
			APP_CMD_SAVE_STATE				= 12,
			APP_CMD_PAUSE					= 13,
			APP_CMD_STOP					= 14,
			APP_CMD_DESTROY					= 15,
		};

		struct SFileOpenContext;
		typedef std::vector<std::string> CFileNameList;
		typedef TGammaList<SFileOpenContext> CFileOpenContextList;
		typedef TGammaRBTree<SFileOpenContext> CFileOpenContextMap;
		struct SFileOpenContext 
			: public CFileOpenContextMap::CGammaRBTreeNode
			, public CFileOpenContextList::CGammaListNode
		{
			uint32_t				m_nID;
			int32_t				m_nType;
			void*				m_pContext;
			void*				m_funCallback;
			CFileNameList		m_vecFileName;
			operator uint32_t() const { return m_nID; }
		};

		char					m_szExternalPath[2048];
		char					m_szPackagePath[2048];
		int32_t					m_nSDKInit;
		uint64_t					m_nVersion;
		HLOCK					m_hInputLock;
		vector<uint16_t>			m_vecInputBuffer;
		bool					m_bFullScreenInput;
		SHardwareDesc			m_HardwareDesc;
		map<void*,InputHandler> m_mapHandlers;

		// The global java VM.
		JavaVM*					m_pJavaVM;

		// The global Application object.
		jobject					m_pApplication;

		// The Input sensor manager.
		ASensorManager*			m_pSensorMgr;		

		// The Input event queue.
		ASensorEventQueue*		m_pSensorEventQueue;

		// When non-NULL, this is the input queue from which the app will
		// receive user input events.
		AInputQueue*			m_pInputQueue;	

		// The ANativeActivity object instance that this app is running in.
		ANativeActivity*		m_pActivity;

		// When non-NULL, this is the window surface that the app can draw in.
		ANativeWindow*			m_pNativeWindow;

		// The current configuration the app is running in.
		AConfiguration*			m_pConfig;

		// This is the last instance's saved state, as provided at creation time.
		// It is NULL if there was no state.  You can use this as you need; the
		// memory will remain around until you call android_app_exec_cmd() for
		// APP_CMD_RESUME, at which point it will be freed and savedState set to NULL.
		// These variables should only be changed when processing a APP_CMD_SAVE_STATE,
		// at which point they will be initialized to NULL and you can malloc your
		// state and place the information here.  In that case the memory will be
		// freed for you later.
		void*					m_pSavedState;
		size_t					m_nSavedStateSize;

		// Current state of the app's activity.  May be either APP_CMD_START,
		// APP_CMD_RESUME, APP_CMD_PAUSE, or APP_CMD_STOP; see below.
		EActivityState			m_eActivityState;

		// This is non-zero when the application's NativeActivity is being
		// destroyed and waiting for the app thread to complete.
		bool					m_bDestroyRequested;

		// MainThread Running. multiply activity associate one thread;
		bool					m_bMainThreadRunning;

		// Activity is resume
		bool					m_bActivityRunning;

		// State data saved.
		bool					m_bStateSaved;

		// Activity is destroyed.
		bool					m_bDestroyed;

		// Input is visible.
		bool					m_bInputVisible;

		// -------------------------------------------------
		// Below are "private" implementation of the glue code.
		pthread_mutex_t			m_Mutex;
		pthread_cond_t			m_Condiction;
		pthread_t				m_threadMain;
		void**					m_pMaxMainStack;
		size_t					m_nMainStackSize;

		int						m_nCommandRead;
		int						m_nCommandWrite;

		AInputQueue*			m_pPendingInputQueue;
		ANativeWindow*			m_pPendingWindow;

		uint8_t					m_arySignatureMd5[33];

		CLock					m_Lock;
		uint8_t					m_nSysFileID;
		CFileOpenContextList	m_listFileContext;
		CFileOpenContextMap		m_mapFileContext;


		CAndroidApp();
		~CAndroidApp();

		int32_t					DispatchMessage( uint32_t nInputID, uint32_t nMsg, uint32_t wParam, uint32_t lParam );
		int32_t 					AndroidInputHandler( AInputEvent* event );
		void 					AndroidCmdHandler( int32_t nCmd );
		void					FetchHardwareInfo();

		void*					SaveInstanceState( size_t* &outLen );
		void					FreeSavedState();
		int8_t					ReadCommand();
		void					PreExecCommand( int8_t nCmd );
		void					PostExecCommand( int8_t nCmd );
		int32_t					ProcessCommand();
		int32_t					ProcessInput();
		int32_t					MainThread();

		// --------------------------------------------------------------------
		// Native activity interaction (called from main thread)
		// --------------------------------------------------------------------
		void					WriteCommand( int8_t nCmd );
		void					SetInput( AInputQueue* inputQueue);
		void					SetWindow( ANativeWindow* window);
		void					SetActivityState( int8_t cmd );
		void					Free();
		void 					OnSystemFileCallback( JNIEnv* env, int32_t nID,
									int32_t nResult, jobject Intent );
		void 					FinishedSystemFileCallback( SFileOpenContext* pFileContext );

		static void				OnDestroy( ANativeActivity* pActivity );
		static void				OnStart( ANativeActivity* pActivity );
		static void				OnResume( ANativeActivity* pActivity );
		static void*			OnSaveInstanceState( ANativeActivity* pActivity, size_t* outLen );

		static void				OnPause( ANativeActivity* pActivity );
		static void				OnStop( ANativeActivity* pActivity );
		static void				OnConfigurationChanged( ANativeActivity* pActivity );
		static void				OnLowMemory( ANativeActivity* pActivity );
		static void				OnWindowFocusChanged( ANativeActivity* pActivity, int focused );
		static void				OnNativeWindowCreated( ANativeActivity* pActivity, ANativeWindow* window );
		static void				OnNativeWindowDestroyed( ANativeActivity* pActivity, ANativeWindow* window );
		static void				OnNativeWindowResized( ANativeActivity* pActivity, ANativeWindow* window );
		static void				OnInputQueueCreated( ANativeActivity* pActivity, AInputQueue* queue );
		static void				OnInputQueueDestroyed( ANativeActivity* pActivity, AInputQueue* queue );

	public:
		static CAndroidApp&		GetInstance();	
		JavaVM*					GetJavaVM() const { return m_pJavaVM; }
		void*					GetApplication() const { return m_pApplication; }
		ANativeActivity*		GetActivity() const { return m_pActivity; }
		ANativeWindow*			GetNativeWindow() const { return m_pNativeWindow; }
		void					GetHardwareDesc( SHardwareDesc& HardwareDesc ){ HardwareDesc = m_HardwareDesc; }
		void**					GetMaxMainStack() const { return m_pMaxMainStack; }
		size_t					GetMainStackSize() const { return m_nMainStackSize; }
		bool					IsWifiConnect();
		bool					GetLocation( double& fLongitude, double& fLatitude, double& fAltitude );

		void					Run( ANativeActivity* pActivity, void* pSavedState, size_t nSavedStateSize );
		void					AddCharMsgFromJava( const jchar* szBuffer, uint32_t nSize );
		void					OnInputMgrShowFromJava( bool bShow );
		void 					OnActivityResult( JNIEnv* env, int32_t nID, int32_t nResult, jobject Intent );
		
		const char*				GetExternalPath();
		const char*				GetPackagePath();
		uint64_t					GetVersion() const;
		void					EnableInput( bool bEnable, const wchar_t* szFullScreenInitText );
		void					RegisterMsgHandler( void* pContext, InputHandler handler );
		int32_t					AndroidMessagePump();
		void					SetClipboardContent( int32_t nType, const void* pContent, uint32_t nSize );
		void					GetClipboardContent( int32_t nType, const void*& pContent, uint32_t& nSize );
		void					GetApplicationSignature( char szMd5[33] );
		void					OpenURL( const char* szUrl );
		bool 					GetSystemFile( bool bList, int32_t nType, void* pContext, void* funCallback );
		void*					LoadDynamicLib( const char* szName );
		void*					GetFunctionAddress( void* pLibContext, const char* szName );
	};
}

#endif
#endif
