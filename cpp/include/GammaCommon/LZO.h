//==============================================================
// LZO.h
// 定义LZO压缩算法，来源于http://baike.baidu.com/view/5767680.htm?tp=3_01
// 柯达昭
// 2013-07-17
//==============================================================
#include "GammaCommon/GammaCommonType.h"

namespace Gamma
{
	GAMMA_COMMON_API int32 lzo_compress( const tbyte* in, uint32 in_len, tbyte* out );
	GAMMA_COMMON_API int32 lzo_decompress( const tbyte* in, uint32 in_len, tbyte* out );
}
