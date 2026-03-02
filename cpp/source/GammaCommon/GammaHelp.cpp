
#include "CGammaDebug.h"
#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/GammaCodeCvs.h"
#include "GammaCommon/TGammaStrStream.h"
#include "CMemoryMgr.h"
#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
#include <Windows.h>
//#else
//#include "google-dump/google/coredumper.h"
#endif

#pragma warning(disable:4740)

namespace Gamma
{
	void DebugGenMiniDump( void* pContext, const char* szFileName, bool bFullMemDump )
	{
#ifdef _WIN32
		if( !pContext )
			return;
		_EXCEPTION_POINTERS* pException = (_EXCEPTION_POINTERS*)pContext;
		wchar_t szFileNameByAddress[256];
		wgammasstream( szFileNameByAddress ) << szFileName << "." 
			<< pException->ExceptionRecord->ExceptionAddress << ".dmp";
		CGammaDebug::Instance().DebugGenMiniDump( pException, szFileNameByAddress, bFullMemDump );
#else
	#ifdef _ANDROID
		if( pContext )
		{
		#if defined( __arm__ )
			struct ucontext_t 
			{
				unsigned long     	uc_flags;
				struct ucontext_t* 	uc_link;
				stack_t       		uc_stack;
				struct sigcontext 	uc_mcontext;
				sigset_t      		uc_sigmask;   /* mask last for extensibility */
			};

			struct frame_t
			{
				void*				fm_pc;
				void*				fm_lr;
				void*				fm_sp;
				void*				fm_fp;
			};

			ucontext_t* pInfo = (ucontext_t*)pContext;
			frame_t* pFP = (frame_t*)(ptrdiff_t)pInfo->uc_mcontext.arm_fp;
			void* aryPC[2] = { (void*)(ptrdiff_t)pInfo->uc_mcontext.arm_pc, pFP->fm_pc };
			GammaLog << "Cur Stack:" << (uint64)(ptrdiff_t)&pContext << std::endl;
		#endif
		}
	#elif !( defined _IOS )
		//WriteCoreDump( szFileName );
	#endif
#endif
	}

	void DebugAddress2Symbol( void* pAddress, char* szSymbolBuf, uint32 nSize )
	{
		CGammaDebug::Instance().DebugAddress2Symbol( pAddress, szSymbolBuf, nSize );
	}

	size_t GetStack( void** pStack, uint16 nBegin, uint16 nEnd )
	{
		return CGammaDebug::Instance().GetStack( pStack, nBegin, nEnd );
	}

	size_t PrintStack( uint16 nMax, int32 nLine, std::ostream& Stream )
	{
		void** pStack = new void*[nMax];
		size_t nSize = CGammaDebug::Instance().GetStack( pStack, 2, 2 + nMax, &nLine );
		size_t nCount = PrintStack( pStack, nSize, nLine, Stream );
		delete [] pStack;
		return nCount;
	}

	size_t PrintStack( void** pStack, size_t nSize, int32 nLine, std::ostream& Stream )
	{
		char szBuf[1024];
		Stream << std::showbase;
		for( size_t i = 0; i < nSize && pStack[i]; i++ )
		{
			DebugAddress2Symbol( pStack[i], szBuf, 1023 );
			Stream << "[" << std::hex << std::setw( 8 ) << pStack[i] << std::dec << "]";
			Stream << szBuf;
			if( !i )
				Stream << ":" << nLine;
			Stream << std::endl;
		}
		Stream << std::noshowbase;

		return nSize;
	}

	size_t PrintStack2File( void** pStack, size_t nSize, FILE* pFile )
	{
		char szBuf[1024];
		char szPointer[32];
		for( size_t i = 0; i < nSize && pStack[i]; i++ )
		{
			DebugAddress2Symbol( pStack[i], szBuf, 1023 );
			gammasstream( szPointer ) << (ptrdiff_t)pStack[i];
			fprintf( pFile, "%s()[%s]\n", szBuf, szPointer);
		}

		fprintf( pFile, "\n\n\n" );
		return nSize;
	}

	void _GlobalAssertDefault( const char* exp, const char* szFile, uint32 nLine )
	{
		GammaLog << exp << " in file " << szFile << ":" << nLine << std::endl;
	}

	GlobalAssertFun& _GetGlobAssertFun()
	{
		static GlobalAssertFun s_GlobalAssertFun = &_GlobalAssertDefault;
		return s_GlobalAssertFun;
	}

	void SetGlobAssertFun( GlobalAssertFun funGlobalLog )
	{
		_GetGlobAssertFun() = funGlobalLog;
	}

	GlobalAssertFun GetGlobAssertFun()
	{
		return _GetGlobAssertFun();
	}

	void GammaException( const char* szMsg, const char* szFile, const char* szDate, 
		const char* szTime, uint32 nLine, const char* szFunction, bool bForceAbort )
	{
		std::string sFile = GammaString( szFile );
		const char* szFileName = strrchr( sFile.c_str(), '/' );
		szFileName = szFileName ? szFileName + 1 : sFile.c_str();

		std::string szText;
		gammasstream( szText ) << ( szMsg ) << std::endl 
			<< ( szFileName ) << std::endl 
			<< ( szDate ) << std::endl 
			<< ( szTime ) << std::endl 
			<< "Line:" << nLine << std::endl 
			<< ( szFunction ) << std::endl;
		GammaLog << "GammaException: \n" << szText.c_str();		

#ifndef _WIN32
		if( bForceAbort )
			abort();
#else
		if( bForceAbort )
		{
			if( MessageBoxA( NULL, szText.c_str(), "Exception", MB_OK|MB_ICONERROR ) == IDOK )
				ExitProcess( 0 );
		}
		else
		{
			switch( MessageBoxA( NULL, szText.c_str(), "Exception", MB_ABORTRETRYIGNORE|MB_ICONERROR ) )
			{
			case IDABORT:
				abort();
			case IDIGNORE:
				break;
			case IDRETRY:
				DebugBreak();
				break;
			}
		}
#endif
	}
	GAMMA_COMMON_API uint32 GetCpuSuport()
	{
		static uint32 nSupport = INVALID_32BITID;
		if( nSupport != INVALID_32BITID )
			return nSupport;

#if( defined _WIN64 )
		nSupport = eMMX|eSSE|eSSE2;
#elif( defined _WIN32 )
		nSupport = 0;
		uint32 nFeature = 0;
		__try 
		{		
			__asm mov eax, 1				;
			__asm cpuid						;
			__asm mov nFeature, edx			;
		}
		__except( EXCEPTION_EXECUTE_HANDLER )
		{
			return 0;
		}

		//暂时禁止对3DNOW的检测
		nFeature &= ~e3DNOW;
		int32 nFlage[4] = { eMMX, eSSE, eSSE2, e3DNOW };

		for( size_t i = 0; i < 4; i++ )
		{
			if( ( nFeature&nFlage[i] ) == 0 )
				continue;

			__try 
			{
				switch( nFlage[i] )
				{
				case eMMX:
					__asm pxor mm0, mm0				;// executing MMX instruction
					__asm emms						;
					break;
				case eSSE:
					__asm xorps xmm0, xmm0			;// executing SSE instruction
					break;
				case eSSE2:
					__asm xorpd xmm0, xmm0			;// executing SSE2 instruction
					break;
				case e3DNOW:
					__asm pfrcp mm0, mm0			;// executing 3DNow! instruction
					__asm emms						;
					break;
				}
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				continue;
			}

			nSupport |= nFlage[i];
		}
#endif
		return nSupport;
	}
}	
