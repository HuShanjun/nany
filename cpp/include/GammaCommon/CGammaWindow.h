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
			uint32_t /*nInputID*/, uint32_t /*nMsg*/, uint32_t /*wParam*/, uint32_t /*lParam*/ );
	 
	class GAMMA_COMMON_API CGammaWindow
	{
	public:
		CGammaWindow();
		virtual ~CGammaWindow();

		void* GetContext() const;
		void* GetHandle() const;
		void Initialize( void* pContext, void* pParent, uint32_t nWidth, uint32_t nHeight, const char* szTitle, uint32_t nIconID);
		void Initialize( void* pContext, void* pHandler, bool bHookMsg );
		void AddMsgCallback( WndMsgCallback funCallback, void* pContext );
		void RemoveMsgCallback( WndMsgCallback funCallback, void* pContext );

		virtual void Destroy();
		virtual void Show( bool bShow );
		virtual void GetClientRect( CIRect& rt );
		virtual void GetWindowRect( CIRect& rt );
		virtual uint32_t GetClientWidth();
		virtual uint32_t GetClientHeight();
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

		static int32_t MessagePump();
		static int16_t GetKeyState( uint8_t nKeyCode );
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
		virtual bool OnProcessMsg( uint32_t nInputID, uint32_t nMsg, uint32_t wParam, uint32_t lParam );

	private:
		friend struct		SWindowContext;
		SWindowContext*		m_pContext;
	};
}
