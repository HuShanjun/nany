/*
*	CLine结构描述了3维空间中的线
*/

#ifndef __LINE_SEGMENT_H__
#define __LINE_SEGMENT_H__

#include "GammaCommon/TVector3.h"
#include "GammaCommon/CMatrix.h"

namespace Gamma
{
	template<class T> 
	struct TLineSegment
	{
		// 线段起点
		TVector3<T>	m_vStartPoint;
		// 线段的方向（包含长度）
		TVector3<T>	m_vDirection;

		TLineSegment(void);
		TLineSegment( const TVector3<T> (&aryPoint)[2] )
			: m_vStartPoint( aryPoint[0] ), m_vDirection( aryPoint[1] - aryPoint[2] ){}
		TLineSegment( const TVector3<T>& vPoint, const TVector3<T>& vDirection )
			: m_vStartPoint( vPoint ), m_vDirection( vDirection ){}

		// 线段上离vPoint最近的点
		inline T ClosetPointSegment( const TVector3<T>& vPoint, TVector3<T>* vOut );

		// 根据坐标轴的一点得到线段上的点
		inline TVector3<T> GetPosition( T v, int32_t eAxis );

		// 根据坐标轴的一点得到线段上的点到起点的距离
		inline T GetDistFromStart( T v, int32_t eAxis );
	};

	/*!
	\param vPoing 考察的点
	\param vOut 线段上离vPoint最近的点
	\return vOut离起点的距离
	*/
	template<class T> inline T TLineSegment<T>::ClosetPointSegment( const TVector3<T>& vPoint, TVector3<T>* vOut )
	{
		TVector3<T> vDelta = vPoint - m_vStartPoint;
		T l = m_vDirection.Dot( m_vDirection );
		T t = l == 0 ? 0 : vDelta.Dot( m_vDirection )/l;
		if( vOut )
			*vOut = m_vDirection * ( t < 0 ? 0 : ( t > 1 ? 1 : t ) ) + m_vStartPoint;
		return t;
	}

	// 根据坐标轴的一点得到线段上的点
	template<class T> inline TVector3<T> TLineSegment<T>::GetPosition( T v, int32_t eAxis )
	{
		GammaAst( m_vDirection.v[eAxis] != 0 );
		return m_vStartPoint + m_vDirection*( ( v - m_vStartPoint.v[eAxis] )/m_vDirection.v[eAxis] );
	}

	// 根据坐标轴的一点得到线段上的点到起点的距离
	template<class T> inline T TLineSegment<T>::GetDistFromStart( T v, int32_t eAxis )
	{
		GammaAst( m_vDirection.v[eAxis] != 0 );
		return m_vDirection.Len()*( ( v - m_vStartPoint.v[eAxis] )/m_vDirection.v[eAxis] );
	}
}

typedef Gamma::TLineSegment<float> CLineSegmentf;

#endif
