/*
*	CPlane结构描述了3维空间中的平面
*  平面到点的距离是有正负的，和法矢的方向一致为正，否则为负值
if the normal pointing away from the origin, which causes d to be negative 
if the normal pointing towards the origin, so d is positive.
if the plane goes through the origin, d is zero 

*  平面的方程是	：	X*m_Normal.x + Y*Normal.y + Z*Normal.z + m_Dist = 0;
平面的法矢已经单位化了

有几个函数没有完成，比如平面分割线段，平面分割多边形，两平面的交线，三平面的交点
*/

#ifndef __PLANE_H__
#define __PLANE_H__

#include "GammaCommon/TVector3.h"
#include "GammaCommon/CMatrix.h"

namespace Gamma
{
    /// 直线和平面的相交关系
    enum ELineIntersectType
    {
        eLineOnPlane = -2,           /// 在面上
        eLineParallelInside = -1,    /// 平行在面内
        eLineParallelOutside = 0,   /// 平行在面外
        eLineIntersect = 1,         /// 是否相交
        eLineIntersectInside,   /// 交点在线段内
        eLineIntersectOutside,  /// 交点在线段外
    };

	class CPlane 
	{
	public:
		enum ESide
		{
			eSide_Positive,
			eSide_Negtive,
			eSide_Both
		};

		// ax + by + cz + d = 0;
		// 平面的法向矢量，是单位矢量( a, b, c )
		CVector3f		m_Normal;
		// 平面方程的d，相当于平面离坐标原点的距离的负值
		float			m_Dist;

		//=======================================================================
		// 构造函数
		//=======================================================================
		CPlane() {}

		/*!
		\param  x    n_x 
		\param  y    n_y 
		\param  z    n_z 
		\param  d   the negative of the signed distance from
        the origin to the plane

		Construct a plane with a unit normal  (x,y,z)  and
		the negative of the signed distance from the origin
		(0,0,0) to the plane.
		*/
		CPlane(float x, float y, float z, float d) : m_Normal( CVector3f( x, y, z ).Unit() ),m_Dist(d){}

		/*!
		\param  n   a unit normal
		\param  d   the negative of the signed distance from
		Construct a plane with a unit normal n and
		the negative of the signed distance from the origin
		(0,0,0) to the plane.
		*/
		CPlane(const CVector3f& n, float d) : m_Normal( n.Unit() ), m_Dist(d){}

		/*!
		\param  n   a unit normal
		\param  p   a point on the plane
		Construct a plane with a unit normal  and a point on the plane.
		*/
		CPlane( const CVector3f& n,  const CVector3f& p ) : m_Normal( n.Unit() ), m_Dist( -n.Unit().Dot(p) ){}


		// 用平面上的三个点构造一个平面, 三个点按照顺时针排列，左手规则
		CPlane(const CVector3f& a, const CVector3f& b, const CVector3f& c)
		{
			m_Normal	= (b-a)^(c-a);	// 叉积
			m_Normal.Normalize();
			m_Dist		= -(m_Normal*a);		// 点积
		}

		//===================================================================================
		// 初始化函数
		//===================================================================================
		/*!
		\param  n   a unit normal
		\param  P   a point on the plane

		Initialize a plane with a unit normal and
		a point on the plane.
		*/
		void Init(const CVector3f& n, const CVector3f& P) 
		{
			m_Normal	= n;
			m_Dist		= -n.Dot(P);
		}

		/*!
		\param  n   a unit normal
		\param  P   a point on the plane

		Initialize a plane with a unit normal  n and
		the negative of the signed distance from the origin
		(0,0,0) to the plane.
		*/
		void Init(const CVector3f& n, float d) 
		{
			m_Normal = n;
			m_Dist = d;
			m_Normal.NormalizeNoneZero();
		}

        // 用平面上的三个点构造一个平面, 三个点按照顺时针排列，左手规则
        void Init(const CVector3f& a, const CVector3f& b, const CVector3f& c)
        {
            m_Normal	= (b-a)^(c-a);	// 叉积
            m_Normal.NormalizeNoneZero();
            m_Dist		= -(m_Normal*a);		// 点积
        };

		// return the unit normal of the plane
		CVector3f& GetNormal() 
		{
			return m_Normal;
		}

		// return the negative of the signed distance from the origin (0,0,0) to the plane
		float& GetDist() 
		{
			return m_Dist;
		}

		/*!
		\param  p   a 3D point
		\return     the signed distance from the plane to the point p

		Compute the signed distance from the plane to the point p
		点到平面的距离，有正负之分
		*/
		float DistTo(const CVector3f& p) const 
		{
			return m_Normal.Dot(p) + m_Dist;
		}

		/*!
		\return     a plane with the values \f$ \left{ -{\bf n}, -dist \right} \f$

		Negate the values of m_Normal and m_Dist.  This creates a plane
		that is a reflection about the origin of the first (so they are parallel),
		and whose normal points the opposite direction.
		*/
		const CPlane operator-() 
		{
			return CPlane(-m_Normal, -m_Dist);
		}

		//******************************************************************************
		/*! \fn     const CPlane operator* ( const CMatrix& mat ) 
		*   \brief  平面的矩阵转换
		*   \param  转换的矩阵
		*   \return 转换后的平面
		*******************************************************************************/
		const CPlane operator* ( const CMatrix& mat ) const
		{
			CVector3f p[2] = { CVector3f( 1, 0, 0 ), CVector3f( 0, 1, 0 ) };
			if( m_Normal.z < -0.99999f )
			{
				p[0] = CVector3f( 0, 1, 0 );
				p[1] = CVector3f( 1, 0, 0 );
			}
			else if( m_Normal.z < 0.99999f )
			{
				p[0] = m_Normal.Cross( CVector3f( 0, 0, 1 ) );
				p[1] = m_Normal.Cross( p[0] );
			}

			CVector3f vBase = ( -m_Normal*m_Dist );
			return CPlane( vBase*mat, ( vBase + p[0]*m_Dist )*mat, ( vBase + p[1]*m_Dist )*mat );
		}

		//******************************************************************************
		/*! \fn     CVector3f Split( const CVector3f& a, const CVector3f& b ) const
		*   \brief  计算一个线段确定的直线和平面的交点
        *   \param[out] vIntersect 交点
		*   \param  const CVector3f& a, const CVector3f& b 两点确定的直线
		*   \return ELineIntersectType
		*******************************************************************************/
		ELineIntersectType Split( CVector3f& vIntersect, const CVector3f& a, const CVector3f& b ) const
		{
			float aDot = (a * m_Normal);
			float bDot = (b * m_Normal);

            if ( aDot == bDot )
            {                
                float d = aDot + m_Dist;
                if ( d > 0 )
                    return eLineParallelInside;
                else if ( d < 0 )
                    return eLineParallelOutside;
                else
                    return eLineOnPlane;                                
            }

			float scale = ( -m_Dist - aDot) / ( bDot - aDot );
			vIntersect = a + (scale * (b - a));
            return eLineIntersect;
		}

        /** a、b的线段和平面是否有交点
        */
        ELineIntersectType SplitSeg( CVector3f& vIntersect, const CVector3f& a, const CVector3f& b ) const
        {
            float aDot = (a * m_Normal);
            float bDot = (b * m_Normal);

            if ( aDot == bDot )
            {                
                float d = aDot + m_Dist;
                if ( d > 0 )
                    return eLineParallelInside;
                else if ( d < 0 )
                    return eLineParallelOutside;
                else
                    return eLineOnPlane;
            }

            float scale = ( -m_Dist - aDot) / ( bDot - aDot );
            if ( scale < 0 || scale > 1 )
                return eLineIntersectOutside;

            vIntersect = a + (scale * (b - a));
            return eLineIntersectInside;
        }

        /// 检测射线和平面相交
        bool IsRayIntersect( const CVector3f& vStart, const CVector3f& vDir, CVector3f& vIntersect ) const
        {
            float fDot = vDir * m_Normal;
            if ( fDot == 0 )
                return false;

            float a = ( -m_Dist - vStart * m_Normal ) / fDot;
            if ( a < 0 )
                return false;

            vIntersect = vStart + a * vDir;
            return true;
        }

		//******************************************************************************
		/*! \fn     bool TVector3<T>::Mirror(const CPlane& Plane)
		*   \brief  矢量关于一个平面的镜像
		*           将自身关于一个传入的平面做镜像, 自身的值将改变
		*   \param  const CPlane& Plane 平面
		*   \return bool	false 表明点在平面上  
		*******************************************************************************/
		bool Mirror(CVector3f& vec)
		{
			float		fDist		= DistTo(vec);//GetPointDist(vec);     //GetPointDist()怎么不见了？－lily
			CVector3f	vNormal		= GetNormal();	

			if ( /*gBMIsZero(fDist)*/ fabs( fDist ) <= 0.000001f )//gBMIsZero()怎么不见了？－lily
				return false;
			else if (fDist > 0.0f)
			{
				vNormal = -vNormal;
			}

			vec = vec + vNormal*fDist*2.0f;
			return true;
		}

        /** 求和另一个平面的交线
        @param intersectLine 返回交线的单位向量
        @return 没有交线返回false
        */
        bool Intersect( CVector3f& intersectLine, const CPlane& anotherPlane )
        {
            // 平行
            if( m_Normal == anotherPlane.m_Normal )
                return false;
            intersectLine = m_Normal.Cross( anotherPlane.m_Normal );
            intersectLine.NormalizeNoneZero();
            return true;
        }

        /// 翻转自身
        void Inverse()
        {
            m_Normal = -m_Normal;
            m_Dist = -m_Dist;
        }

		ESide GetSide( const CVector3f &vCenter, const CVector3f& halfSize )
		{
			float dist = DistTo( vCenter );
			float maxAbsDist = Abs( m_Normal.x*halfSize.x ) + 
							   Abs( m_Normal.y*halfSize.y ) + 
							   Abs( m_Normal.z*halfSize.z );

			if( dist < -maxAbsDist )
				return CPlane::eSide_Negtive;

			if( dist > +maxAbsDist )
				return CPlane::eSide_Positive;
			
			return CPlane::eSide_Both;
		}
	};
}

#endif
//EOF
