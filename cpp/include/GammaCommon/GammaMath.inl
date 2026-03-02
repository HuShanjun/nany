//===============================================
// GammaMath.inl
// 定义常用数学函数inline实现
// 柯达昭
// 2008-01-04
//===============================================
#include "GammaMath.h"

namespace Gamma
{
	class GAMMA_COMMON_API CTrigFunctionTable
	{
		CTrigFunctionTable();
	public:
		// 0~1.57( 0~90度 )
		enum
		{ 
			eBitCount	= 16,
			eCircle		= 1 << ( eBitCount + 2 ),
			eCircleMask	= eCircle - 1,
			eTableSize	= 1 << eBitCount, 
			eTableMask	= eTableSize - 1, 
			eSignedMask = 1 << ( eBitCount + 1 ),
			eInvMask	= eTableSize
		};

		static const CTrigFunctionTable& Instance();
		float	m_fSinBuf[eTableSize];
		float	m_fASinBuf[eTableSize];

		inline float Sin( float f ) const
		{
			int32_t nIndex = ( (int32_t)( ( f/GM_2PI )*eCircle ) )&eCircleMask;
			float fResult = nIndex&eInvMask ? m_fSinBuf[ eTableSize - ( nIndex&eTableMask ) ] : m_fSinBuf[ nIndex&eTableMask ];
			return nIndex&eSignedMask ? -fResult : fResult;
		}

		inline float ASin( float f ) const
		{
			if( f >= 0 )
				return m_fASinBuf[( (int32_t)( Limit( f, 0.0f, 1.0f )*eTableSize ) )&eTableMask];
			else
				return -m_fASinBuf[( (int32_t)( Limit( -f, 0.0f, 1.0f )*eTableSize ) )&eTableMask];
		}

		inline float Cos( float f ) const	{ return Sin( GM_PI_2 - f ); }
		inline float ACos( float f ) const	{ return GM_PI_2 - ASin( f );}
	};

	inline float GammaSin( float f )	{ return CTrigFunctionTable::Instance().Sin( f ); }
	inline float GammaCos( float f )	{ return CTrigFunctionTable::Instance().Cos( f ); }
	inline float GammaASin( float f )	{ return CTrigFunctionTable::Instance().ASin( f ); }
	inline float GammaACos( float f )	{ return CTrigFunctionTable::Instance().ACos( f ); }
	template<class T>
	inline T GammaAbs( T t )			{ return t > 0 ? t : -t; }
	template<class T>
	inline T GammaSigned( T t )			{ return (T)( t > 0 ? 1 : ( t < 0 ? -1 : 0 ) );}
    //================================================================================================================
    //! \fn     T TriInterpolate( const T& T0, const T& T1, const T& T2, const T& T3, float x, float y )
    //  \brief  在T0~T3之间进行三角形插值
    //
    //  \param  T& T0, T& T1, T& T2, T& T3        插值参数        T1 _______________________ T3 ( x = 1.0f, y = 1.0f )
    //            float x, float y                插值位置        |\                         |
    //                                                            |     \                    |
    //                                                            |          \               |
    //                                                            |               \          |
    //                                                            |( x = 0, y = 0 )   \      |
    //                                                            T0 ______________________\|T2
    //                                                            
    //
    //  \return T                                插值结果
    //=================================================================================================================
    template< class T >
    inline T TriInterpolate( const T& T0, const T& T1, const T& T2, const T& T3, float x, float y )
    {
        if( y > 1.0f - x )
        {
            x -= 1.0f;
            y -= 1.0f;
            return (T)( ( T3 - T1 )*x + ( T3 - T2 )*y + T3 );
        }
        else
        {
            return (T)( ( T2 - T0 )*x + ( T1 - T0 )*y + T0 );
        }
	}

	//==================================================================================================================
	//! \fn		T BezierInterpolate( const T& T0, const T& T1, const T& T2, const T& T3, float p );
	//  \brief	贝塞尔曲线插值
	//
	//  \param	T0 起点， T3终点， T1是T0的控制点，T2是T3的控制点，p的值由0〜1
	//  \return T 插值结果
	//==================================================================================================================
	template<class T>
	inline T BezierInterpolate( const T& T0, const T& T1, const T& T2, const T& T3, float p1 )
	{
		float p2  = 1.0f - p1;
		float p12 = p1*p1;
		float p22 = p2*p2;
		return T0*( p22*p2 ) + T1*( 3.0f*p22*p1 ) + T2*( 3.0f*p12*p2 ) + T3*( p12*p1 );
	}

    /** 厄密特曲线插值，效果和贝塞尔等价，控制方式不同
    @param p0 起始点
    @param p1 终止点
    @param t0 起始点处的切线
    @param t1 终止点处的切线
    @param u 比例，0~1
    */
    template<class T>
    inline T HermiteInterpolate( const T& p0, const T& p1, const T& t0, const T& t1, float u )
    {
        float u2 = u * u;
        float u3 = u2 * u;

        float h0 = 2.0f * u3 - 3.0f * u2 + 1.0f;
        float h1 = u3 - 2.0f * u2 + u;
        float h2 = u3 - u2;
        float h3 = -2.0f * u3 + 3.0f * u2;

        return h0 * p0 + h1 * t0 + h2 * p1 + h3 * t1;
    }

    /** Catmull-Rom 曲线插值
    @param v0 前一个点
    @param v1 当前点
    @param v2 下一个点
    @param v3 下2个点
    @param a v1到v2间的比重，0~1
    */
    template<class T>
    inline T CatmullRomSpline( const T& v0, const T& v1, const T& v2, const T& v3, float a, T* pOutCoeffs /*= NULL*/ )
    {        
        T c2 = -0.5f * v0 + 0.5f * v2;
        T c3 = v0 - 2.5f * v1 + 2.0f * v2 - 0.5f * v3;
        T c4 = -0.5f * v0 + 1.5f * v1 - 1.5f * v2 + 0.5f * v3;

        if ( pOutCoeffs )
        {
            pOutCoeffs[0] = v1;
            pOutCoeffs[1] = c2;
            pOutCoeffs[2] = c3;
            pOutCoeffs[3] = c4;
        }

        return ( ( c4 * a + c3 ) * a + c2 ) * a + v1;
    }

//     /** float 的特化版本，避免基础类型传引用 */
//     template<>
//     inline float CatmullRomSpline<float>( float v0, float v1, float v2, float v3, float a )
//     {
//         float c2 = -0.5f * v0 + 0.5f * v2;
//         float c3 = v0 - 2.5f * v1 + 2.0f * v2 - 0.5f * v3;
//         float c4 = -0.5f * v0 + 1.5f * v1 - 1.5f * v2 + 0.5f * v3;
// 
//         return ( ( c4 * a + c3 ) * a + c2 ) * a + v1;
	//     }


    template< class M, class V >
    inline void CreateShadowMatrixOnPlane( M& mat, const V& vecLightDir, 
        const V& vecPlaneNormal, const V& posPointOfPlane )
    {
        float fDiv = 1.0f/( vecPlaneNormal.Dot( vecLightDir ) );
        float fDot = vecPlaneNormal.Dot( posPointOfPlane );
        mat._11 = (  vecPlaneNormal.y*vecLightDir.y + vecPlaneNormal.z*vecLightDir.z )*fDiv;
        mat._12 = ( -vecPlaneNormal.x*vecLightDir.y )*fDiv;
        mat._13 = ( -vecPlaneNormal.x*vecLightDir.z )*fDiv;
        mat._14 = 0;
        mat._21 = ( -vecPlaneNormal.y*vecLightDir.x )*fDiv;
        mat._22 = (  vecPlaneNormal.x*vecLightDir.x + vecPlaneNormal.z*vecLightDir.z )*fDiv;
        mat._23 = ( -vecPlaneNormal.y*vecLightDir.z )*fDiv;
        mat._24 = 0;
        mat._31 = ( -vecPlaneNormal.z*vecLightDir.x )*fDiv;
        mat._32 = ( -vecPlaneNormal.z*vecLightDir.y )*fDiv;
        mat._33 = (  vecPlaneNormal.x*vecLightDir.x + vecPlaneNormal.y*vecLightDir.y )*fDiv;
        mat._34 = 0;
        mat._41 = ( fDot*vecLightDir.x )*fDiv;
        mat._42 = ( fDot*vecLightDir.y )*fDiv;
        mat._43 = ( fDot*vecLightDir.z )*fDiv;
        mat._44 = 1;
    }

    template<typename ValueType, class VecType>
    inline bool CutLine( const ValueType& xMin, const ValueType& xMax, 
		const ValueType& xStart, const ValueType& xEnd, VecType& vStart, const VecType& vEnd )
    {
        ValueType xTempStart = Limit<ValueType>( xStart, xMin, xMax );
        if( xTempStart != xStart )
        {
			ValueType nOrgLen = GammaAbs( xEnd - xStart );
            if( nOrgLen < GammaAbs( xTempStart - xStart ) )
				return false;
			if( nOrgLen < GammaAbs( xEnd - xTempStart ) )
				return false;
            vStart = ( vEnd - vStart )*( xTempStart - xStart )/( xEnd - xStart ) + vStart;
        }
        return true;
    }

    template<class _Rect, class _VECTOR>
    inline bool Cut2DLine( const _Rect& rt, _VECTOR& posStart, _VECTOR& posEnd )
    {
        return
            CutLine( rt.left, rt.right, posStart.x, posEnd.x, posStart, posEnd ) &&
            CutLine( rt.top,  rt.bottom, posStart.y, posEnd.y, posStart, posEnd ) &&
            CutLine( rt.left, rt.right, posEnd.x, posStart.x, posEnd, posStart ) &&
            CutLine( rt.top,  rt.bottom, posEnd.y, posStart.y, posEnd, posStart ) ;
    }

	template<class _VECTOR>
	inline bool Cut3DLine( const _VECTOR& vMin, const _VECTOR& vMax, _VECTOR& vStart, _VECTOR& vEnd )
	{
		if( !CutLine( vMin.x, vMax.x, vStart.x, vEnd.x, vStart, vEnd ) )
			return false;
		if( !CutLine( vMin.x, vMax.x, vEnd.x, vStart.x, vEnd, vStart ) )
			return false;
		if( !CutLine( vMin.y, vMax.y, vStart.y, vEnd.y, vStart, vEnd ) )
			return false;
		if( !CutLine( vMin.y, vMax.y, vEnd.y, vStart.y, vEnd, vStart ) )
			return false;
		if( !CutLine( vMin.z, vMax.z, vStart.z, vEnd.z, vStart, vEnd ) )
			return false;
		if( !CutLine( vMin.z, vMax.z, vEnd.z, vStart.z, vEnd, vStart ) )
			return false;
		return true;
	}

	template<typename ILineAction>
	inline bool TightLineTo( int32_t xSrc, int32_t ySrc, int32_t xDes, int32_t yDes, ILineAction& Action )
	{
		int32_t xRange = xDes < xSrc ? xSrc-xDes : xDes-xSrc;		//instead of abs()
		int32_t yRange = yDes < ySrc ? ySrc-yDes : yDes-ySrc;		//abs

		int32_t xDelta = xDes < xSrc ? -1 : 1;
		int32_t yDelta = yDes < ySrc ? -1 : 1;


		if( !Action.Do( xSrc, ySrc ) )
			return false;

		int32_t nCurDx = 0;
		int32_t nCurDy = 0;
		int32_t nJudge = 0;

		if( yRange > xRange )
		{
			nCurDy = 1;
			//nJudge = -( max( yRange, 2 )>>1 );
			nJudge = -( yRange>2? yRange>>1 : 2>>1 );
		}
		else
		{
			nCurDx = 1;
			nJudge = ( xRange>>1 );
		}
		int32_t nCurx = xSrc;
		int32_t nCury = ySrc;
	
		while( (nCurx != xDes ) || ( nCury != yDes ) )
		{
			if( nJudge < 0 )
			{
				nCury = nCury + yDelta;
				nCurDy++;
				nJudge += xRange;
			}
			else
			{
				nCurx = nCurx + xDelta;
				nCurDx++;
				nJudge -= yRange;
			}

			if( !Action.Do( nCurx, nCury ) )
				return false;
		}
		return true;
	}


	template<typename ILineAction>
	inline bool LineTo( int32_t xSrc, int32_t ySrc, int32_t xDes, int32_t yDes, ILineAction Action, bool bCheckEnd /*= false*/ )
	{
		int32_t xRange1 = xDes < xSrc ? xSrc - xDes : xDes - xSrc;
		int32_t yRange1 = yDes < ySrc ? ySrc - yDes : yDes - ySrc;
		int32_t xRange2 = xRange1 << 1;
		int32_t yRange2 = yRange1 << 1;

		int32_t xDelta = xDes < xSrc ? -1 : 1;
		int32_t yDelta = yDes < ySrc ? -1 : 1;

		if( !Action( xSrc, ySrc ) )
			return false;

		int nJudge = 0;
		
		int32_t nCurx = xSrc;
		int32_t nCury = ySrc;
		
		if( yRange1 > xRange1 )
		{
			while( nCury != yDes )
			{
				if( nJudge - xRange2 < -yRange1 )
				{
					nCurx += xDelta;
					nJudge += yRange2;
				}

				nCury += yDelta;
				nJudge -= xRange2;

				if( !Action( nCurx, nCury ) )
					return false;
			}
		}
		else if( yRange1 < xRange1 )
		{
			while( nCurx != xDes )
			{
				if( nJudge - yRange2 < -xRange1 )
				{
					nCury += yDelta;
					nJudge += xRange2;
				}

				nCurx += xDelta;
				nJudge -= yRange2;

				if( !Action( nCurx, nCury ) )
					return false;
			}
		}
		else
		{
			while( nCurx != xDes )
			{
				nCury += yDelta;
				nCurx += xDelta;
				if( !Action( nCurx, nCury ) )
					return false;
			}
		}

		return bCheckEnd ? Action( xDes, yDes ) : true;
	}

	//=============================================================================================================================
	// 生成球体
	// nHalfCircleDivCount为纵向分割的块数
	// 返回顶点数量以及索引数量
	//=============================================================================================================================
	template< typename _VertexVec>
	inline std::pair<uint32_t,uint32_t> BuildSphere( _VertexVec* pVertex, uint16_t* pIndex, 
		int32_t nCircleDivCount, int32_t nHeightDivCount, float fRadiusY, float fUpRadiusXZ, float fDownRadiusXZ, bool bTop, bool bBottom, 
		float fUSectorStart, float fUSectorEnd, float fVSegmentStart, float fVSegmentEnd, bool bWrapUV,
		float fSectorStart, float fSectorEnd, float fSegmentStart, float fSegmentEnd, uint32_t nVetexContext )
	{	
		fSectorStart		= Limit( fSectorStart, 0.0f, 1.0f );
		fSectorEnd			= Limit( fSectorEnd, 0.0f, 1.0f );
		fSegmentStart		= Limit( fSegmentStart, 0.0f, 1.0f );
		fSegmentEnd			= Limit( fSegmentEnd, 0.0f, 1.0f );

		int32_t nYSegStart	= (int32_t)floor( fSegmentStart*nHeightDivCount );
		int32_t nYSegEnd		= (int32_t)ceil( fSegmentEnd*nHeightDivCount );
		int32_t nYSecStart	= (int32_t)floor( fSectorStart*nCircleDivCount );
		int32_t nYSecEnd		= (int32_t)ceil( fSectorEnd*nCircleDivCount );
		int32_t nLineSector	= nYSecEnd - nYSecStart;
		float fMinPitch		= ( 1 - fSegmentEnd*2.0f );
		float fMaxPitch		= ( 1 - fSegmentStart*2.0f );
		float fMinYaw		= fSectorStart*GM_2PI;
		float fMaxYaw		= fSectorEnd*GM_2PI;

		if( pVertex == NULL || pIndex == NULL )
		{
			int32_t nVertexCount = ( nLineSector + 1 )*( nYSegEnd - nYSegStart + 1 );
			int32_t nIndexCount = nLineSector*( nYSegEnd - nYSegStart )*6;

			if( fSegmentStart == 0.0f && fDownRadiusXZ < 0 )
			{
				nVertexCount -= nLineSector;
				nIndexCount  -= nLineSector*3;
			}
			else if( bTop )
			{
				nVertexCount++;
				nIndexCount += nLineSector*3;
			}

			if( fSegmentEnd == 1.0f && fDownRadiusXZ < 0 )
			{
				nVertexCount -= nLineSector;
				nIndexCount  -= nLineSector*3;
			}
			else if( bBottom )
			{
				nVertexCount++;
				nIndexCount += nLineSector*3;
			}

			return std::pair<uint32_t,uint32_t>( Max( nVertexCount, 0 ), Max( nIndexCount, 0 ) );
		}

		uint32_t nCurVertexCount = 0;
		uint32_t nCurIndexCount = 0;
		if( bTop && ( fSegmentStart != 0.0f || fDownRadiusXZ >= 0 ) )
		{
			if( fDownRadiusXZ < 0 )
				pVertex[nCurVertexCount++] = _VertexVec( 0, sinf( fMaxPitch*GM_PI_2 )*fRadiusY, 0, 0.5f, bWrapUV ? 0.0f : 0.5f, nVetexContext );
			else
				pVertex[nCurVertexCount++] = _VertexVec( 0, fMaxPitch*fRadiusY, 0, 0.5f, bWrapUV ? 0.0f : 0.5f, nVetexContext );

			for( int32_t i = 0; i < nLineSector; i++ )
			{
				pIndex[nCurIndexCount++] = 0;
				pIndex[nCurIndexCount++] = (uint16_t)( i + 2 );
				pIndex[nCurIndexCount++] = (uint16_t)( i + 1 );
			}
		}

		///< Y方向
		for( int32_t i = nYSegStart; i <= nYSegEnd; i++ )
		{
			if( i == 0 && fSegmentStart == 0.0f && fDownRadiusXZ < 0 )
			{
				pVertex[nCurVertexCount++] = _VertexVec( 0, fRadiusY, 0, 0.5f, bWrapUV ? 0.0f : 0.5f, nVetexContext );
			}
			else if( i == nHeightDivCount && fSegmentEnd == 1.0f && fDownRadiusXZ < 0 )
			{
				int32_t nCurVerCount = nCurVertexCount;
				int32_t nLineStart = nCurVerCount - nLineSector - 1;
				pVertex[nCurVertexCount++] = _VertexVec( 0, -fRadiusY, 0, 0.5f, bWrapUV ? 1.0f : 0.5f, nVetexContext );

				for( int32_t i = 0; i < nLineSector; i++ )
				{
					pIndex[nCurIndexCount++] = (uint16_t)( nCurVertexCount - 1 );
					pIndex[nCurIndexCount++] = (uint16_t)( i + nLineStart );
					pIndex[nCurIndexCount++] = (uint16_t)( i + 1 + nLineStart );
				}
			}
			else
			{
				float fCurSegment = Limit( i/(float)nHeightDivCount, fSegmentStart, fSegmentEnd );
				float fPitch = Limit( ( nHeightDivCount - i*2.0f )/nHeightDivCount, fMinPitch, fMaxPitch );
				float fRadiusXZ;
				float fHeightY;
				if( fDownRadiusXZ < 0 )
				{
					fRadiusXZ = cosf( fPitch*GM_PI_2 )*fUpRadiusXZ;
					fHeightY = sinf( fPitch*GM_PI_2 )*fRadiusY;
				}
				else
				{
					fRadiusXZ = fUpRadiusXZ + ( fDownRadiusXZ - fUpRadiusXZ )*fCurSegment;
					fHeightY = fPitch*fRadiusY;
				}

				///< XZ方向
				float v = ( fCurSegment - fVSegmentStart )/( fVSegmentEnd - fVSegmentStart );
				if( !bWrapUV )
					v = 0.5f - Abs<float>( v - 0.5f );

				for( int32_t j = nYSecStart; j <= nYSecEnd; j++ )
				{
					float fCurSector = Limit( j/(float)nCircleDivCount, fSectorStart, fSectorEnd );
					float fXZAngle = Limit( j*GM_2PI/nCircleDivCount, fMinYaw, fMaxYaw );
					float x = cosf( fXZAngle );
					float y = fHeightY;
					float z = sinf( fXZAngle );
					if( !bWrapUV )
						pVertex[nCurVertexCount++] = _VertexVec( x*fRadiusXZ, y, z*fRadiusXZ, 
						v*x + 0.5f, v*z + 0.5f, nVetexContext );
					else
						pVertex[nCurVertexCount++] = _VertexVec( x*fRadiusXZ, y, z*fRadiusXZ, 
						( fCurSector - fUSectorStart )/( fUSectorEnd - fUSectorStart ), v, nVetexContext );
				}

				if( i == nYSegStart )
					continue;

				if( i == 1 && fSegmentStart == 0.0f && fDownRadiusXZ < 0 )
				{
					int32_t nCurVerCount = nCurVertexCount;
					int32_t nLine2Start = nCurVerCount - nLineSector - 1;
					for( int32_t i = 0; i < nLineSector; i++ )
					{
						pIndex[nCurIndexCount++] = 0;
						pIndex[nCurIndexCount++] = (uint16_t)( nLine2Start + i + 1 );
						pIndex[nCurIndexCount++] = (uint16_t)( nLine2Start + i );
					}
				}
				else
				{
					int32_t nCurVerCount = nCurVertexCount;
					int32_t nLine2Start = nCurVerCount - nLineSector - 1;
					int32_t nLine1Start = nLine2Start  - nLineSector - 1;
					for( int32_t i = 0; i < nLineSector; i++ )
					{
						uint16_t nVer11 = (uint16_t)( nLine1Start + i );
						uint16_t nVer21 = (uint16_t)( nLine2Start + i );
						uint16_t nVer12 = (uint16_t)( nLine1Start + i + 1 );
						uint16_t nVer22 = (uint16_t)( nLine2Start + i + 1 );

						pIndex[nCurIndexCount++] = nVer21;
						pIndex[nCurIndexCount++] = nVer11;
						pIndex[nCurIndexCount++] = nVer22;
						pIndex[nCurIndexCount++] = nVer22;
						pIndex[nCurIndexCount++] = nVer11;
						pIndex[nCurIndexCount++] = nVer12;
					}
				}
			}
		}

		if( bBottom && ( fSegmentEnd != 1.0f || fDownRadiusXZ >= 0 ) )
		{
			uint16_t nVertStart = (uint16_t)( nCurVertexCount - nLineSector - 1 );
			if( fDownRadiusXZ < 0 )
				pVertex[nCurVertexCount++] = _VertexVec( 0, sinf( fMinPitch*GM_PI_2 )*fRadiusY, 0, 0.5f, bWrapUV ? 1.0f : 0.5f, nVetexContext );
			else
				pVertex[nCurVertexCount++] = _VertexVec( 0, fMinPitch*fRadiusY, 0, 0.5f, bWrapUV ? 1.0f : 0.5f, nVetexContext );

			for( int32_t i = 0; i < nLineSector; i++ )
			{
				pIndex[nCurIndexCount++] = (uint16_t)( nCurVertexCount - 1 );
				pIndex[nCurIndexCount++] = (uint16_t)( nVertStart + i + 0 );
				pIndex[nCurIndexCount++] = (uint16_t)( nVertStart + i + 1 );
			}
		}

		return std::pair<uint32_t,uint32_t>( nCurVertexCount, nCurIndexCount );
	}

	//=============================================================================================================================
	// 线段(a,b)上离某点(c)的最近的点d
	//=============================================================================================================================
	template< typename Vec >
	inline float ClosetPointSegment( const Vec& a, const Vec& b, const Vec& c, Vec* d )
	{
		Vec _ab = b - a;
		Vec _ac = c - a;
		float l = _ab.Dot( _ab );
		float t = l == 0 ? 0 : _ac.Dot( _ab )/l;
		if( d )
			*d = _ab * ( t < 0 ? 0 : ( t > 1 ? 1 : t ) ) + a;
		return t;
	}

	//=============================================================================================================================
	// 线段(a1,a2)是否和线段(b1,b2)相交
	//=============================================================================================================================
	template<class T>
	inline bool Intersect2DLine( const TVector2<T>& a, const TVector2<T>& b, const TVector2<T>& c, const TVector2<T>& d )  
	{  
		// 三角形abc 面积的2倍  
		T area_abc = ( a.x - c.x ) * ( b.y - c.y ) - ( a.y - c.y ) * ( b.x - c.x );  

		// 三角形abd 面积的2倍  
		T area_abd = ( a.x - d.x ) * ( b.y - d.y ) - ( a.y - d.y) * ( b.x - d.x );   

		// 面积符号相同则两点在线段同侧,不相交 (对点在线段上的情况,本例当作不相交处理);  
		if ( area_abc*area_abd >= 0 )
			return false; 

		// 三角形cda 面积的2倍  
		T area_cda = (c.x - a.x) * (d.y - a.y) - (c.y - a.y) * (d.x - a.x);  
		// 三角形cdb 面积的2倍  
		// 注意: 这里有一个小优化.不需要再用公式计算面积,而是通过已知的三个面积加减得出.  
		T area_cdb = area_cda + area_abc - area_abd ;  
		if( area_cda * area_cdb >= 0 )
			return false;

		return true;
	}

	//=============================================================================================================================
	// 矩形拉伸填充
	// 外部保证参数合法，不越界
	//=============================================================================================================================
	template<typename _Elem, typename _IntType>
	inline void RectStretch( _Elem* pDes, _IntType nWidth, _IntType nHeight, const TRect<_IntType>& rtDes, 
		const _Elem* pSrc, _IntType nSrcWidth, _IntType nSrcHeight, const TRect<_IntType>& rtSrc )
	{
		GammaAst( rtSrc.left >= 0 );
		GammaAst( rtSrc.left < rtSrc.right );
		GammaAst( rtSrc.right >= 0 );
		GammaAst( rtSrc.right <= nSrcWidth );
		GammaAst( rtSrc.top >= 0 );
		GammaAst( rtSrc.top < rtSrc.bottom );
		GammaAst( rtSrc.bottom >= 0 );
		GammaAst( rtSrc.bottom <= nSrcHeight );

		GammaAst( rtDes.left >= 0 );
		GammaAst( rtDes.left < rtDes.right );
		GammaAst( rtDes.right >= 0 );
		GammaAst( rtDes.right <= nWidth );
		GammaAst( rtDes.top >= 0 );
		GammaAst( rtDes.top < rtDes.bottom );
		GammaAst( rtDes.bottom >= 0 );
		GammaAst( rtDes.bottom <= nHeight );

		float fScaleX = rtSrc.Width()/(float)rtDes.Width();
		float fScaleY = rtSrc.Height()/(float)rtDes.Height();
		for( _IntType nRow = rtDes.top; nRow < rtDes.bottom; ++nRow )
		{
			for( _IntType nCol = rtDes.left; nCol < rtDes.right; ++nCol )
			{
				CVector4f vColor;
				float fStartY = fScaleY*( nRow - rtDes.top ) + rtSrc.top;
				float fEndY = fScaleY*( nRow + 1 - rtDes.top ) + rtSrc.top;
				float fStartX = fScaleX*( nCol - rtDes.left ) + rtSrc.left;
				float fEndX = fScaleX*( nCol + 1 - rtDes.left ) + rtSrc.left;
				_IntType nStartY = (_IntType)fStartY;
				_IntType nEndY = (_IntType)fEndY;
				_IntType nStartX = (_IntType)fStartX;
				_IntType nEndX = (_IntType)fEndX;
				float fWeightY = 1.0f;
				float fWeightX = 1.0f;
				for( _IntType y = nStartY; y <= nEndY; ++y )
				{
					if( y < fStartY )
						fWeightY = Min( ( y + 1 ) - fStartY, fScaleY );
					else if( y == nEndY )
						fWeightY = Min( fEndY - y, fScaleY );
					else
						fWeightY = Min( 1.0f, fScaleY );
					if( fWeightY == 0 )
						continue;
					for( _IntType x = nStartX; x <= nEndX; ++x )
					{
						if( x < fStartX )
							fWeightX = Min( ( x + 1 ) - fStartX, fScaleX );
						else if( x == nEndX )
							fWeightX = Min( fEndX - x, fScaleX );
						else
							fWeightX = Min( 1.0f, fScaleX );
						if( fWeightX == 0 )
							continue;
						_IntType nSrcX = Min( nSrcWidth - 1, x );
						_IntType nSrcY = Min( nSrcHeight - 1, y );
						_IntType nIndex = nSrcY*nSrcWidth + nSrcX;
						vColor += ((CVector4f)pSrc[nIndex] )*fWeightX*fWeightY;
					}
				}
				pDes[nRow*nWidth + nCol] = vColor/( fScaleX*fScaleY );
			}
		}
	}
	template<class T>
	inline T Clamp( T num, T min, T max )
	{
		if( num < min )
			num = min;
		else if( num > max )
			num = max;
		return num;
	}

	template<class T>
	T SmoothDamp(T current, T target, T& currentVelocity, T smoothTime, T maxSpeed, float deltaTime)
	{
		if ( current == target )
			return target;
		smoothTime = Max(0.0001f, smoothTime);
		T inverseTTBy2 = 4 / smoothTime ;     // NOTE Unity中分子为2 但改为4才正确，原因不明
		T percentX2 = inverseTTBy2 * deltaTime;
		T factorA = 1 / (1 + percentX2 + 0.48 * percentX2 * percentX2 + 0.235 * percentX2 * percentX2 * percentX2);
		T distanceVV = current - target;
		T maxMove = maxSpeed * smoothTime;
		distanceVV = Clamp (distanceVV, -maxMove, maxMove);
		T fixTarget = current - distanceVV;
		T dDis = (currentVelocity + inverseTTBy2 * distanceVV) * deltaTime;
		currentVelocity = (currentVelocity - inverseTTBy2 * dDis) * factorA;
		T newCur = fixTarget + (distanceVV + dDis) * factorA;
		if( (fixTarget > current) == (newCur > fixTarget) )
		{
			newCur = fixTarget;
			currentVelocity = (newCur - fixTarget) / deltaTime;;
		}
		return newCur;
	}

	/**
	* https://www.geometrictools.com/Documentation/DistanceLine3Line3.pdf
	* A Robust Implementation
	*/
	template <typename Vec, typename Real>
	class CSegmentSegmentDist
	{
	public:
		struct Result
		{
			float distance, sqrDistance;
			float parameter[2];
			Vec closest[2];
		};

		Result operator()(const Vec& P0, const Vec& P1,
			const Vec& Q0, const Vec& Q1)
		{
			Result result;

			Vec P1mP0 = P1 - P0;
			Vec Q1mQ0 = Q1 - Q0;
			Vec P0mQ0 = P0 - Q0;
			mA = P1mP0.Dot(P1mP0);
			mB = P1mP0.Dot(Q1mQ0);
			mC = Q1mQ0.Dot(Q1mQ0);
			mD = P1mP0.Dot(P0mQ0);
			mE = Q1mQ0.Dot(P0mQ0);

			mF00 = mD;
			mF10 = mF00 + mA;
			mF01 = mF00 - mB;
			mF11 = mF10 - mB;

			mG00 = -mE;
			mG10 = mG00 - mB;
			mG01 = mG00 + mC;
			mG11 = mG10 + mC;

			if (mA > (Real)0 && mC > (Real)0)
			{
				Real sValue[2];
				sValue[0] = GetClampedRoot(mA, mF00, mF10);
				sValue[1] = GetClampedRoot(mA, mF01, mF11);

				int classify[2];
				for (int i = 0; i < 2; ++i)
				{
					if (sValue[i] <= (Real)0)
					{
						classify[i] = -1;
					}
					else if (sValue[i] >= (Real)1)
					{
						classify[i] = +1;
					}
					else
					{
						classify[i] = 0;
					}
				}

				if (classify[0] == -1 && classify[1] == -1)
				{
					result.parameter[0] = (Real)0;
					result.parameter[1] = GetClampedRoot(mC, mG00, mG01);
				}
				else if (classify[0] == +1 && classify[1] == +1)
				{
					result.parameter[0] = (Real)1;
					result.parameter[1] = GetClampedRoot(mC, mG10, mG11);
				}
				else
				{
					int32_t edge[2];
					Real end[2][2];
					ComputeIntersection(sValue, classify, edge, end);
					ComputeMinimumParameters(edge, end, result.parameter);
				}
			}
			else
			{
				if (mA > (Real)0)
				{
					result.parameter[0] = GetClampedRoot(mA, mF00, mF10);
					result.parameter[1] = (Real)0;
				}
				else if (mC > (Real)0)
				{
					result.parameter[0] = (Real)0;
					result.parameter[1] = GetClampedRoot(mC, mG00, mG01);
				}
				else
				{
					result.parameter[0] = (Real)0;
					result.parameter[1] = (Real)0;
				}
			}

			result.closest[0] =
				((Real)1 - result.parameter[0]) * P0 + result.parameter[0] * P1;
			result.closest[1] =
				((Real)1 - result.parameter[1]) * Q0 + result.parameter[1] * Q1;
			Vec diff = result.closest[0] - result.closest[1];
			result.sqrDistance = diff.Dot(diff);
			result.distance = std::sqrt(result.sqrDistance);
			return result;
		}

	private:
		Real GetClampedRoot(Real slope, Real h0, Real h1)
		{
			Real r;
			if (h0 < (Real)0)
			{
				if (h1 > (Real)0)
				{
					r = -h0 / slope;
					if (r > (Real)1)
					{
						r = (Real)0.5;
					}
				}
				else
				{
					r = (Real)1;
				}
			}
			else
			{
				r = (Real)0;
			}
			return r;
		}

		void ComputeIntersection(Real const sValue[2], int const classify[2],
			int edge[2], Real end[2][2])
		{
			if (classify[0] < 0)
			{
				edge[0] = 0;
				end[0][0] = (Real)0;
				end[0][1] = mF00 / mB;
				if (end[0][1] < (Real)0 || end[0][1] > (Real)1)
				{
					end[0][1] = (Real)0.5;
				}

				if (classify[1] == 0)
				{
					edge[1] = 3;
					end[1][0] = sValue[1];
					end[1][1] = (Real)1;
				}
				else
				{
					edge[1] = 1;
					end[1][0] = (Real)1;
					end[1][1] = mF10 / mB;
					if (end[1][1] < (Real)0 || end[1][1] > (Real)1)
					{
						end[1][1] = (Real)0.5;
					}
				}
			}
			else if (classify[0] == 0)
			{
				edge[0] = 2;
				end[0][0] = sValue[0];
				end[0][1] = (Real)0;

				if (classify[1] < 0)
				{
					edge[1] = 0;
					end[1][0] = (Real)0;
					end[1][1] = mF00 / mB;
					if (end[1][1] < (Real)0 || end[1][1] > (Real)1)
					{
						end[1][1] = (Real)0.5;
					}
				}
				else if (classify[1] == 0)
				{
					edge[1] = 3;
					end[1][0] = sValue[1];
					end[1][1] = (Real)1;
				}
				else
				{
					edge[1] = 1;
					end[1][0] = (Real)1;
					end[1][1] = mF10 / mB;
					if (end[1][1] < (Real)0 || end[1][1] > (Real)1)
					{
						end[1][1] = (Real)0.5;
					}
				}
			}
			else
			{
				edge[0] = 1;
				end[0][0] = (Real)1;
				end[0][1] = mF10 / mB;
				if (end[0][1] < (Real)0 || end[0][1] > (Real)1)
				{
					end[0][1] = (Real)0.5;
				}

				if (classify[1] == 0)
				{
					edge[1] = 3;
					end[1][0] = sValue[1];
					end[1][1] = (Real)1;
				}
				else
				{
					edge[1] = 0;
					end[1][0] = (Real)0;
					end[1][1] = mF00 / mB;
					if (end[1][1] < (Real)0 || end[1][1] > (Real)1)
					{
						end[1][1] = (Real)0.5;
					}
				}
			}
		}

		void ComputeMinimumParameters(int const edge[2], Real const end[2][2],
			Real parameter[2])
		{
			Real delta = end[1][1] - end[0][1];
			Real h0 = delta * (-mB * end[0][0] + mC * end[0][1] - mE);
			if (h0 >= (Real)0)
			{
				if (edge[0] == 0)
				{
					parameter[0] = (Real)0;
					parameter[1] = GetClampedRoot(mC, mG00, mG01);
				}
				else if (edge[0] == 1)
				{
					parameter[0] = (Real)1;
					parameter[1] = GetClampedRoot(mC, mG10, mG11);
				}
				else
				{
					parameter[0] = end[0][0];
					parameter[1] = end[0][1];
				}
			}
			else
			{
				Real h1 = delta * (-mB * end[1][0] + mC * end[1][1] - mE);
				if (h1 <= (Real)0)
				{
					if (edge[1] == 0)
					{
						parameter[0] = (Real)0;
						parameter[1] = GetClampedRoot(mC, mG00, mG01);
					}
					else if (edge[1] == 1)
					{
						parameter[0] = (Real)1;
						parameter[1] = GetClampedRoot(mC, mG10, mG11);
					}
					else
					{
						parameter[0] = end[1][0];
						parameter[1] = end[1][1];
					}
				}
				else 
				{
					Real z = std::min(std::max(h0 / (h0 - h1), (Real)0), (Real)1);
					Real omz = (Real)1 - z;
					parameter[0] = omz * end[0][0] + z * end[1][0];
					parameter[1] = omz * end[0][1] + z * end[1][1];
				}
			}
		}

		Real mA, mB, mC, mD, mE;

		Real mF00, mF10, mF01, mF11;

		Real mG00, mG10, mG01, mG11;
	};

	template< typename Vec >
	inline float SegmentSegmentDist(const Vec& a, const Vec& b, const Vec& c, const Vec& d, float* s, float* t)
	{
		CSegmentSegmentDist<Vec, float> sovler;
		auto result = sovler(a, b, c, d);
		if (s != nullptr)
		{
			*s = result.parameter[0];
		}
		if (t != nullptr)
		{
			*t = result.parameter[1];
		}
		return result.distance;
	}

}
