
#include "GammaCommon/GammaSignal.h"
#include "GammaCommon/CThread.h"

namespace Gamma
{
	//=====================================================================
	// struct SSignalContext
	//=====================================================================
	struct SSignalContext
	{
		SignalHandler					m_arySignal[INVALID_8BITID];

#ifdef _WIN32
		LPTOP_LEVEL_EXCEPTION_FILTER	m_pPreFilter;
		static void						OnSignal( int32_t nSignal );
		void							EnableUnhandledException( bool bEnable );
		static long WINAPI				ExceptionFilter( _EXCEPTION_POINTERS* pException );
#else
		static void						OnSignal( int nSignum, siginfo_t* pInfo, void* pContext );
#endif

		SSignalContext()
		{
			memset( m_arySignal, 0, sizeof(m_arySignal) );
#ifdef _WIN32
			m_pPreFilter = NULL;
#endif
		}

		SignalHandler GetSignalHandler( int32_t nSignal )
		{
			GammaAst( nSignal < ELEM_COUNT( m_arySignal ) );
			if( nSignal >= ELEM_COUNT( m_arySignal ) )
				return NULL;
			return m_arySignal[nSignal];
		}

		void SetSignalHandler( int32_t nSignal, SignalHandler funHandler )
		{
			GammaAst( nSignal < ELEM_COUNT( m_arySignal ) );
			if( nSignal >= ELEM_COUNT( m_arySignal ) )
				return;
			m_arySignal[nSignal] = funHandler;
		}
	};

	SSignalContext g_SignalContext;

#ifdef _WIN32
	void SSignalContext::OnSignal( int32_t nSignal )
	{
		SignalHandler funHandler = g_SignalContext.GetSignalHandler( nSignal );
		if( !funHandler )
			return IgnoreSignal( nSignal );
		funHandler( nSignal, NULL );
	}

	void SSignalContext::EnableUnhandledException( bool bEnable )
	{
		if( ( !!m_pPreFilter ) == bEnable )
			return;

		if( bEnable )
			m_pPreFilter = SetUnhandledExceptionFilter( &ExceptionFilter );
		else
			SetUnhandledExceptionFilter( g_SignalContext.m_pPreFilter );
		m_pPreFilter = bEnable ? m_pPreFilter : NULL;
	}

	long WINAPI SSignalContext::ExceptionFilter( _EXCEPTION_POINTERS* pException )
	{
		SetUnhandledExceptionFilter( g_SignalContext.m_pPreFilter );
		g_SignalContext.m_pPreFilter = NULL;
		SignalHandler funHandler = g_SignalContext.GetSignalHandler( SIGSEGV );
		if( funHandler )
			funHandler( SIGSEGV, pException );
#ifdef _DEBUG
		return EXCEPTION_CONTINUE_SEARCH;
#else
		return EXCEPTION_EXECUTE_HANDLER;
#endif
	}
#else
	void SSignalContext::OnSignal( int nSignal, siginfo_t* pInfo, void* pContext )
	{
		SignalHandler funHandler = g_SignalContext.GetSignalHandler( nSignal );
		if( !funHandler )
			return;
		funHandler( nSignal, pContext );
	}
#endif

	//=====================================================================
	// Signal API
	//=====================================================================
	GAMMA_COMMON_API void InstallSignalHandler( int32_t nSignal, SignalHandler funSigHandler )
	{
		g_SignalContext.SetSignalHandler( nSignal, funSigHandler );
#ifdef _WIN32
		if( funSigHandler )
		{
			if( nSignal == SIGSEGV )
				g_SignalContext.EnableUnhandledException( true );
			else
				signal( nSignal, &SSignalContext::OnSignal );
		}
		else
		{
			if( nSignal == SIGSEGV )
				g_SignalContext.EnableUnhandledException( false );
			else
				signal( nSignal, NULL );
		}
#else
		struct sigaction sa;
		memset( &sa, 0, sizeof(sa) );
		if( funSigHandler )
		{
			sa.sa_sigaction = &SSignalContext::OnSignal;
			sa.sa_flags = SA_RESETHAND|SA_SIGINFO;
		}
		if( 0 == sigaction( nSignal, &sa, NULL ) )
			return;
		GammaThrow( L"system call sigaction failed." );
#endif
	}

	GAMMA_COMMON_API void IgnoreSignal( int32_t nSignal )
	{
#ifdef _WIN32
		g_SignalContext.SetSignalHandler( nSignal, NULL );
		signal( nSignal, &SSignalContext::OnSignal );
#else
		struct sigaction sa;
		memset( &sa, 0, sizeof(sa) );
		sa.sa_handler = SIG_IGN;
		sa.sa_flags = 0;
		if( 0 == sigaction( nSignal, &sa, NULL ) )
			return;
		GammaThrow( L"system call sigaction failed." );
#endif
	}

	GAMMA_COMMON_API void RaiseSignal( int32_t nSignal )
	{
#ifdef _WIN32
		if( nSignal == 9 )
			ExitProcess( 0 );
#endif
		raise( nSignal );
	}

}
