#include "GammaCommon/CBoundingBox.h"
#include <cfloat>
#include <iostream>

namespace Gamma
{
	CFRect CAxisAlignBoudingBox::CalBoundingRectInScreen( const CMatrix& matWorldViewProject ) const
	{
		CVector3f vPos[8] =
		{
			GetLeftTopFront(),
			GetLeftTopBack(),
			GetRightTopFront(),
			GetRightTopBack(),
			GetLeftBottomFront(),
			GetLeftBottomBack(),
			GetRightBottomFront(),
			GetRightBottomBack()
		};

		CVector2f vMin = CVector2f(  FLT_MAX,  FLT_MAX );
		CVector2f vMax = CVector2f( -FLT_MAX, -FLT_MAX );
		for ( size_t i = 0; i < 8; i++ )
		{
			CVector3f vPosInScreen = vPos[i]*matWorldViewProject;
			vMin.x = Min( vMin.x, vPosInScreen.x );
			vMin.y = Min( vMin.y, vPosInScreen.y );
			vMax.x = Max( vMax.x, vPosInScreen.x );
			vMax.y = Max( vMax.y, vPosInScreen.y );
		}

		return CFRect( vMin.x, vMin.y, vMax.x, vMax.y );
	}

	CAxisAlignBoudingBox CAxisAlignBoudingBox::operator+( const CVector3f& vPos ) const
	{
		if( m_bInfinite )
			return *this;
		return CAxisAlignBoudingBox( m_vMin + vPos, m_vMax + vPos );
	}

	CAxisAlignBoudingBox CAxisAlignBoudingBox::operator*( const CVector3f& vScale ) const
	{
		if( m_bInfinite )
			return *this;

		CVector3f vMin = m_vMin;
		CVector3f vMax = m_vMax;
		vMin.x = vMin.x*vScale.x;
		vMin.y = vMin.y*vScale.y;
		vMin.z = vMin.z*vScale.z;
		vMax.x = vMax.x*vScale.x;
		vMax.y = vMax.y*vScale.y;
		vMax.z = vMax.z*vScale.z;
		return CAxisAlignBoudingBox( vMin, vMax );
	}

	CAxisAlignBoudingBox CAxisAlignBoudingBox::operator*( const CMatrix& matTrans ) const
	{
		if( m_bInfinite )
			return *this;

		CVector3f vPos[8] =
		{
			GetLeftTopFront(),
			GetLeftTopBack(),
			GetRightTopFront(),
			GetRightTopBack(),
			GetLeftBottomFront(),
			GetLeftBottomBack(),
			GetRightBottomFront(),
			GetRightBottomBack()
		};

		CVector3f vMin = CVector3f(  FLT_MAX,  FLT_MAX,  FLT_MAX );
		CVector3f vMax = CVector3f( -FLT_MAX, -FLT_MAX, -FLT_MAX );
		for ( size_t i = 0; i < 8; i++ )
		{
			CVector3f vPosInScreen = vPos[i]*matTrans;
			vMin.x = Min( vMin.x, vPosInScreen.x );
			vMin.y = Min( vMin.y, vPosInScreen.y );
			vMin.z = Min( vMin.z, vPosInScreen.z );
			vMax.x = Max( vMax.x, vPosInScreen.x );
			vMax.y = Max( vMax.y, vPosInScreen.y );
			vMax.z = Max( vMax.z, vPosInScreen.z );
		}

		return CAxisAlignBoudingBox( vMin, vMax );
	}

	CAxisAlignBoudingBox CAxisAlignBoudingBox::operator&( const CAxisAlignBoudingBox& r ) const
	{
		return CAxisAlignBoudingBox(
			Max<float>( m_vMin.x, r.m_vMin.x ),
			Max<float>( m_vMin.y, r.m_vMin.y ),
			Max<float>( m_vMin.z, r.m_vMin.z ),
			Min<float>( m_vMax.x, r.m_vMax.x ),
			Min<float>( m_vMax.y, r.m_vMax.y ),
			Min<float>( m_vMax.z, r.m_vMax.z ) );
	}

	CAxisAlignBoudingBox CAxisAlignBoudingBox::operator|( const CAxisAlignBoudingBox& r ) const
	{
		CAxisAlignBoudingBox merge = *this;
		merge.Merge( r );
		return merge;
	}

    //////////////////////////////////////////////////////////////////////////
    const COrientBoundingBox& COrientBoundingBox::Transform( const CMatrix& matWorld )
	{
		CVector3f vMin = CVector3f(  FLT_MAX,  FLT_MAX,  FLT_MAX );
		CVector3f vMax = CVector3f( -FLT_MAX, -FLT_MAX, -FLT_MAX );
        for ( size_t i = 0; i < CornerCount; i++ )
        {
            m_vCorners[i] *= matWorld;
            const CVector3f& vCorner = m_vCorners[i];
            vMin.x = Min( vMin.x, vCorner.x );
            vMin.y = Min( vMin.y, vCorner.y );
            vMin.z = Min( vMin.z, vCorner.z );
            vMax.x = Max( vMax.x, vCorner.x );
            vMax.y = Max( vMax.y, vCorner.y );
            vMax.z = Max( vMax.z, vCorner.z );
        }

		m_AABB.SetExtents( vMin, vMax );
        m_vCenter = (m_vCorners[LeftBottomFront] + m_vCorners[RightTopBack]) / 2;
        m_vScale = m_vCorners[RightTopBack] - m_vCorners[LeftBottomFront];
        m_fRadius = m_vScale.Len() / 2;
        UpdateFaces();

        return *this;
    }

    void COrientBoundingBox::Build( const CVector3f& vCenter, const CVector3f& vScale, const CMatrix* pMatWorld /*= NULL*/ )
    {        
        m_vCorners[LeftTopFront] = vCenter + CVector3f( -vScale.x,  vScale.y, -vScale.z )/2.0f;
        m_vCorners[LeftTopBack] = vCenter + CVector3f( -vScale.x,  vScale.y,  vScale.z )/2.0f;
        m_vCorners[RightTopFront] = vCenter + CVector3f( vScale.x,  vScale.y, -vScale.z  )/2.0f;
        m_vCorners[RightTopBack] = vCenter + CVector3f(   vScale.x,  vScale.y,  vScale.z  )/2.0f;
        m_vCorners[LeftBottomFront] = vCenter + CVector3f( -vScale.x, -vScale.y, -vScale.z )/2.0f;
        m_vCorners[LeftBottomBack] = vCenter + CVector3f( -vScale.x, -vScale.y,  vScale.z )/2.0f;
        m_vCorners[RightBottomFront] = vCenter + CVector3f( vScale.x, -vScale.y, -vScale.z )/2.0f;
        m_vCorners[RightBottomBack] = vCenter + CVector3f( vScale.x, -vScale.y,  vScale.z )/2.0f;

		if( pMatWorld )
		{
			CVector3f vMin = CVector3f(  FLT_MAX,  FLT_MAX,  FLT_MAX );
			CVector3f vMax = CVector3f( -FLT_MAX, -FLT_MAX, -FLT_MAX );
			for( size_t i = 0; i < CornerCount; i++ )
			{
				m_vCorners[i] *= *pMatWorld;
				const CVector3f& vCorner = m_vCorners[i];
				vMin.x = Min( vMin.x, vCorner.x );
				vMin.y = Min( vMin.y, vCorner.y );
				vMin.z = Min( vMin.z, vCorner.z );
				vMax.x = Max( vMax.x, vCorner.x );
				vMax.y = Max( vMax.y, vCorner.y );
				vMax.z = Max( vMax.z, vCorner.z );
			}
			m_AABB.SetExtents( vMin, vMax );
		}
		else
		{
			m_AABB.Set( vCenter, vScale );
		}

        m_vCenter = (m_vCorners[LeftBottomFront] + m_vCorners[RightTopBack]) / 2;
        m_vScale = m_vCorners[RightTopBack] - m_vCorners[LeftBottomFront];
        m_fRadius = m_vScale.Len() / 2;

        UpdateFaces();
    }

    void COrientBoundingBox::UpdateFaces()
    {
        // 三个点索引和下个平行面的距离在Scale的索引
        const static uint8_t FaceCornerIndex[eBoxFace_Count/2][4] = {
            { LeftBottomBack, LeftTopBack, LeftTopFront, 0 }, // left
            { LeftTopBack, RightTopBack, RightTopFront, 1 }, // top
            { LeftTopFront, RightTopFront, RightBottomFront, 2}, // near
        };

        for ( uint8_t i = 0; i < eBoxFace_Count/2; i++ )
        {
            uint8_t nCorner1 = FaceCornerIndex[i][0];
            uint8_t nCorner2 = FaceCornerIndex[i][1];
            uint8_t nCorner3 = FaceCornerIndex[i][2];
            m_faces[i*2].Init( m_vCorners[ nCorner1 ], m_vCorners[ nCorner2 ], m_vCorners[ nCorner3 ] );
            m_faces[i*2+1].m_Normal = -m_faces[i*2].m_Normal;
            m_faces[i*2+1].m_Dist = -m_vScale[ FaceCornerIndex[i][3] ] - m_faces[i*2].m_Dist ;
        }
    }

    bool COrientBoundingBox::Contain( const CVector3f& vPoint ) const
    {
        
        for ( uint32_t i = 0; i < eBoxFace_Count; i++ )
        {
            if ( m_faces[i].DistTo( vPoint ) > 0 )
                return false;
        }

        return true;
    }

    bool COrientBoundingBox::Intersect( const CVector3f& vStart, const CVector3f& vEnd, std::vector<CVector3f>* pvIntersects /*= NULL*/ ) const
    {
        static const uint32_t nParallelFaceIndex[eBoxFace_Count] = { 1, 0, 3, 2, 5, 4 };
        bool bSkipFace[eBoxFace_Count] = {false};
        bool bIntersect = false;
        for ( uint32_t i = 0; i < eBoxFace_Count; i++ )
        {
            if ( bSkipFace[i] )
                continue;

            CVector3f vIntersect;
            ELineIntersectType eLineIntersectType = m_faces[i].SplitSeg( vIntersect, vStart, vEnd );
            if ( eLineParallelOutside == eLineIntersectType )
                return false;

            if ( eLineIntersectInside == eLineIntersectType )
            {
                if ( Contain(vIntersect) )
                {
                    if ( pvIntersects )
                        pvIntersects->push_back( vIntersect );
                    bIntersect = true;
                }
            }
            else if ( eLineOnPlane == eLineIntersectType )
            {
                bSkipFace[ nParallelFaceIndex[i] ] = true;
                if ( pvIntersects )
                {
                    pvIntersects->push_back( vStart );
                    pvIntersects->push_back( vEnd );
                }

                bIntersect = true;
            }

            // 早点退出
            if ( !pvIntersects && bIntersect )
                return true;
        }

        return bIntersect;
    }

    std::ostream& operator << ( std::ostream& os, const COrientBoundingBox& box )
    {
        return os << "AABB: vMax= " << box.m_AABB.GetMax().x << " " << box.m_AABB.GetMax().y << " " << box.m_AABB.GetMax().z <<
            " vMin= " << box.m_AABB.GetMin().x << " " << box.m_AABB.GetMin().y << " " << box.m_AABB.GetMin().z << std::endl;
    }
}
