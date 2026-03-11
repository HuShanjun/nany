//======================================================
// 文件内存映射
//======================================================
#ifndef __GAMMA_FILE_MAP_H__
#define __GAMMA_FILE_MAP_H__

#include "GammaCommon/GammaCommonType.h"

namespace Gamma
{
	typedef struct SFileMapInfo* HFILEMAP;

	//=========================================================
	// 建立文件内存映射
	// szFile 文件名
	// bReadOnly 是否只读
	// nOffset 文件的起始偏移
	// nSize 映射内存的大小，如果超过文件大小将会映射到文件末尾
	// nTruncateSize 将文件截取到指定大小
	//		不指定的话，文件必须存在，否则返回NULL
	//		如果指定，文件不存在则创建，存在则截取到指定长度
	// 返回值为内存映射句柄
	//=========================================================
	GAMMA_COMMON_API HFILEMAP GammaMemoryMap( const char* szFile, 
		bool bReadOnly, uint32_t nOffset, uint32_t nSize, uint32_t nTruncateSize );

	//=========================================================
	// 重新内存映射
	// bReadOnly 是否只读
	// nOffset 文件的起始偏移
	// nSize 映射内存的大小，如果超过文件大小将会映射到文件末尾
	// nTruncateSize 将文件截取到指定大小
	//		不指定的话，文件必须存在，否则返回NULL
	//		如果指定，文件不存在则创建，存在则截取到指定长度
	// 返回值 是否成功
	//=========================================================
	GAMMA_COMMON_API bool GammaMemoryRemapMap( HFILEMAP hMap,
		bool bReadOnly, uint32_t nOffset, uint32_t nSize, uint32_t nTruncateSize );

	//=========================================================
	// 得到内存映射地址
	//=========================================================
	GAMMA_COMMON_API void* GammaGetMapMemory( HFILEMAP hMap );

	//=========================================================
	// 得到内存映射大小
	//=========================================================
	GAMMA_COMMON_API uint32_t GammaGetMapMemorySize( HFILEMAP hMap );

	//=========================================================
	// 解除文件内存映射
	//=========================================================
	GAMMA_COMMON_API void GammaMemoryUnMap( HFILEMAP hMap );

	//=========================================================
	// 同步文件内存映射的内容到文件
	// nOffset 相对于文件映射（不是文件的偏移）
	// nSize 同步的大小，如果超过映射大小将会只同步映射的最大值
	//=========================================================
	GAMMA_COMMON_API void GammaMemoryMapSyn( HFILEMAP hMap, size_t nOffset, size_t nSize, bool bAsync );
};

#endif
