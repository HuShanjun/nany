//=====================================================================
// GammaMemory.h 
// 内存分配器
// 柯达昭
// 2010-07-26
//=======================================================================

#ifndef _GAMMA_MEMORY_H_
#define _GAMMA_MEMORY_H_

#include "GammaCommon/GammaCommonType.h"

// 检查内存泄露
//#define CHECK_MEMORY_ERROR
//#define MEMORY_LEASK	

namespace Gamma
{
	#define VIRTUAL_PAGE_READ		0x01
	#define VIRTUAL_PAGE_WRITE		0x02
	#define VIRTUAL_PAGE_EXECUTE	0x04

	//========================================================================
	// 获取内存页大小
	//========================================================================
	GAMMA_COMMON_API uint32 GetVirtualPageSize();

	//========================================================================
	// 保留一段内存
	//========================================================================
	GAMMA_COMMON_API void* ReserveMemoryPage( void* pAddress, size_t nSize );

	//========================================================================
	// 释放保留区域保留一段内存
	//========================================================================
	GAMMA_COMMON_API bool FreeMemoryPage( void* pAddress, size_t nSize );

	//========================================================================
	// 提交一段内存
	//========================================================================
	GAMMA_COMMON_API bool CommitMemoryPage( void* pAddress, size_t nSize, uint32 nProtectFlag );

	//========================================================================
	// 取消提交的一段内存
	//========================================================================
	GAMMA_COMMON_API bool DecommitMemoryPage( void* pAddress, size_t nSize );

	//========================================================================
	// 分配被管理的内存 
	//========================================================================
	GAMMA_COMMON_API void* GammaAlloc( size_t nSize, void* pCallAddress = NULL );

	//========================================================================
	// 释放被管理的内存 
	//========================================================================
	GAMMA_COMMON_API void  GammaFree( void* );

	//========================================================================
	// 被管理的已分配内存大小
	//========================================================================
	GAMMA_COMMON_API int64  GetTotalAllocSize();

	//========================================================================
	// 被管理的内存总大小
	//========================================================================
	GAMMA_COMMON_API int64  GetTotalMgrSize();

	//========================================================================
	// 被管理的空闲内存大小
	//========================================================================
	GAMMA_COMMON_API int64  GetFreeMgrSize();

	//========================================================================
	// 得到在pAddress处分配的总内存数量
	//========================================================================
	GAMMA_COMMON_API uint32  GetAllocStack( uint32 nIndex, void**& pAddress, uint32& nAllocSize );

	//========================================================================
	// Dump内存信息
	//========================================================================
	GAMMA_COMMON_API void    DumpMemoryInfo();
	GAMMA_COMMON_API void    ClearMemoryInfo();
	//========================================================================
	// 页分配器
	//========================================================================
	template< uint32 nPageSize = 8192, uint32 nFlag = VIRTUAL_PAGE_READ|VIRTUAL_PAGE_WRITE >
	class TFixedPageAlloc
	{
	public:
		enum { ePageSize = nPageSize, eMemoryType = nFlag };
		static void* Alloc() 
		{ 
			void* pBuffer = ReserveMemoryPage( NULL, ePageSize );
			CommitMemoryPage( pBuffer, ePageSize, eMemoryType );
			return pBuffer;
		}

		static void Free( void* p ) 
		{ 
			DecommitMemoryPage( p, ePageSize );
			FreeMemoryPage( p, ePageSize );
		};
	};
}

#endif
