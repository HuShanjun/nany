
#include <stdlib.h>
#include "CGammaDebug.h"
#include "GammaCommon/CThread.h"
#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/CPathMgr.h"
#include "GammaCommon/TGammaStrStream.h"
#include "CMgrInstance.h"

#pragma warning(disable:4740)

#ifdef _ANDROID
#include "Android.h"
#endif

namespace Gamma
{
	CGammaDebug& CGammaDebug::Instance()
	{
		return CMgrInstance::Instance().GetDebugMgrInstance();
	}

	CGammaDebug::CGammaDebug()
	{ 
#ifdef USE_DEBUGHELP
		m_nMoudleCount		= 0;
		m_nMoudleMaxCount	= 0;
		m_pModuleBase		= NULL;
		InitLock( m_Lock );
		SymInitialize( GetCurrentProcess(), NULL, FALSE );
#endif

#ifdef _ANDROID
		void* pHandle = dlopen( "libcorkscrew.so", RTLD_NOW );
		if( pHandle )
		{
			m_unwind_backtrace = (unwindFn)dlsym( pHandle, "unwind_backtrace" );  
			m_get_backtrace_symbols = (unwindSymbFn)dlsym( pHandle, "get_backtrace_symbols" );  
			m_free_backtrace_symbols = (unwindSymbFreeFn)dlsym( pHandle, "free_backtrace_symbols" );  
		}
		else
		{
			m_unwind_backtrace = NULL;  
			m_get_backtrace_symbols = NULL;  
			m_free_backtrace_symbols = NULL;  
		}
#endif
	}

	CGammaDebug::~CGammaDebug()
	{ 
#ifdef USE_DEBUGHELP
		SymCleanup( GetCurrentProcess() );
		DestroyLock( m_Lock );
#endif
	}

#ifdef USE_DEBUGHELP
	bool CGammaDebug::AddModule( uint64_t nModuleBase )
	{
		for( size_t i = 0; i < m_nMoudleCount; i++ )
			if( m_pModuleBase[i] == nModuleBase )
				return false;
		if( m_nMoudleCount == m_nMoudleMaxCount )
		{
			m_nMoudleMaxCount += 32;
			uint64_t* pNewBuf = (uint64_t*)malloc( m_nMoudleMaxCount*sizeof(uint64_t) );
			memcpy( pNewBuf, m_pModuleBase, m_nMoudleCount*sizeof(uint64_t) );
			if( m_pModuleBase )
				free( m_pModuleBase );
			m_pModuleBase = pNewBuf;
		}

		if( m_pModuleBase )
			m_pModuleBase[m_nMoudleCount++] = nModuleBase;
		return true;
	}

	void CGammaDebug::CheckAndLoadSymbol()
	{
		EnterLock( m_Lock );
		BOOL bRe = EnumerateLoadedModules64( GetCurrentProcess(), (PENUMLOADED_MODULES_CALLBACK64)&EnumModuleCallback, this );
		LeaveLock( m_Lock );
		if( !bRe )
		{
			char szError[64];
			sprintf( szError, "EnumerateLoadedModules64 failed with error code:%d", GetLastError() );
			GammaThrow( szError );
		}
	}

	BOOL CALLBACK CGammaDebug::EnumModuleCallback( char* sModuleName, uint64_t nModuleBase, uint32_t, void* pArg )
	{
		if( static_cast<CGammaDebug*>( pArg )->AddModule( nModuleBase ) )
			SymLoadModule64( GetCurrentProcess(), NULL, sModuleName, sModuleName, nModuleBase, NULL );

		return TRUE;
	}
#endif

	void CGammaDebug::DebugAddress2Symbol( void* pAddress, char* szSymbolBuf, uint32_t nSize )
	{
#ifdef _WIN32
#ifdef USE_DEBUGHELP
		char szBuffer[sizeof(SYMBOL_INFO)+1024];
		SYMBOL_INFO* pInfo		= reinterpret_cast<SYMBOL_INFO*>( szBuffer );
		pInfo->SizeOfStruct		= sizeof(SYMBOL_INFO);
		pInfo->MaxNameLen		= sizeof(szBuffer) - sizeof(SYMBOL_INFO);
		DWORD64 uDisplacement	= 0;

		if( !SymFromAddr( GetCurrentProcess(), (DWORD64)pAddress, &uDisplacement, pInfo ) )
		{
			CGammaDebug::Instance().CheckAndLoadSymbol();
			if( !SymFromAddr( GetCurrentProcess(), (DWORD64)pAddress, &uDisplacement, pInfo ) )
				return;
		}

		strcpy_safe( szSymbolBuf, pInfo->Name, nSize, INVALID_32BITID );
#endif
#else
#ifndef _ANDROID
		char** arySymbol;
		arySymbol = backtrace_symbols( &pAddress, 1 );
		strcpy_safe( szSymbolBuf, *arySymbol, nSize, INVALID_32BITID );
		free( arySymbol );
#else
		if( m_get_backtrace_symbols == NULL || m_free_backtrace_symbols == NULL )
			return;
		backtrace_symbol_t symbols;  
		backtrace_frame_t frame = { (uintptr_t)pAddress, 0, 0 };
		m_get_backtrace_symbols( &frame, 1, &symbols );
		const char* symbolName = symbols.demangled_name ? symbols.demangled_name : symbols.symbol_name;  
		strcpy_safe( szSymbolBuf, symbolName, nSize, INVALID_32BITID );
		m_free_backtrace_symbols( &symbols, 1 ); 
#endif
#endif
	}

#ifdef _WIN32
	void CGammaDebug::DebugGenMiniDump( EXCEPTION_POINTERS* pException, const wchar_t* szFileName, bool bFullMemDump )
	{
		HANDLE hReportFile = CreateFile( (const char*)szFileName, GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_FLAG_WRITE_THROUGH, 0 );
		if( !hReportFile )
			return;

#ifdef USE_DEBUGHELP
		_MINIDUMP_EXCEPTION_INFORMATION ExInfo;

		ExInfo.ThreadId = ::GetCurrentThreadId();
		ExInfo.ExceptionPointers = pException;
		ExInfo.ClientPointers = NULL;

		BOOL bRet = MiniDumpWriteDump( GetCurrentProcess(), GetCurrentProcessId(), hReportFile, bFullMemDump ? MiniDumpWithFullMemory : MiniDumpNormal , &ExInfo, NULL, NULL );
		if( !bRet )
		{  
			LPVOID lpMsgBuf;
			DWORD dw = GetLastError(); 

			FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL );
			
			wchar_t szBuf[80];
			wgammasstream( szBuf, ELEM_COUNT( szBuf ) ) << L"failed with error " << dw << L": " << lpMsgBuf;
			MessageBoxW(NULL, szBuf, L"Error", MB_OK); 
			LocalFree(lpMsgBuf);
		}
#else
		DWORD nWriten;
		#define WRITE_RECORD( n )   \
		sprintf( szBuf, #n" = %d(%08x)\r\n", pException->ExceptionRecord->n, pException->ExceptionRecord->n );\
		WriteFile( hReportFile, szBuf, strlen( szBuf ), &nWriten, NULL );

		#define WRITE_CONTEXT( n )   \
		sprintf( szBuf, #n" = %d(%08x)\r\n", pException->ContextRecord->n, pException->ContextRecord->n );\
		WriteFile( hReportFile, szBuf, strlen( szBuf ), &nWriten, NULL );

		char szBuf[256];
		sprintf( szBuf, "%s\r\n", "EXCEPTION_RECORD" );
		WriteFile( hReportFile, szBuf, strlen( szBuf ), &nWriten, NULL );
		if( pException->ExceptionRecord )
		{
			WRITE_RECORD( ExceptionCode );
			WRITE_RECORD( ExceptionFlags );
			WRITE_RECORD( ExceptionAddress );
			WRITE_RECORD( NumberParameters );
		}

		sprintf( szBuf, "%s\r\n", "EXCEPTION_CONTEXT" );
		WriteFile( hReportFile, szBuf, strlen( szBuf ), &nWriten, NULL );
		if( pException->ContextRecord )
		{
			WRITE_CONTEXT( ContextFlags );

			WRITE_CONTEXT( Dr0 );
			WRITE_CONTEXT( Dr1 );
			WRITE_CONTEXT( Dr2 );
			WRITE_CONTEXT( Dr3 );
			WRITE_CONTEXT( Dr6 );
			WRITE_CONTEXT( Dr7 );

			WRITE_CONTEXT( SegGs );
			WRITE_CONTEXT( SegFs );
			WRITE_CONTEXT( SegEs );
			WRITE_CONTEXT( SegDs );

			WRITE_CONTEXT( Edi );
			WRITE_CONTEXT( Esi );
			WRITE_CONTEXT( Ebx );
			WRITE_CONTEXT( Edx );
			WRITE_CONTEXT( Ecx );
			WRITE_CONTEXT( Eax );

			WRITE_CONTEXT( Ebp );
			WRITE_CONTEXT( Eip );
			WRITE_CONTEXT( SegCs );
			WRITE_CONTEXT( EFlags );
			WRITE_CONTEXT( Esp );
			WRITE_CONTEXT( SegSs );
		}

		const char* szAddress = (const char*)pException->ExceptionRecord->ExceptionAddress;
		try
		{
			for( int32_t i = -100; i < 100; i++ )
			{
				uint32_t nCurAddr = (uint32_t)( szAddress + i );
				if( nCurAddr > (uint32_t)( szAddress + 100 ) )
					continue;
				sprintf( szBuf, "%08x:\t%02x\t(%d)\r\n", nCurAddr, (uint8_t)szAddress[i], (uint8_t)szAddress[i] );
				WriteFile( hReportFile, szBuf, strlen( szBuf ), &nWriten, NULL );
			}
		}
		catch(...)
		{			
		}
#endif
		CloseHandle( hReportFile );
	}
#endif

	size_t CGammaDebug::GetStack( void** pStack, uint16_t nBegin, uint16_t nEnd, void* pMinStack )
	{
#ifdef _WIN32
#ifdef USE_DEBUGHELP
		typedef USHORT (WINAPI *FunctionType)( __in ULONG, __in ULONG, __out PVOID*, __out_opt PULONG );
		static FunctionType funBackTrace = (FunctionType)( 
			GetProcAddress( LoadLibraryW( L"kernel32.dll" ), "RtlCaptureStackBackTrace" ) );
		// Then use 'func' as if it were CaptureStackBackTrace
		return funBackTrace ? funBackTrace( nBegin, nEnd - nBegin, pStack, NULL ) : 0;
#endif
#else
	#ifndef _ANDROID
		nBegin++;
		nEnd++;

		void* pTempStack[2048];
		::backtrace( pTempStack, 2048 );
		for( size_t i = nBegin; i < 2048 && i < nEnd; i++ )
		{
			if( pTempStack[i] == 0 )
				return i - nBegin;
			pStack[ i - nBegin ] = pTempStack[i];
		}
		return Min<size_t>( 2048, nEnd ) - nBegin;
	#else
		if( m_unwind_backtrace == NULL )
			return 0;

		void** pStackStart = (void**)( pMinStack ? pMinStack : &pStack );
		void** pMaxStack = CAndroidApp::GetInstance().GetMaxMainStack();
		uint32_t nMaxCount = CAndroidApp::GetInstance().GetMainStackSize()/(sizeof(void*));
		if( pStackStart > pMaxStack || pMaxStack - pStackStart > nMaxCount )
		{
			nMaxCount = 1024;
			pMaxStack = pStackStart + nMaxCount;
		}

		uint32_t nCount = 0;
		for( int i = 0; pStackStart < pMaxStack && nCount < nEnd; i++, pStackStart++ )
		{
			backtrace_symbol_t symbols;  
			backtrace_frame_t frame = { (uintptr_t)*pStackStart, 0, 0 };
			m_get_backtrace_symbols( &frame, 1, &symbols );
			const char* symbolName = symbols.demangled_name ? symbols.demangled_name : symbols.symbol_name;  
			if( symbolName == NULL || symbolName[0] == 0 || symbolName[0] == '_' || 
				( symbolName[0] >= 'a' && symbolName[0] <= 'z' ) || nCount++ < nBegin )
				continue;
			pStack[ nCount - nBegin - 1] = *pStackStart;
		}
		return nCount > nBegin ? nCount - nBegin : 0;

		//backtrace_frame_t StackFrame[1024];  
		//uint32_t nCount = m_unwind_backtrace( StackFrame, nBegin, nEnd );  
		//for( int i = 0; i < nCount; i++ )
		//	pStack[i] = (void*)(ptrdiff_t)StackFrame[i].absolute_pc;
		//return nCount;
	#endif // _ANDROID
#endif
	}

};
