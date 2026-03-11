//=========================================================================
// TRangeSearch.h 
// 定义用于生成由中心到周围的一个坐标序列
// 柯达昭
// 2008-01-26
//=========================================================================
#ifndef __TRANGE_SEARCH_H__
#define __TRANGE_SEARCH_H__

#include <algorithm>

namespace Gamma
{

	template< class PosType, int32_t nRange >
	class TRangeSearch
	{
		struct DistPos : public PosType
		{	
			bool operator < ( const DistPos& arg ) const
			{ 
				return int32_t(PosType::x)*int32_t(PosType::x) + int32_t(PosType::y)*int32_t(PosType::y) 
					< int32_t(arg.x)*int32_t(arg.x) + int32_t(arg.y)*int32_t(arg.y); 
			} 
		};

		enum{ nBufferSize = ( nRange*2 + 1 )*( nRange*2 + 1 )  };
		DistPos m_aryDisPos[nBufferSize];

	public:
		TRangeSearch()
		{
			for( int32_t i = -nRange, n = 0; i <= nRange; i++ )
			{
				for( int32_t j = -nRange; j <= nRange; j++, n++ )
				{ 
					m_aryDisPos[n].x = j;
					m_aryDisPos[n].y = i;
				}
			}
			std::sort( m_aryDisPos, m_aryDisPos + nBufferSize );
		}	

		const PosType*	GetArr() const	{ return m_aryDisPos; };
		size_t			GetSize() const { return nBufferSize; }
	};
}

#endif
