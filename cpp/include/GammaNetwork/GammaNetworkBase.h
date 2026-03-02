#ifndef _GAMMA_NETWORK_BASE_H_
#define _GAMMA_NETWORK_BASE_H_

#include "GammaCommon/GammaHelp.h"

#if ( defined( _WIN32 ) && defined( GAMMA_DLL ) )
	#if defined( GAMMA_NETWORK_EXPORTS )
		#define GAMMA_NETWORK_API __declspec(dllexport)
	#else
		#define GAMMA_NETWORK_API __declspec(dllimport)
	#endif
#else
	#define GAMMA_NETWORK_API 
#endif

#endif
