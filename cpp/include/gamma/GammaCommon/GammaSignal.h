//===============================================
// GammaSignal.h 
// 定义平台信号函数
// 柯达昭
// 2017-07-26
//===============================================

#ifndef __GAMMA_SIGNAL_H__
#define __GAMMA_SIGNAL_H__

#include "GammaCommon/GammaCommonType.h"
#include <signal.h>

//#define SIGHUP           1
//#define SIGINT           2
//#define SIGQUIT          3
//#define SIGILL           4
//#define SIGTRAP          5
//#define SIGABRT          6
//#define SIGIOT           6
//#define SIGBUS           7
//#define SIGFPE           8
//#define SIGKILL          9
//#define SIGUSR1         10
//#define SIGSEGV         11
//#define SIGUSR2         12
//#define SIGPIPE         13
//#define SIGALRM         14
//#define SIGTERM         15
//#define SIGSTKFLT       16
//#define SIGCHLD         17
//#define SIGCONT         18
//#define SIGSTOP         19
//#define SIGTSTP         20
//#define SIGTTIN         21
//#define SIGTTOU         22
//#define SIGURG          23
//#define SIGXCPU         24
//#define SIGXFSZ         25
//#define SIGVTALRM       26
//#define SIGPROF         27
//#define SIGWINCH        28
//#define SIGIO           29
//#define SIGPOLL         SIGIO
///*
//#define SIGLOST         29
//*/
//#define SIGPWR          30
//#define SIGSYS          31
//#define SIGUNUSED       31

namespace Gamma
{
	typedef void (*SignalHandler)( int32_t, void* );

	// 安装信号处理，触发信号后会被重置成默认行为
	GAMMA_COMMON_API void InstallSignalHandler( int32_t nSignal, SignalHandler funSigHandler );

	// 永远忽略某个信号
	GAMMA_COMMON_API void IgnoreSignal( int32_t nSignal );

	// 产生某个信号
	GAMMA_COMMON_API void RaiseSignal( int32_t nSignal );
}

#endif
