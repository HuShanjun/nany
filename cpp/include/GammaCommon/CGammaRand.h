//===============================================
// CGammaRand.h
// 定义常用随机函数
// 柯达昭
// 2007-08-31
//===============================================

#ifndef __GAMMA_RAND_H__
#define __GAMMA_RAND_H__

#include "GammaCommonType.h"
#include <time.h>
#include <math.h>
#include "GammaMath.h"

namespace Gamma
{
    class CGammaRand
    {
        int32 nRandSeed;
        bool bBoxmullerGen;
        float fBoxmullerNext;

    public:
        CGammaRand( int32 nSeed )
        {
            nRandSeed = nSeed;
            bBoxmullerGen = false;
        }

        CGammaRand()
        {
            nRandSeed = (int32)time(NULL);
            bBoxmullerGen = false;
        }

        operator int32()
        {
            return ( ( ( nRandSeed = nRandSeed * 214013L + 2531011L) >> 16 ) & 0x7fff );
        }

        int32 operator = ( int32 nSeed )
        {
            return nRandSeed = nSeed;
		}

		int32 SetSeed( int32 nSeed ) 
		{
			return nRandSeed = nSeed;
		}

        int32 GetSeed() const
        {
            return nRandSeed;
		}

		int32 GetRand()
		{
			return (int32)( *this );
		}

        bool Percent(int32 Pct)
        {
            return GetRand(0.0f, 1.0f) * 100 < Pct;
        }

		float BoxMuller()
		{
			if (bBoxmullerGen)
			{
				bBoxmullerGen = false;
				return fBoxmullerNext;
			}
			float u1 = 1.0f - GetRand(0.0f, 1.0f);
			float u2 = GetRand(0.0f, 1.0f);
			float radius = sqrtf(-2.0f * logf(u1));
			float theta = GM_2PI * u2;
			float result = radius * cosf(theta);
			fBoxmullerNext = radius * sinf(theta);
			bBoxmullerGen = true;
			return result;
		}

		template<class T>
		T GetRand( const T& Min, const T& Max )
		{ 
			return (T)( (*this)*( Max - Min )/0x8000 + Min ); 
		}

		/** @remark 正态分布随机值。Mean为均值，StdDev为标准差 */
		template<class T>
		T GaussianRand(const T& Mean, const T& StdDev)
		{
			return BoxMuller() * StdDev + Mean;
		}

		template<typename T>
		void Shuffle( T* pElems, uint32 nElems )
		{
			for( uint32 i = 0; i < nElems; i++ )
			{
				uint32 r = GetRand<uint32>( i, nElems );
				if( r == i )
					continue;
				T temp = pElems[i];
				pElems[i] = pElems[r];
				pElems[r] = temp;
			}
		}

        /** @remark [min, max) 右开区间 */
        template<class T>
        static T Rand( const T& Min, const T& Max )
        { 
            static CGammaRand sGlob;
            return sGlob.GetRand<T>( Min, Max ); 
        }

        /** @remark [min, max) 右开区间 */
        template<class T>
        static T RandEx( const T& Min, const T& Max )
        { 
            static CGammaRand sGlob;
            int32 nFirst = sGlob;
            int32 nSecond = sGlob;
            double f = (nFirst<<15)|nSecond;
            return (T)( f*( Max - Min )/0x40000000 + Min ); 
        }

		static CGammaRand& Inst()
		{
			static CGammaRand g_inst;
			return g_inst;
		}
    };
}

#endif
