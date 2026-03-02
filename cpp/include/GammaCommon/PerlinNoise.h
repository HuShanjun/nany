//=====================================================================
// GammaHash.h 
// 定义字符串哈希函数
// 柯达昭
// 2007-09-15
//=======================================================================

#ifndef __PERLINNOISE_H__
#define __PERLINNOISE_H__

#include "GammaCommonType.h"

namespace Gamma
{
	float	GAMMA_COMMON_API PerlinNoise_2D( float x, float y, int32 octave_num, float persistence = 0.25f, float frequenceInc = 2.0f );
	void	GAMMA_COMMON_API PerlinNoise2D( float* aryNoise, uint32 nWidth, float persistence = 0.25f, float frequenceInc = 2.0f );
}
#endif
