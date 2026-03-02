
#include "CMgrInstance.h"

namespace Gamma
{
	CMgrInstance::CMgrInstance(void)
	{
		new ( m_MemoryMgr )CMemoryMgr();
		new ( m_DebugMgr  )CGammaDebug();	
	}

	CMgrInstance::~CMgrInstance(void)
	{
		GetMemoryMgrInstance().~CMemoryMgr();
	}

	CMemoryMgr& CMgrInstance::GetMemoryMgrInstance()
	{
		return *( (CMemoryMgr*)m_MemoryMgr );
	}

	CGammaDebug& CMgrInstance::GetDebugMgrInstance()
	{
		return *( (CGammaDebug*)m_DebugMgr );
	}

	CMgrInstance& CMgrInstance::Instance()
	{
		static CMgrInstance _Instance;
		return _Instance;
	}
}
