#pragma once
#include "GammaCommon/GammaCommonType.h"
#include "GammaCommon/GammaMath.h"
#include "GammaCommon/GammaTime.h"
#include "GammaCommon/TVector2.h"
#include "GammaCommon/TRect.h"

#ifdef _WIN32
#include <Windows.h>
#endif

namespace Gamma
{
	class CGammaWindow;
	struct SWindowContext;

	typedef bool (*WndMsgCallback)( void* /*pContext*/, CGammaWindow* /*pWnd*/,
			uint32 /*nInputID*/, uint32 /*nMsg*/, uint32 /*wParam*/, uint32 /*lParam*/ );
	 
	class GAMMA_COMMON_API CGammaWindow
	{
	public:
		CGammaWindow();
		virtual ~CGammaWindow();

		void* GetContext() const;
		void* GetHandle() const;
		void Initialize( void* pContext, void* pParent, uint32 nWidth, uint32 nHeight, const char* szTitle, uint32 nIconID);
		void Initialize( void* pContext, void* pHandler, bool bHookMsg );
		void AddMsgCallback( WndMsgCallback funCallback, void* pContext );
		void RemoveMsgCallback( WndMsgCallback funCallback, void* pContext );

		virtual void Destroy();
		virtual void Show( bool bShow );
		virtual void GetClientRect( CIRect& rt );
		virtual void GetWindowRect( CIRect& rt );
		virtual uint32 GetClientWidth();
		virtual uint32 GetClientHeight();
		virtual void SetTitle( const char* szTitle );
		virtual void EnableInput( bool bEnable, bool bFullScreen, const wchar_t* szInitText );

		bool IsFocus() const;
		bool IsHide() const;
		bool IsValid() const;

		void SetCursor( const char* szCursorName );
		void UpdateCursor();
		CPos GetCursorPos();
		void SetCursorPos( CPos vPos );
		void ClipCursor( const CIRect* rtRange, bool bLockCenter );

		static int32 MessagePump();
		static int16 GetKeyState( uint8 nKeyCode );
	protected:
		virtual void OnCreated(){}
		virtual bool OnClose(){ return true; };
		virtual void OnSetFocus( bool bFocus ){};
		virtual void OnActive( bool bActive ){};
		virtual void OnShow( bool bShow ){};
		virtual void OnMinimized(){};
		virtual void OnMaximized(){};
		virtual void OnRestore(){};
		virtual void OnLowMemory(){}
		virtual bool OnProcessMsg( uint32 nInputID, uint32 nMsg, uint32 wParam, uint32 lParam );

	private:
		friend struct		SWindowContext;
		SWindowContext*		m_pContext;
	};
}
