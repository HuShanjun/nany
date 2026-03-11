//=====================================================================
// TRectMerge.h 
// 矩形的各种算法
// 柯达昭
// 2008-09-16
//=======================================================================

#ifndef _GAMMA_TRECT_MERGE_H_
#define _GAMMA_TRECT_MERGE_H_

#include "GammaCommon/TRect.h"
#include <vector>

namespace Gamma
{
	/// 矩形合并算法
	template <typename T>
	class TRectMerge : public TRect<T>
	{
		typedef T ItemType;
		typedef TRect<ItemType> TParent;
		typedef TParent RectType;
		std::vector< RectType >	m_vecRectFree;

		///< 加入一个空闲的矩形
		void AddFreeRect( RectType rt )
		{
			if( !rt.Width() || !rt.Height() )
				return;
			for( size_t i = 0; i < m_vecRectFree.size(); i++ )
			{
				RectType rtAnd = m_vecRectFree[i]&rt;
				if( rtAnd == rt )
					return;
				if( rtAnd == m_vecRectFree[i] )
				{
					m_vecRectFree[i] = rt;
					return;
				}
			}
			m_vecRectFree.push_back( rt );
		}

	public:
		TRectMerge()
		{
		}

		TRectMerge( const RectType& rtOrg )
			: RectType( rtOrg )
		{
			m_vecRectFree.push_back( rtOrg );
		}

		const std::vector<RectType>& GetFreeRect()
		{
			return m_vecRectFree;
		}

		void FreeRect( const RectType& rt )
		{
			AddFreeRect( rt );
		}

		void Reset()
		{
			m_vecRectFree.clear();
			m_vecRectFree.push_back( (RectType&)*this );
		}

		RectType UseRect( T w, T h )
		{
			GammaAst( w > 0 && h > 0 );

			size_t nIndex = 0;
			int fPercent = 0x7fffffff;
			for( size_t i = 0; i < m_vecRectFree.size(); i++ )
			{
				int fCurPercent = Min( m_vecRectFree[i].Width() - w, m_vecRectFree[i].Height() - h );
				if( fCurPercent < 0 || fCurPercent >= fPercent )
					continue;
				fPercent = fCurPercent;
				nIndex = i;
			}

			if( fPercent == 0x7fffffff )
				return RectType();

			RectType rtUse = m_vecRectFree[nIndex];
			rtUse.right = rtUse.left + w;
			rtUse.bottom = rtUse.top + h;

			for( size_t i = 0; i < m_vecRectFree.size(); )
			{
				RectType rtAnd = m_vecRectFree[i]&rtUse;
				if( !rtAnd.Width() || !rtAnd.Height() )
				{
					i++;
					continue;
				}

				RectType rtFree = m_vecRectFree[i];
				m_vecRectFree.erase( m_vecRectFree.begin() + i );

				RectType rtUseOnMe = rtUse&rtFree;
				if( rtUseOnMe.left == rtFree.left )
				{
					AddFreeRect( RectType( rtFree.left, rtFree.top, rtFree.right, rtAnd.top ) );
					AddFreeRect( RectType( rtAnd.right, rtFree.top, rtFree.right, rtFree.bottom ) );
					AddFreeRect( RectType( rtFree.left, rtAnd.bottom, rtFree.right, rtFree.bottom ) );
				}
				else if( rtUseOnMe.top == rtFree.top )
				{
					AddFreeRect( RectType( rtFree.left, rtFree.top, rtAnd.left, rtFree.bottom ) );
					AddFreeRect( RectType( rtFree.left, rtAnd.bottom, rtFree.right, rtFree.bottom ) );
					AddFreeRect( RectType( rtAnd.right, rtFree.top, rtFree.right, rtFree.bottom ) );
				}
				else if( rtUseOnMe.right == rtFree.right )
				{
					AddFreeRect( RectType( rtFree.left, rtFree.top, rtFree.right, rtAnd.top ) );
					AddFreeRect( RectType( rtFree.left, rtFree.top, rtAnd.left, rtFree.bottom ) );
					AddFreeRect( RectType( rtFree.left, rtAnd.bottom, rtFree.right, rtFree.bottom ) );
				}
				else if( rtUseOnMe.bottom == rtFree.bottom )
				{
					AddFreeRect( RectType( rtFree.left, rtFree.top, rtAnd.left, rtFree.bottom ) );
					AddFreeRect( RectType( rtFree.left, rtFree.top, rtFree.right, rtAnd.top ) );
					AddFreeRect( RectType( rtAnd.right, rtFree.top, rtFree.right, rtFree.bottom ) );
				}
				else
					GammaThrow( "Impossible!!!" );
			}

			return rtUse;
		}

		void Resize( T w, T h )
		{
			if( ( w == TParent::Width() && h == TParent::Height() ) || w < TParent::Width() || h < TParent::Height() )
				return;

			if( w > TParent::Width() )
			{
				for( size_t i = 0; i < m_vecRectFree.size(); i++ )
				{
					if( m_vecRectFree[i].right == TParent::right )
						m_vecRectFree[i].right = w;
				}
			}

			if( h > TParent::Height() )
			{
				for( size_t i = 0; i < m_vecRectFree.size(); i++ )
					if( m_vecRectFree[i].bottom == TParent::bottom )
						m_vecRectFree[i].bottom = h;
			}

			AddFreeRect( RectType( TParent::right, TParent::top, w, TParent::bottom ) );
			AddFreeRect( RectType( TParent::left, TParent::bottom, TParent::right, h ) );
			TParent::right = w;
			TParent::bottom = h;
		}
	};
}

#endif