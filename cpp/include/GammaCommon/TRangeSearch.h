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

	template< class PosType, int32 nRange >
	class TRangeSearch
	{
		struct DistPos : public PosType
		{	
			bool operator < ( const DistPos& arg ) const
			{ 
				return int32(PosType::x)*int32(PosType::x) + int32(PosType::y)*int32(PosType::y) 
					< int32(arg.x)*int32(arg.x) + int32(arg.y)*int32(arg.y); 
			} 
		};

		enum{ nBufferSize = ( nRange*2 + 1 )*( nRange*2 + 1 )  };
		DistPos m_aryDisPos[nBufferSize];

	public:
		TRangeSearch()
		{
			for( int32 i = -nRange, n = 0; i <= nRange; i++ )
			{
				for( int32 j = -nRange; j <= nRange; j++, n++ )
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
