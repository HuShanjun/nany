
#include "GammaCommon/PerlinNoise.h"
#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/GammaMath.h"

namespace Gamma
{
	inline float noise( int32_t x, int32_t y )   
	{   
		int32_t n = x + y*57;   
		n = ( n << 13 )^n;   
		return (float)( 1.0 - ( ( n * (n * n * 15731 + 789221 ) + 1376312589 ) & 0x7fffffff ) / 1073741824.0 );    
	}   

	inline float smoothedNoise( int32_t x, int32_t y )   
	{   
		float corners = ( noise( x - 1, y - 1 ) + noise( x + 1, y - 1 ) + noise( x - 1, y + 1 ) + noise( x + 1, y + 1) ) / 16;   
		float sides = ( noise( x - 1, y ) + noise( x + 1, y ) +noise( x, y - 1 ) + noise( x, y + 1 ) ) / 8;   
		float center = noise( x, y ) / 4;   
		return corners + sides + center;   
	}   

	inline float interpolate( float a, float b, float x )   
	{   
		float f = ( 1 - cos( x*3.141592658f ) )*0.5f;   
		return a*( 1 - f ) + b*f;   
		/*return a*(1-x) + b*x;*/   
	}   

	inline float interpolatedNoise( float x, float y )   
	{   
		int32_t intX = x < 0 ? (int32_t)x - 1 : (int32_t)x;
		int32_t intY = y < 0 ? (int32_t)y - 1 : (int32_t)y; 
		float fractionalX = x - intX;   
		float fractionalY = y - intY; 
		float v1 = smoothedNoise( intX, intY );   
		float v2 = smoothedNoise( intX + 1, intY );   
		float v3 = smoothedNoise( intX, intY + 1 );   
		float v4 = smoothedNoise( intX + 1, intY + 1 );
		float i1 = interpolate( v1, v2, fractionalX );   
		float i2 = interpolate( v3, v4, fractionalX ); 
		return interpolate(i1 , i2 , fractionalY);   
	}   

	float PerlinNoise_2D( float x, float y, int32_t octave_num, float persistence, float frequenceInc )   
	{       
		float total = 0;   
		float amplitude = 1;
		float frequency = 1; 
		for( int32_t i = 0; i < octave_num; ++i ) 
		{
			total += interpolatedNoise( x * frequency, y * frequency ) * amplitude;
			amplitude = amplitude * persistence;
			frequency = frequency * frequenceInc;
		}
		return total;   
	}

	GAMMA_COMMON_API void PerlinNoise2D( float* aryNoise, uint32_t nWidth, float persistence, float frequenceInc )
	{
		uint8_t nNum = GammaLog2( nWidth );
		GammaAst( nWidth == (uint32_t)( 1 << nNum ) );
		for( uint32_t i = 0; i < nWidth; i++ )
			for( uint32_t j = 0; j < nWidth; j++ )
				aryNoise[i*nWidth+j] = PerlinNoise_2D( (float)j, (float)i, nNum, persistence, frequenceInc );
	}

}
