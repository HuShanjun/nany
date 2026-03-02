#pragma once
#include "CMemoryMgr.h"
#include "CGammaDebug.h"

namespace Gamma
{
	class CMgrInstance
	{
		char m_MemoryMgr[ TAligenUp< sizeof(CMemoryMgr), sizeof(ptrdiff_t) >::eValue ];
		char m_DebugMgr[ TAligenUp< sizeof(CGammaDebug), sizeof(ptrdiff_t) >::eValue ];
	public:
		CMgrInstance(void);
		~CMgrInstance(void);

		static CMgrInstance& Instance();

		CMemoryMgr&	GetMemoryMgrInstance();
		CGammaDebug& GetDebugMgrInstance();
	};
}
