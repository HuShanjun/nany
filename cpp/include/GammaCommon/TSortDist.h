//==============================================================
// TSortDist.h  
// 定义向量按距离的排序
// 柯达昭
// 2007-09-10
//==============================================================

#ifndef _GAMMA_SORT_DIST_H_
#define _GAMMA_SORT_DIST_H_

#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/TVector2.h"
#include <algorithm>

namespace Gamma
{
	template<class T, uint32 nRange = 100 >
	class TSortDist
	{
	public:
		enum 
		{ 
			nRadius	   = nRange,
			nDiameter  = ( nRange*2 + 1 ),
			nArraySize = nDiameter*nDiameter,
		};

		TSortDist()
		{
			for( T i = 0; i < nDiameter; i++ )
				for( T j = 0; j < nDiameter; j++ )
					m_vUnitSortbyDist[i*nDiameter + j] = TDistGrid( (T)( j - nRange ), (T)( i - nRange ) );
			std::sort( m_vUnitSortbyDist, m_vUnitSortbyDist + nArraySize );
		}

		size_t GetGridCount() const
		{
			return nArraySize;
		}

		TVector2<T> GetGrid( size_t nIndex ) const
		{
			return m_vUnitSortbyDist[nIndex];
		}

		T GetGridDistSqr( size_t nIndex ) const
		{
			return m_vUnitSortbyDist[nIndex].nDist;
		}

		uint32 GetRadius() const { return nRadius; }

private:
		struct TDistGrid : public TVector2<T>
		{
			T nDist;

			TDistGrid(){}
			TDistGrid( T x, T y ) : TVector2<T>( x, y ), nDist( x*x + y*y ){}
			bool operator< ( const TDistGrid& r ) const { return nDist < r.nDist; };
		};

		TDistGrid m_vUnitSortbyDist[nArraySize];

	};
}

#endif
