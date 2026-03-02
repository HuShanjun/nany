//====================================================================
// CBoundingBox.h 
// 定义包围盒以及包围盒操作
// 柯达昭
// 2008-11-20
//                      back  
//            -----------------
//           /			      /|
//          /	top          / |
//         /				/  |
//        |----------------|   |
//   left |			front  | right  
//       y|				   |   |
//        |   /z  	  	   |  /	
//        |  / 	    	   | / 	
//        | /	 	x      |/	 
//         ---------------- 
//              bottom
//====================================================================
#pragma once
#include "GammaCommon/TVector3.h"
#include "GammaCommon/TRect.h"
#include "GammaCommon/CMatrix.h"
#include "GammaCommon/CPlane.h"
#include <iosfwd>
#include <cfloat>

namespace Gamma
{
    enum ECornerIndex
    {
        LeftTopFront,
        LeftTopBack,
        RightTopFront,
        RightTopBack,
        LeftBottomFront,
        LeftBottomBack,
        RightBottomFront,
        RightBottomBack,            

        CornerCount,        
    };

    enum EBoxFace
    {
        eBoxFace_Left,
        eBoxFace_Right,
        eBoxFace_Bottom,
        eBoxFace_Top,
        eBoxFace_Near,
        eBoxFace_Far,

        eBoxFace_Count
    };

    /// AABB
    class CAxisAlignBoudingBox
	{
		bool m_bInfinite;
		CVector3f m_vMax;
		CVector3f m_vMin;
	public:

        CAxisAlignBoudingBox()
            : m_vMin(FLT_MAX, FLT_MAX, FLT_MAX)
            , m_vMax(-FLT_MAX, -FLT_MAX, -FLT_MAX)
			, m_bInfinite(false)
        {}

        CAxisAlignBoudingBox( const CVector3f& vMin, const CVector3f& vMax )
            : m_vMin( vMin ), m_vMax( vMax ), m_bInfinite(false)
        {}
        CAxisAlignBoudingBox( float fXMin, float fYMin, float fZMin, float fXMax, float fYMax, float fZMax )
            : m_vMin( fXMin, fYMin, fZMin ), m_vMax( fXMax, fYMax, fZMax ), m_bInfinite(false)
        {}

		inline void SetExtents( const CVector3f& vMin, const CVector3f& vMax )
		{
			m_vMin = vMin;
			m_vMax = vMax;
			m_bInfinite = false;
		}

		inline void MarkInfinite()
		{
			m_bInfinite = true;
			m_vMax = CVector3f(FLT_MAX, FLT_MAX, FLT_MAX);
			m_vMin = CVector3f(-FLT_MAX, -FLT_MAX, -FLT_MAX);
		}

		inline bool IsInfinite() const
		{
			return m_bInfinite;
		}

		inline CVector3f GetHalfSize(void) const
		{
			return (m_vMax - m_vMin) * 0.5f;
		}

		inline CVector3f GetSize() const
		{
			return m_vMax - m_vMin;
		}

		inline const CVector3f& GetMax() const
		{
			return m_vMax;
		}

		inline const CVector3f& GetMin() const
		{
			return m_vMin;
		}

		inline void SetMax( const CVector3f& v )
		{
			m_bInfinite = false;
			m_vMax = v;
		}

		inline void SetMin( const CVector3f& v )
		{
			m_bInfinite = false;
			m_vMin = v;
		}

        CVector3f GetCenter() const { return ( m_vMin + m_vMax ) * 0.5f; }
        CVector3f GetScale() const { return m_vMax - m_vMin; }

        void Set( const CVector3f& vCenter, const CVector3f& vScale )
		{
			m_bInfinite = false;
            m_vMin = vCenter - vScale / 2;
            m_vMax = vCenter + vScale / 2;
		}

        void Reset()
		{
			m_bInfinite = false;
            m_vMax.x = m_vMax.y = m_vMax.z = -FLT_MAX;
            m_vMin.x = m_vMin.y = m_vMin.z = FLT_MAX;
        }

		void Expand(const CVector3f& size)
		{
			if (!m_bInfinite)
			{
				m_vMin -= size * 0.5f;
				m_vMax += size * 0.5f;
			}
		}

		void Expand(float size)
		{
			Expand(CVector3f(size));
		}

		void Translate(const CVector3f& translation)
		{
			m_vMin += translation;
			m_vMax += translation;
		}

        /// 有交点
        bool Intersect( const CAxisAlignBoudingBox& otherBox ) const
        {
            if ( m_vMax.x < otherBox.m_vMin.x || m_vMin.x > otherBox.m_vMax.x )
                return false;
            if ( m_vMax.y < otherBox.m_vMin.y || m_vMin.y > otherBox.m_vMax.y )
                return false;
            if ( m_vMax.z < otherBox.m_vMin.z || m_vMin.z > otherBox.m_vMax.z )
                return false;

            return true;
		}

		bool Intersect( CVector3f vDir, CVector3f vPos ) const
		{
			CVector3f vPos0 = vPos + vDir*( ( m_vMin.x - vPos.x )/vDir.x );
			if( vPos0.y >= m_vMin.y && vPos0.y < m_vMax.y && vPos0.z >= m_vMin.z && vPos0.z < m_vMax.z )
				return true;
			CVector3f vPos1 = vPos + vDir*( ( m_vMax.x - vPos.x )/vDir.x );
			if( vPos1.y >= m_vMin.y && vPos1.y < m_vMax.y && vPos1.z >= m_vMin.z && vPos1.z < m_vMax.z )
				return true;
			CVector3f vPos2 = vPos + vDir*( ( m_vMin.y - vPos.y )/vDir.y );
			if( vPos2.x >= m_vMin.x && vPos2.x < m_vMax.x && vPos2.z >= m_vMin.z && vPos2.z < m_vMax.z )
				return true;
			CVector3f vPos3 = vPos + vDir*( ( m_vMax.y - vPos.y )/vDir.y );
			if( vPos3.x >= m_vMin.x && vPos3.x < m_vMax.x && vPos3.z >= m_vMin.z && vPos3.z < m_vMax.z )
				return true;
			CVector3f vPos4 = vPos + vDir*( ( m_vMin.z - vPos.z )/vDir.z );
			if( vPos4.x >= m_vMin.x && vPos4.x < m_vMax.x && vPos4.y >= m_vMin.y && vPos4.y < m_vMax.y )
				return true;
			CVector3f vPos5 = vPos + vDir*( ( m_vMax.z - vPos.z )/vDir.z );
			if( vPos5.x >= m_vMin.x && vPos5.x < m_vMax.x && vPos5.y >= m_vMin.y && vPos5.y < m_vMax.y )
				return true;
			return false;
		}

        /// 是否完全包含另一个实例
        bool Contain( const CAxisAlignBoudingBox& otherBox ) const
        {
            return 
				otherBox.m_vMax.x <= m_vMax.x && otherBox.m_vMin.x >= m_vMin.x &&
				otherBox.m_vMax.y <= m_vMax.y && otherBox.m_vMin.y >= m_vMin.y &&
				otherBox.m_vMax.z <= m_vMax.z && otherBox.m_vMin.z >= m_vMin.z;
        }

        bool Contain( const CVector3f& vPoint ) const
        {
            return 
				vPoint.x <= m_vMax.x && vPoint.x >= m_vMin.x &&
				vPoint.y <= m_vMax.y && vPoint.y >= m_vMin.y &&
				vPoint.z <= m_vMax.z && vPoint.z >= m_vMin.z;
        }

        void Merge( const CAxisAlignBoudingBox& box )
        {
			if( m_bInfinite )
				return;
			if( box.m_bInfinite )
				MarkInfinite();
			else
			{
				m_vMin.x = Min( m_vMin.x, box.m_vMin.x );
				m_vMin.y = Min( m_vMin.y, box.m_vMin.y );
				m_vMin.z = Min( m_vMin.z, box.m_vMin.z );
				m_vMax.x = Max( m_vMax.x, box.m_vMax.x );
				m_vMax.y = Max( m_vMax.y, box.m_vMax.y );
				m_vMax.z = Max( m_vMax.z, box.m_vMax.z );
			}
        }

        void Merge( const CVector3f& vPoint )
		{
			if( m_bInfinite )
				return;
            m_vMin.x = Min( m_vMin.x, vPoint.x );
            m_vMin.y = Min( m_vMin.y, vPoint.y );
            m_vMin.z = Min( m_vMin.z, vPoint.z );
            m_vMax.x = Max( m_vMax.x, vPoint.x );
            m_vMax.y = Max( m_vMax.y, vPoint.y );
            m_vMax.z = Max( m_vMax.z, vPoint.z );
		}

		CVector3f GetLeftTopFront() const	
		{ 
			return CVector3f( m_vMin.x,  m_vMax.y, m_vMin.z );
		}

		CVector3f GetLeftTopBack() const	
		{ 
			return CVector3f( m_vMin.x,  m_vMax.y,  m_vMax.z );
		}

		CVector3f GetRightTopFront() const
		{ 
			return CVector3f( m_vMax.x,  m_vMax.y, m_vMin.z );
		}

		CVector3f GetRightTopBack() const	
		{ 
			return CVector3f( m_vMax.x,  m_vMax.y,  m_vMax.z );
		}

		CVector3f GetLeftBottomFront() const
		{ 
			return CVector3f( m_vMin.x, m_vMin.y, m_vMin.z );
		}

		CVector3f GetLeftBottomBack() const
		{ 
			return CVector3f( m_vMin.x, m_vMin.y,  m_vMax.z );
		}

		CVector3f GetRightBottomFront() const
		{ 
			return CVector3f(  m_vMax.x, m_vMin.y, m_vMin.z );
		}

		CVector3f GetRightBottomBack() const
		{ 
			return CVector3f(  m_vMax.x, m_vMin.y,  m_vMax.z );
		}

		CFRect CalBoundingRectInScreen( const CMatrix& matWorldViewProject ) const;
		CAxisAlignBoudingBox operator+( const CVector3f& vPos ) const;
		CAxisAlignBoudingBox operator*( const CVector3f& vScale ) const;
		CAxisAlignBoudingBox operator*( const CMatrix& matTrans ) const;
		CAxisAlignBoudingBox operator&( const CAxisAlignBoudingBox& r ) const;
		CAxisAlignBoudingBox operator|( const CAxisAlignBoudingBox& r ) const;
    };

    /// OBB
    class COrientBoundingBox
    {
        CVector3f m_vCorners[8];
        float m_fRadius;    /// 包围球半径
        CAxisAlignBoudingBox m_AABB;    /// 等价的AABB
        CVector3f m_vCenter;
        CVector3f m_vScale;
        CPlane m_faces[6];  /// 面是朝外的

        void UpdateFaces();
    public:
        COrientBoundingBox() : m_fRadius(0) {}
        COrientBoundingBox( CAxisAlignBoudingBox& box )
        {
            Build( box.GetCenter(), box.GetScale() );
        }

        COrientBoundingBox( const CVector3f& vCenter, const CVector3f& vScale )
        {
            Build( vCenter, vScale );
        }
        
        /**
        @remark 如果pMatWorld不为空，则vCenter和vScale必须为未变换过
        */
        void Build( const CVector3f& vCenter, const CVector3f& vScale, const CMatrix* pMatWorld = NULL );
        const COrientBoundingBox& Transform( const CMatrix& matWorld );

        const CVector3f* GetCorners() const { return m_vCorners; }
        const CVector3f& GetCorner( uint32 nIndex ) const { return m_vCorners[nIndex]; }

        const CAxisAlignBoudingBox& GetAABB() const { return m_AABB; }

        float GetRadius() const { return m_fRadius; }

        const CVector3f& GetCenter() const { return m_vCenter; }
        const CVector3f& GetScale() const { return m_vScale; }

        const CPlane& GetFace( uint32 nFaceIndex ) const { return m_faces[nFaceIndex]; }
        bool Contain( const CVector3f& vPoint ) const;
        bool Intersect( const CVector3f& vStart, const CVector3f& vEnd, std::vector<CVector3f>* pvIntersects = NULL ) const;

        friend std::ostream& operator << ( std::ostream& os, const COrientBoundingBox& box );
    };
}
