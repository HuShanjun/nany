
#include "GammaCommon/CGammaWindow.h"
#include "GammaCommon/GammaCodeCvs.h"
#include "GammaCommon/TTinyList.h"
#include "GammaCommon/TRect.h"
#include "CAniCursorFile.h"

#ifdef _ANDROID
#include "Android.h"
#elif ( defined _IOS || defined _MAC )
#include "IOS.h"
#else
#include "Win32.h"
#ifdef _WIN32
#include <tchar.h>
#endif
#endif

namespace Gamma
{
	struct SWindowContext;
	typedef TTinyList<SWindowContext> CWinList;
	CWinList g_mapWindowList;
	#define INVALID_LOCK_RECT \
	CIRect( MIN_INT16, MIN_INT16, MAX_INT16, MAX_INT16 )

	struct SCallback
	{
		WndMsgCallback 			m_funCallback;
		void* 					m_pContext;
	};

	struct SWindowContext : public CWinList::CTinyListNode
	{
		SWindowContext( CGammaWindow* pWnd )
			: m_pWnd( pWnd )
			, m_pCurCursorFile( NULL )
			, m_pContext( NULL )
			, m_pHandler( NULL )
#ifdef _WIN32
			, m_pOrgWndProc( NULL )
			, m_bTimerRegister( false )
			, m_bLockCursor( false )
			, m_rtLock( INVALID_LOCK_RECT )
#elif ( defined _ANDROID )
			, m_bHide( false )
			, m_bActive( false )
			, m_bFocus( false )
			, m_bCreated( false )
#elif ( defined _IOS )
			, m_bHide( true )
			, m_bActive( true )
			, m_bFocus( false )
			, m_bCreated( false )
#endif
		{
		}

		CGammaWindow*			m_pWnd;
		void*					m_pContext;
		void*					m_pHandler;
		CAniCursorFile*			m_pCurCursorFile;
		CPos					m_posCursor;
		std::vector<SCallback>	m_vecCallbackInfo;

#ifdef _WIN32
		WNDPROC					m_pOrgWndProc;
		bool					m_bTimerRegister;
		bool					m_bLockCursor;
		CIRect					m_rtLock;
#elif ( defined _ANDROID || defined _IOS )
		bool					m_bHide;
		bool					m_bActive;
		bool					m_bFocus;
		bool					m_bCreated;
		CIRect					m_rtWnd;
#endif

#ifdef _WIN32
		static LRESULT WINAPI WindowProc( HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam );
		static VOID CALLBACK TimerProc( HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime );
		static void CheckClipCursor( CGammaWindow* pWnd, bool bIsFocus );
#elif ( defined _ANDROID || defined _IOS )
		bool IsFocus() { return m_bFocus && m_bActive; }
        static int32_t ProcessInput( CGammaWindow* pWnd, uint32_t nInputID, uint32_t nMsg, uint32_t wParam, uint32_t lParam );
#endif
	};

	int32_t CGammaWindow::MessagePump()
	{
#ifdef _WIN32
		return CWin32App::GetInstance().WindowMessagePump();
#elif ( defined _ANDROID )
        return CAndroidApp::GetInstance().AndroidMessagePump();
#elif ( defined _IOS )
        return CIOSApp::GetInstance().IOSMessagePump();
#else
		return 0;
#endif
	}


	CGammaWindow::CGammaWindow()
	{
		m_pContext = new SWindowContext( this );
	}

	CGammaWindow::~CGammaWindow()
	{
		Destroy();
		delete m_pContext;
	}	

#ifdef _WIN32
	void CGammaWindow::Initialize( void* pContext, void* pParent, 
		uint32_t nWidth, uint32_t nHeight, const char* szTitle, uint32_t nIconID)
	{
		HWND hParentWnd = ::IsWindow( (HWND)pParent ) ? (HWND)pParent : (HWND)NULL;
		DWORD dwStyle		= hParentWnd ? WS_CHILD | WS_VISIBLE : WS_OVERLAPPEDWINDOW;
		DWORD dwStyleEx		= hParentWnd ? 0 : WS_EX_APPWINDOW;

		WNDCLASSEX wcex;
		wcex.cbSize			= sizeof(WNDCLASSEX); 
		wcex.style			= CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc	= (WNDPROC)&SWindowContext::WindowProc;
		wcex.cbClsExtra		= 0;
		wcex.cbWndExtra		= 0;
		wcex.hInstance		= (HINSTANCE)CWin32App::GetInstance().GetModuleHandle();
		wcex.hIcon			= NULL;
		wcex.hCursor		= LoadCursor( NULL, IDC_ARROW );
		wcex.hbrBackground	= NULL;
		wcex.lpszMenuName	= NULL;
		wcex.lpszClassName	= _T("GammaWindow");
		wcex.hIconSm		= NULL;

		RegisterClassEx(&wcex);

		RECT rtClientRect	 = { 0, 0, (int32_t)nWidth, (int32_t)nHeight };
		if( hParentWnd )
			::GetClientRect( hParentWnd, &rtClientRect );

		m_pContext->m_pContext = pContext;

#ifdef _UNICODE
		m_pContext->m_pHandler = CreateWindowExW( dwStyleEx, wcex.lpszClassName, 
			Utf8ToUcs( szTitle ).c_str(), dwStyle, 0, 0,
			rtClientRect.right - rtClientRect.left, rtClientRect.bottom - rtClientRect.top,
			hParentWnd, NULL, wcex.hInstance, this );
#else
		m_pContext->m_pHandler = CreateWindowExA( dwStyleEx, wcex.lpszClassName, 
			UcsToAnsi( Utf8ToUcs( szTitle ).c_str() ).c_str(), dwStyle, 0, 0,
			rtClientRect.right - rtClientRect.left, rtClientRect.bottom - rtClientRect.top,
			hParentWnd, NULL, wcex.hInstance, this );
#endif

		if( hParentWnd == NULL )
		{
			AdjustWindowRectEx( &rtClientRect, 
				(DWORD)GetWindowLong( (HWND)GetHandle(), GWL_STYLE ),
				GetMenu( (HWND)GetHandle() ) != NULL, 
				(DWORD)GetWindowLong( (HWND)GetHandle(), GWL_EXSTYLE ) );

			int32_t cx = GetSystemMetrics( SM_CXFULLSCREEN );
			int32_t cy = GetSystemMetrics( SM_CYFULLSCREEN );

			nWidth = rtClientRect.right - rtClientRect.left;
			nHeight = rtClientRect.bottom - rtClientRect.top;
			int32_t xPos = ( cx - (int32_t)nWidth ) / 2;
			int32_t yPos = ( cy - (int32_t)nHeight ) / 2;

			MoveWindow( (HWND)GetHandle(), xPos, yPos, nWidth, nHeight, FALSE );
		}

		UpdateWindow( (HWND)GetHandle() );
		OnCreated();
	}

	VOID CALLBACK SWindowContext::TimerProc( HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime )
	{
		SWindowContext* pContext = g_mapWindowList.GetFirst();
		while( pContext && pContext->m_pHandler != hWnd )
			pContext = pContext->GetNext();
		if( !pContext )
			return;
		pContext->m_pWnd->UpdateCursor();
	}

	LRESULT WINAPI SWindowContext::WindowProc( HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam )
	{
		if( nMsg == WM_CREATE )
		{
			void* pParam = ((LPCREATESTRUCT)lParam)->lpCreateParams;
			CGammaWindow* pWnd = (CGammaWindow*)pParam;
			g_mapWindowList.PushFront( *pWnd->m_pContext );
			return 0;
		}

		SWindowContext* pContext = g_mapWindowList.GetFirst();
		while( pContext && pContext->m_pHandler != hWnd )
			pContext = pContext->GetNext();
		CGammaWindow* pWin = pContext ? pContext->m_pWnd : NULL;
		if( nMsg == WM_MOUSEWHEEL )
		{
			POINT ptCursor = { (short)LOWORD( lParam ), (short)HIWORD( lParam ) };
			::ScreenToClient( (HWND)pWin->GetHandle(), &ptCursor );
			lParam = MAKELONG( ptCursor.x, ptCursor.y );
		}

		if( !pWin )
			return DefWindowProc( hWnd, nMsg, wParam, lParam );
		if( pContext->m_bLockCursor && nMsg == WM_MOUSEMOVE && 
			LOINT16( lParam ) == (int32_t)pWin->GetClientWidth()/2 &&
			HIINT16( lParam ) == (int32_t)pWin->GetClientHeight()/2 )
			return DefWindowProc( hWnd, nMsg, wParam, lParam );
		bool bProcessResult = pWin->OnProcessMsg( 0, nMsg, (uint32_t)wParam, (uint32_t)lParam );
		if( pContext->m_bLockCursor && nMsg == WM_MOUSEMOVE && ::GetFocus() == hWnd )
		{
			int32_t xPos = pWin->GetClientWidth()/2;
			int32_t yPos = pWin->GetClientHeight()/2;
			pWin->SetCursorPos( CPos( xPos, yPos ) );
		}

		if( bProcessResult )
			return 0;
		if( pWin->m_pContext->m_pOrgWndProc )
			return pWin->m_pContext->m_pOrgWndProc( hWnd, nMsg, (uint32_t)wParam, (uint32_t)lParam );
		return DefWindowProc( hWnd, nMsg, wParam, lParam );
	}

	void SWindowContext::CheckClipCursor( CGammaWindow* pWnd, bool bIsFocus )
	{
		if( pWnd->m_pContext->m_rtLock == INVALID_LOCK_RECT )
			return;

		if( !bIsFocus )
		{
			::ClipCursor( NULL );
			return;
		}

		RECT rtScreen;
		rtScreen.left = pWnd->m_pContext->m_rtLock.left;
		rtScreen.right = pWnd->m_pContext->m_rtLock.right;
		rtScreen.top = pWnd->m_pContext->m_rtLock.top;
		rtScreen.bottom = pWnd->m_pContext->m_rtLock.bottom;
		::ClientToScreen( (HWND)pWnd->m_pContext->m_pHandler, (POINT*)&rtScreen.left );
		::ClientToScreen( (HWND)pWnd->m_pContext->m_pHandler, (POINT*)&rtScreen.right );
		::ClipCursor( (const RECT*)&rtScreen );
	}

#elif ( defined _ANDROID )
	void CGammaWindow::Initialize( void* pContext, void* pParent, uint32_t nWidth, uint32_t nHeight, const char* szTitle, uint32_t nIconID )
	{
		GammaAst( CAndroidApp::GetInstance().GetActivity() );
		g_mapWindowList.PushFront( *m_pContext );
		m_pContext->m_pContext = pContext;
		m_pContext->m_pHandler = CAndroidApp::GetInstance().GetNativeWindow();
		CAndroidApp::GetInstance().RegisterMsgHandler( this, (InputHandler)&SWindowContext::ProcessInput );
		if( !m_pContext->m_pHandler )
			return;
		SWindowContext::ProcessInput( this, 0, WM_CREATE, 0, 0 );
	}

	//初始化Native窗口，必须在这里初始化EGL，将OpenGL ES版本指定为2.0
	int32_t SWindowContext::ProcessInput( CGammaWindow* pWnd, uint32_t nInputID, uint32_t nMsg, uint32_t wParam, uint32_t lParam )
	{
		STATCK_LOG( pWnd );
		CAndroidApp& App = CAndroidApp::GetInstance();
		CIRect rtWnd = pWnd->m_pContext->m_rtWnd;
		bool bPreActive = pWnd->m_pContext->m_bActive && pWnd->m_pContext->m_bFocus;
		bool bCurActive;

		switch( nMsg )
		{
		case WM_ACTIVATE:
		case WM_KILLFOCUS:
		case WM_SETFOCUS:
			if( nMsg == WM_ACTIVATE )
				pWnd->m_pContext->m_bActive = wParam != 0;
			else if( nMsg == WM_KILLFOCUS )
				pWnd->m_pContext->m_bFocus = false;
			else
				pWnd->m_pContext->m_bFocus = true;
			bCurActive = pWnd->m_pContext->m_bActive && pWnd->m_pContext->m_bFocus;
			if( bCurActive == bPreActive )
				return 0;
			return pWnd->OnProcessMsg( nInputID, bCurActive ? WM_SETFOCUS : WM_KILLFOCUS, 0, 0 );
		case WM_CREATE:
			PROCESS_LOG;
			pWnd->m_pContext->m_pHandler = App.GetNativeWindow();
			pWnd->m_pContext->m_rtWnd.left = 0;
			pWnd->m_pContext->m_rtWnd.top = 0;
			pWnd->m_pContext->m_rtWnd.right = ANativeWindow_getWidth( (ANativeWindow*)pWnd->m_pContext->m_pHandler );
			pWnd->m_pContext->m_rtWnd.bottom = ANativeWindow_getHeight( (ANativeWindow*)pWnd->m_pContext->m_pHandler );
			PROCESS_LOG;
			if( !pWnd->m_pContext->m_bCreated )
				pWnd->OnCreated();
			else
				pWnd->OnProcessMsg( nInputID, WM_DISPLAYCHANGE, 1, lParam );

			if( rtWnd != pWnd->m_pContext->m_rtWnd )
			{
				lParam = MAKE_UINT32( pWnd->m_pContext->m_rtWnd.right, pWnd->m_pContext->m_rtWnd.bottom );
				pWnd->OnProcessMsg( nInputID, WM_SIZE, SIZE_RESTORED, lParam );
			}

			PROCESS_LOG;
			pWnd->m_pContext->m_bCreated = true;
			return 1;
		case WM_DESTROY:
			pWnd->m_pContext->m_pHandler = NULL;
			pWnd->OnProcessMsg( nInputID, WM_DISPLAYCHANGE, 0, lParam );
			return 1;
		case WM_ACTIVATEAPP:
			pWnd->m_pContext->m_pContext = wParam ? App.GetActivity() : 0;
			pWnd->OnProcessMsg( nInputID, WM_ACTIVATEAPP, wParam, lParam );
			return 1;
		case WM_SIZE:
			pWnd->m_pContext->m_rtWnd.left = 0;
			pWnd->m_pContext->m_rtWnd.top = 0;
			pWnd->m_pContext->m_rtWnd.right = ANativeWindow_getWidth( (ANativeWindow*)pWnd->m_pContext->m_pHandler );
			pWnd->m_pContext->m_rtWnd.bottom = ANativeWindow_getHeight( (ANativeWindow*)pWnd->m_pContext->m_pHandler );
			lParam = MAKE_UINT32( pWnd->m_pContext->m_rtWnd.right, pWnd->m_pContext->m_rtWnd.bottom );
			break;
		case WM_LOW_MEMORY:
			GammaLog << "Low memory warning!!!!" << endl;
			pWnd->OnLowMemory();
			break;
		}

		return pWnd->OnProcessMsg( nInputID, nMsg, wParam, lParam );
	}
#elif ( defined _IOS )
    void CGammaWindow::Initialize( void* pContext, void* pParent, uint32_t nWidth, uint32_t nHeight, const char* szTitle, uint32_t nIconID )
    {
        g_mapWindowList.PushFront( *m_pContext );
        m_pContext->m_pContext = pContext;
        m_pContext->m_pHandler = CIOSApp::GetInstance().CreateIOSGLView( (InputHandler)&SWindowContext::ProcessInput, this );
    }
    
    int32_t SWindowContext::ProcessInput( CGammaWindow* pWnd, uint32_t nInputID, uint32_t nMsg, uint32_t wParam, uint32_t lParam )
    {
        STATCK_LOG( pWnd );
		CIRect rtWnd = pWnd->m_pContext->m_rtWnd;
		
        switch( nMsg )
		{
		case WM_SHOWWINDOW:
			pWnd->m_pContext->m_bHide = !wParam;
			return pWnd->OnProcessMsg( nInputID, WM_DISPLAYCHANGE, wParam, 0 );
			break;
		case WM_SETFOCUS:
			if( pWnd->m_pContext->m_bHide )
				pWnd->m_pContext->m_bHide = false;
			if( pWnd->m_pContext->m_bFocus )
				return 0;
			pWnd->m_pContext->m_bFocus = true;
			return pWnd->OnProcessMsg( nInputID, WM_SETFOCUS, 0, 0 );
		case WM_KILLFOCUS:
			if( !pWnd->m_pContext->m_bFocus )
				return 0;
			pWnd->m_pContext->m_bFocus = false;
			return pWnd->OnProcessMsg( nInputID, WM_KILLFOCUS, 0, 0 );
		case WM_CREATE:
			PROCESS_LOG;
			pWnd->m_pContext->m_rtWnd.left = 0;
			pWnd->m_pContext->m_rtWnd.top = 0;
			pWnd->m_pContext->m_rtWnd.right = wParam;
			pWnd->m_pContext->m_rtWnd.bottom = lParam;
			PROCESS_LOG;
			if( !pWnd->m_pContext->m_bCreated )
				pWnd->OnCreated();
			else
				pWnd->OnProcessMsg( nInputID, WM_DISPLAYCHANGE, 1, lParam );
			
			if( rtWnd != pWnd->m_pContext->m_rtWnd )
			{
				lParam = MAKE_UINT32( pWnd->m_pContext->m_rtWnd.right, pWnd->m_pContext->m_rtWnd.bottom );
				pWnd->OnProcessMsg( nInputID, WM_SIZE, SIZE_RESTORED, lParam );
			}
			
			PROCESS_LOG;
			pWnd->m_pContext->m_bCreated = true;
			return 1;
		case WM_DESTROY:
			pWnd->m_pContext->m_pHandler = NULL;
			pWnd->OnProcessMsg( nInputID, WM_DISPLAYCHANGE, 0, lParam );
			return 1;
		case WM_ACTIVATEAPP:
			return 1;
		case WM_SIZE:
			pWnd->m_pContext->m_rtWnd.left = 0;
			pWnd->m_pContext->m_rtWnd.top = 0;
			pWnd->m_pContext->m_rtWnd.right = LOUINT16( lParam );
			pWnd->m_pContext->m_rtWnd.bottom = HIUINT16( lParam );
			break;
		case WM_LOW_MEMORY:
				GammaLog << "Low memory warning!!!!" << std::endl;
			pWnd->OnLowMemory();
			break;
		}
        
        return pWnd->OnProcessMsg( nInputID, nMsg, wParam, lParam );
    }
#else
	void CGammaWindow::Initialize( void* pContext, void* pParent, uint32_t nWidth, uint32_t nHeight, const char* szTitle, uint32_t nIconID )
	{
		OnCreated();
	}
#endif

	void CGammaWindow::Initialize( void* pContext, void* pHandler, bool bHookMsg )
	{
		m_pContext->m_pContext = pContext;
		m_pContext->m_pHandler = pHandler;
#ifdef _WIN32
		if( bHookMsg )
		{
			m_pContext->m_pOrgWndProc = (WNDPROC)(ptrdiff_t)::GetWindowLong( (HWND)pContext, GWLP_WNDPROC );
			::SetWindowLongPtr( (HWND)pContext, GWLP_WNDPROC, (LONG_PTR)(WNDPROC)&SWindowContext::WindowProc );
			g_mapWindowList.PushFront( *m_pContext );
			OnCreated();
		}
#elif defined _ANDROID
		g_mapWindowList.PushFront( *m_pContext );
		CAndroidApp::GetInstance().RegisterMsgHandler( this, (InputHandler)&SWindowContext::ProcessInput );
#endif
	}

	void CGammaWindow::AddMsgCallback( WndMsgCallback funCallback, void* pContext )
	{
		if( !m_pContext )
			return;

		for( size_t i = 0; i < m_pContext->m_vecCallbackInfo.size(); i++ )
		{
			if( m_pContext->m_vecCallbackInfo[i].m_funCallback == funCallback &&
				m_pContext->m_vecCallbackInfo[i].m_pContext == pContext  )
				return;
		}

		SCallback Callback = { funCallback, pContext };
		m_pContext->m_vecCallbackInfo.push_back( Callback );
	}

	void CGammaWindow::RemoveMsgCallback( WndMsgCallback funCallback, void* pContext )
	{
		if( !m_pContext )
			return;

		size_t i = 0;
		while( i < m_pContext->m_vecCallbackInfo.size() )
		{
			if( m_pContext->m_vecCallbackInfo[i].m_funCallback == funCallback &&
				m_pContext->m_vecCallbackInfo[i].m_pContext == pContext  )
				m_pContext->m_vecCallbackInfo.erase( m_pContext->m_vecCallbackInfo.begin() + i );
			else
				i++;
		}
	}

	void CGammaWindow::Destroy()
	{
		m_pContext->Remove();
#ifdef _WIN32
		::DestroyWindow( (HWND)GetHandle() );
#endif
		m_pContext->m_pHandler = NULL;
	}

	void CGammaWindow::Show( bool bShow )
	{
#ifdef _WIN32
		::ShowWindow( (HWND)GetHandle(), bShow ? SW_SHOW : SW_HIDE );
		HWND hForegdWnd	= ::GetForegroundWindow(); 
		DWORD dwCurID	= ::GetCurrentThreadId(); 
		DWORD dwForeID	= ::GetWindowThreadProcessId( hForegdWnd, NULL ); 
		::AttachThreadInput( dwCurID, dwForeID, TRUE ); 
		::SetForegroundWindow( (HWND)GetHandle() ); 
		::AttachThreadInput( dwCurID, dwForeID, FALSE );

		if( !bShow )
			return;
		::SetFocus( (HWND)GetHandle() );
#endif
	}

	void CGammaWindow::GetClientRect( CIRect& rt )
	{
#ifdef _WIN32
		::GetClientRect( (HWND)GetHandle(), (RECT*)&rt );
#elif ( defined _ANDROID || defined _IOS )
		rt = m_pContext->m_rtWnd;
#endif
	}

	void CGammaWindow::GetWindowRect( CIRect& rt )
	{
#ifdef _WIN32
		::GetWindowRect( (HWND)GetHandle(), (RECT*)&rt );
#elif ( defined _ANDROID || defined _IOS )
		rt = m_pContext->m_rtWnd;
#endif
	}

	uint32_t CGammaWindow::GetClientWidth()
	{
		CIRect rtClient;
		GetClientRect( rtClient );
		return rtClient.Width();
	}

	uint32_t CGammaWindow::GetClientHeight()
	{
		CIRect rtClient;
		GetClientRect( rtClient );
		return rtClient.Height();
	}

	bool CGammaWindow::IsFocus() const
	{
#ifdef _WIN32
		return ::GetFocus() == GetHandle();
#elif ( defined _ANDROID || defined _IOS )
		return GetHandle() && !IsHide() && m_pContext->IsFocus();
#else
		return true;
#endif
	}

	void CGammaWindow::SetTitle( const char* szTitle )
	{
#ifdef _UNICODE
		::SetWindowTextW( (HWND)GetHandle(), Utf8ToUcs( szTitle ).c_str() );
#else
#ifdef _WIN32
		::SetWindowTextA( (HWND)GetHandle(), 
			UcsToAnsi( Utf8ToUcs( szTitle ).c_str() ).c_str() );
#endif
#endif
	}

	bool CGammaWindow::IsHide() const
	{
#ifdef _WIN32
		return IsWindowVisible( (HWND)GetHandle() ) != TRUE;
#elif ( defined _ANDROID || defined _IOS )
		return GetHandle() == NULL || m_pContext->m_bHide;
#else
		return false;
#endif
	}

	bool CGammaWindow::IsValid() const
	{
#if ( defined _WIN32 || defined _ANDROID || defined _IOS )
		return GetHandle() != NULL;
#else
		return false;
#endif
	}

	void CGammaWindow::SetCursor( const char* szCursorName )
	{
		m_pContext->m_pCurCursorFile = szCursorName ?
			CAniCursorFile::GetCursor( szCursorName ) : NULL;
		UpdateCursor();
	}

	CPos CGammaWindow::GetCursorPos()
	{
#ifdef _WIN32
		CPos vCursorPos;
		::GetCursorPos( (POINT*)&vCursorPos );
		::ScreenToClient( (HWND)m_pContext->m_pHandler, (POINT*)&vCursorPos );
		return vCursorPos;
#else
		return m_pContext->m_posCursor;
#endif
	}

	void CGammaWindow::SetCursorPos( CPos vPos )
	{
#ifdef _WIN32
		::ClientToScreen( (HWND)m_pContext->m_pHandler, (POINT*)&vPos );
		::SetCursorPos( vPos.x, vPos.y );
#endif
	}

	void CGammaWindow::ClipCursor( const CIRect* rtRange, bool bLockCenter )
	{
#ifdef _WIN32
		#define INVALID_CURSOR	((CAniCursorFile*)1)
		if( !rtRange )
		{
			m_pContext->m_bLockCursor = false;
			m_pContext->m_rtLock = INVALID_LOCK_RECT;
			::ClipCursor( NULL );
			UpdateCursor();
			return;
		}

		CIRect rtWndClient;
		GetClientRect( rtWndClient );
		int32_t nWidth = rtWndClient.Width();
		int32_t nHeight = rtWndClient.Height();
		m_pContext->m_rtLock.left = Limit<int32_t>( rtRange->left, 0, nWidth );
		m_pContext->m_rtLock.right = Limit<int32_t>( rtRange->right, 0, nWidth );
		m_pContext->m_rtLock.top = Limit<int32_t>( rtRange->top, 0, nHeight );
		m_pContext->m_rtLock.bottom = Limit<int32_t>( rtRange->bottom, 0, nHeight );

		RECT rtScreen;
		rtScreen.left = rtRange->left;
		rtScreen.right = rtRange->right;
		rtScreen.top = rtRange->top;
		rtScreen.bottom = rtRange->bottom;
		::ClientToScreen( (HWND)m_pContext->m_pHandler, (POINT*)&rtScreen.left );
		::ClientToScreen( (HWND)m_pContext->m_pHandler, (POINT*)&rtScreen.right );
		::ClipCursor( (const RECT*)&rtScreen );
		if( !bLockCenter )
			return;
		m_pContext->m_bLockCursor = true;
		UpdateCursor();
#endif
	}

	void CGammaWindow::UpdateCursor()
	{
#ifdef _WIN32
		bool bRegisterTimer = !m_pContext->m_bLockCursor &&
			m_pContext->m_pCurCursorFile &&
			m_pContext->m_pCurCursorFile->GetFrameCount() > 1;

		if( m_pContext->m_bTimerRegister != bRegisterTimer )
		{
			if( bRegisterTimer )
				SetTimer( (HWND)GetHandle(), (ptrdiff_t)this, 50, &SWindowContext::TimerProc );
			else
				KillTimer( (HWND)GetHandle(), (ptrdiff_t)this );
			m_pContext->m_bTimerRegister = bRegisterTimer;
		}

		if( m_pContext->m_bLockCursor )
		{
			::SetCursor( NULL );
			return;
		}
#endif
		if( !m_pContext->m_pCurCursorFile )
		{
#ifdef _WIN32
			::DefWindowProc( (HWND)m_pContext->m_pHandler, WM_SETCURSOR, 0, 0 );
#endif
			return;
		}

		m_pContext->m_pCurCursorFile->Update();
	}

	void* CGammaWindow::GetContext() const
	{
		return m_pContext->m_pContext;
	}

	void* CGammaWindow::GetHandle() const
    {
#if ( defined _IOS )
        return CIOSApp::GetNativeHandler( (SIOSWnd*)( m_pContext->m_pHandler ) );
#else
        return m_pContext->m_pHandler;
#endif
	}

	bool CGammaWindow::OnProcessMsg( uint32_t nInputID, uint32_t nMsg, uint32_t wParam, uint32_t lParam )
	{
		bool bBreak = false;
		for( size_t i = 0; i < m_pContext->m_vecCallbackInfo.size(); i++ )
		{
			void* pContext = m_pContext->m_vecCallbackInfo[i].m_pContext;
			WndMsgCallback funCallback = m_pContext->m_vecCallbackInfo[i].m_funCallback;
			bBreak = bBreak ||funCallback( pContext, this, nInputID, nMsg, wParam, lParam );
		}

		if( bBreak )
			return true;

		if( nMsg >= WM_MOUSEFIRST && nMsg <= WM_MOUSELAST )
			UpdateCursor();

		switch( nMsg )
		{
		case WM_SETCURSOR:
			UpdateCursor();
			return true;
		case WM_SIZE:
			switch( wParam )
			{
			case SIZE_MINIMIZED:
				OnMinimized();
				break;
			case SIZE_MAXIMIZED:
				OnMaximized();
				break;
			case SIZE_RESTORED:
				OnRestore();
				break;
			}
			break;
		case WM_SETFOCUS:
#ifdef _WIN32
			SWindowContext::CheckClipCursor( this, true );
#endif
			OnSetFocus( true );
			break;
		case WM_KILLFOCUS:
#ifdef _WIN32
			SWindowContext::CheckClipCursor( this, false );
#endif
			OnSetFocus( false );
			break;
		case WM_CLOSE:
			if( !OnClose() )
				return true;
			Destroy();
			break;
		case WM_DESTROY:
			Destroy();
			break;
		case WM_ACTIVATE:
			OnActive( LOWORD(wParam) != WA_INACTIVE );
			break;
		case WM_SHOWWINDOW:
			OnShow( wParam > 0 );
			break;
		}
		return false;
	}

	int16_t CGammaWindow::GetKeyState( uint8_t nKeyCode )
	{
#ifdef _WIN32
		return (int16_t)::GetKeyState( nKeyCode ); 
#else
		return 0;
#endif
	}

	void CGammaWindow::EnableInput( bool bEnable, bool bFullScreen, const wchar_t* szInitText )
	{
#ifdef _ANDROID
        CAndroidApp::GetInstance().EnableInput( bEnable, bFullScreen ? szInitText : NULL );
#elif ( defined _IOS )
		SIOSWnd* pWnd = (SIOSWnd*)( m_pContext->m_pHandler );
        CIOSApp::GetInstance().EnableInput( pWnd, bEnable, bFullScreen, szInitText );
#endif
	}
}
