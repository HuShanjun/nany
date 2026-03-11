#pragma once
#ifdef __linux__
#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/GammaPlatform.h"
namespace Gamma
{   
	class CLinuxApp
	{
		SHardwareDesc			m_HardwareDesc;

		CLinuxApp(void);
		~CLinuxApp(void);

		void					FetchHardwareInfo();
	public:
		static CLinuxApp&		GetInstance();	
		void					GetHardwareDesc( SHardwareDesc& HardwareDesc ){ HardwareDesc = m_HardwareDesc; }
		void*					LoadDynamicLib( const char* szName );
		void*					GetFunctionAddress( void* pLibContext, const char* szName );
	};
}

#endif