//=====================================================================
// GammaHash.h 
// 定义字符串哈希函数
// 柯达昭
// 2007-09-15
//=======================================================================

#ifndef __GAMMA_HASH_H__
#define __GAMMA_HASH_H__

#include "GammaCommonType.h"

namespace Gamma
{
	uint32_t GAMMA_COMMON_API GammaHash( const void* pMem, size_t size );
}
#endif
