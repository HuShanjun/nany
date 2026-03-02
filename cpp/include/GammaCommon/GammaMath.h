//===============================================
// GammaMath.h
// 定义常用数学函数
// 柯达昭
// 2007-08-31
//===============================================

#ifndef __GAMMA_MATH_H__
#define __GAMMA_MATH_H__

#include "GammaCommon/GammaCommonType.h"
#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/TVector2.h"
#include "GammaCommon/TVector3.h"
#include "GammaCommon/TVector4.h"
#include "GammaCommon/TRect.h"
#include "GammaCommon/CMatrix.h"

#define GM_PI					3.1415926536f
#define GM_2PI					6.2831853072f
#define GM_PI_2					1.5707963268f
#define GM_MATH_EPSILON			0.00001f
#define	GM_SQRT2				1.4142135623f
#define GM_INV_SQRT2			(1.0f / GM_SQRT2)
#define GM_2_RADIAN( fDegree )	( ( fDegree )*( GM_PI/180.0f ) )
#define GM_2_DEGREE( fRadian )	( ( fRadian )*( 180.0f/GM_PI ) )
#define GM_IS_NAN( f )			( ( f ) != ( f ) )

namespace Gamma
{
    class CMatrix;

	inline float	Arc2Angle( float a )							{ return a*180/GM_PI; }
	inline float	Angle2Arc( float a )							{ return a*GM_PI/180; }

	//========================================================================
	// 查表三角函数  
	//========================================================================
	float GammaSin( float f );
	float GammaCos( float f );
	float GammaASin( float f );
	float GammaACos( float f );

	template<class T>
	T GammaAbs( T t );

	template<class T>
	T GammaSigned( T t );
    //========================================================================
    // 求整数平方根  
    // 比sqrt快  
    //=========================================================================
    GAMMA_COMMON_API uint16_t sqrti( uint32_t M );
    GAMMA_COMMON_API uint32_t sqrti64( uint64_t M );

    //========================================================================
    // 不受浮点精度影响转换函数  
    //========================================================================
    GAMMA_COMMON_API double u2d( uint32_t uValue );

    //========================================================================
    // 极速浮点转整形函数  
    // 比系统提供的快  
    //=========================================================================
    GAMMA_COMMON_API int32_t f2i( float f );

    //========================================================================
    // 浮点转无符号整形函数  
    //=========================================================================
	GAMMA_COMMON_API uint32_t f2u( float f );

	//========================================================================
	// 字符串转换为整数  
    ///@return 如果不能转换为数字则返回零  
	//========================================================================
	GAMMA_COMMON_API int32_t GammaA2I( const wchar_t* szStr );
	GAMMA_COMMON_API int32_t GammaA2I( const char* szStr );


	GAMMA_COMMON_API int64_t GammaA2I64( const wchar_t* szStr );
	GAMMA_COMMON_API int64_t GammaA2I64( const char* szStr );

	//========================================================================
	// 字符串转换为浮点  
	//========================================================================
	GAMMA_COMMON_API double GammaA2F( const wchar_t* szStr );
	GAMMA_COMMON_API double GammaA2F( const char* szStr );

	//========================================================================
	// 向下取整  
	//========================================================================
	template<typename IntegerType>
	IntegerType FloorDiv( IntegerType v, IntegerType d )
	{
		if( ( v >= (IntegerType)0 && d >= (IntegerType)0 ) ||
			( v <= (IntegerType)0 && d <= (IntegerType)0 ) )
			return v/d;
		if( v < (IntegerType)0 )
			return ( v + 1 )/d - (IntegerType)1;
		return ( v - (IntegerType)1 )/d - (IntegerType)1;
	}

	//========================================================================
	// 整数常量乘方  
	//========================================================================
	template<int32_t m, int32_t n>
	struct TPower
	{
		enum { eValue = TPower< m, n/2 >::eValue*TPower< m, n - n/2 >::eValue };
	};

	template <int32_t m>
	struct TPower< m, 1 >
	{
		enum { eValue = m };
	};

	//========================================================================
	// 整数常量开方  
	//========================================================================
	template<uint32_t s>
	struct TSqrt
	{
		template <uint32_t m, uint32_t n, uint32_t t, uint32_t i>
		struct TLoop
		{
			enum
			{
				eN = n << 1,
				eT = ( t << 2 ) + ( m >> 30 ),
				eV = ( eN << 1 ) + 1,
				eValue =
				i == 0 ?
				n :
				TLoop<
					m << 2,
					eT >= eV ? eN + 1 : eN,
					eT >= eV ? eT - eV : eT,
					i - 1
				>::eValue
			};
		};

		template <uint32_t m, uint32_t n, uint32_t t>
		struct TLoop< m, n, t, 0>
		{
			enum
			{
				eValue = n
			};
		};

		enum
		{
			eV = s >> 30,
			eN = eV >= 1 ? 1 : 0,
			eT = eV >= 1 ? eV - 1 : eV,
			eValue = TLoop< s << 2, eN, eT, 15 >::eValue
		};
	};

	//========================================================================
	// 整数对数  
	//========================================================================
	inline uint8_t GammaLog2( uint32_t x )
	{
		return uint8_t(
			( ( ( ( x ) & 0xAAAAAAAA ) ? 1 : 0 )      ) |
			( ( ( ( x ) & 0xCCCCCCCC ) ? 1 : 0 ) << 1 ) |
			( ( ( ( x ) & 0xF0F0F0F0 ) ? 1 : 0 ) << 2 ) |
			( ( ( ( x ) & 0xFF00FF00 ) ? 1 : 0 ) << 3 ) |
			( ( ( ( x ) & 0xFFFF0000 ) ? 1 : 0 ) << 4 ) );
	}

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
    inline T TriInterpolate( const T& T0, const T& T1, const T& T2, const T& T3, float x, float y );

	//==================================================================================================================
	//! \fn		T BezierInterpolate( const T& T0, const T& T1, const T& T2, const T& T3, float p );
	//  \brief	贝塞尔曲线插值
	//
	//  \param	T0 起点， T3终点， T1是T0的控制点，T2是T3的控制点，p的值由0～1
	//  \return T 插值结果
	//==================================================================================================================
	template<class T>
	inline T BezierInterpolate( const T& T0, const T& T1, const T& T2, const T& T3, float p );


    /** 厄密特曲线插值，效果和贝塞尔等价，控制方式不同  
    @param p0 起始点  
    @param p1 终止点  
    @param t0 起始点处的切线  
    @param t1 终止点处的切线  
    @param u 比例，0~1  
    */
    template<class T>
    inline T HermiteInterpolate( const T& p0, const T& p1, const T& t0, const T& t1, float u );

    /** Catmull-Rom 曲线插值  
    @param v0 前一个点  
    @param v1 当前点  
    @param v2 下一个点  
    @param v3 下2个点  
    @param a v1到v2间的比重，0~1  
    @param pOutCoeffs 如果不为空，存储方程的4个系数，以便后续使用  
    */
    template<class T>
    inline T CatmullRomSpline( const T& v0, const T& v1, const T& v2, const T& v3, float a, T* pOutCoeffs = NULL );

    //=================================================================================================================
    // 画线  
    // TightLine
    //   ██  
    //     ██  
    //       ██  
    // Line
    //   █  
    //     █  
    //       █  
    //=================================================================================================================
	template<typename ILineAction>
	inline bool	TightLineTo( int32_t xSrc, int32_t ySrc, int32_t xDes, int32_t yDes, ILineAction Action );
	template<typename ILineAction>
    inline bool	LineTo( int32_t xSrc, int32_t ySrc, int32_t xDes, int32_t yDes, ILineAction Action, bool bCheckEnd = false );


	GAMMA_COMMON_API CVector3f	SceneToScreen( const CIRect& rtScreen, const CVector3f& vScene, const CMatrix& matWorldViewProject );
	GAMMA_COMMON_API CVector3f	ScreenToScene( const CIRect& rtScreen, const CVector3f& vScreen, const CMatrix& matWorldView, const CMatrix& matProject );

	//=================================================================================================================
	// 是否在三角形里 -1:不在; 0:边上; 1:在  
	//=================================================================================================================
	GAMMA_COMMON_API int32_t		IsInTriangle( const CVector2f& v, const CVector2f& v0, const CVector2f& v1, const CVector2f& v2 );
	GAMMA_COMMON_API int32_t		IsInTriangle( const CVector3f& v, const CVector3f& v0, const CVector3f& v1, const CVector3f& v2 );

    //=================================================================================================================
    // 3D碰撞测试  
	//=================================================================================================================
	GAMMA_COMMON_API bool		IsSelectTriangle( CPos posScreen, CIRect rtScreen, CVector3f v0, CVector3f v1, CVector3f v2, const CMatrix& matWorldViewProject );
	GAMMA_COMMON_API bool		DetectInBox( const CVector2f& vPosInProject, CVector3f vScale, CVector3f vOffset, const CMatrix& matWorldViewProject );

    template< class M, class V >
	inline void CreateShadowMatrixOnPlane( M& mat, const V& vecLightDir, const V& vecPlaneNormal, const V& posPointOfPlane );

	//=================================================================================================================
	// 同一平面内线段（xStart，xEnd）是否在线段（xMin，xMan）内
	//=================================================================================================================
	template<typename ValueType, class VecType>
	inline bool CutLine( const ValueType& xMin, const ValueType& xMax, const ValueType& xStart, const ValueType& xEnd, VecType& vStart, const VecType& vEnd );

	//=================================================================================================================
	// 线段（posStart，posEnd）是否与矩形rt相交
	//=================================================================================================================
	template<class _Rect, class _VECTOR>
	inline bool Cut2DLine( const _Rect& rt, _VECTOR& posStart, _VECTOR& posEnd );


	template<class _VECTOR>
	inline bool Cut3DLine( const _VECTOR& vMin, const _VECTOR& vMax, _VECTOR& vStart, _VECTOR& vEnd );


	//=============================================================================================================================
	// 线段(a,b)上离某点(c)的最近的点d
	//=============================================================================================================================
	template< typename Vec >
	inline float ClosetPointSegment( const Vec& a, const Vec& b, const Vec& c, Vec* d );

	//=============================================================================================================================
	// 线段(a,b)上离线段(c,d)的距离，s、t为线段ab、cd的参数值
	//=============================================================================================================================
	template< typename Vec >
	inline float SegmentSegmentDist( const Vec& a, const Vec& b, const Vec& c, const Vec& d, float* s, float* t);

	//**************************************************************************************************************
	/*! \fn     std::pair<uint32_t,uint32_t> BuildSphere  
	*   \brief  生成球体，椎体，圆柱体mesh  
	*			pVertex和pIndex任意一个为空则进返回顶点数以及索引数  
	*
	*   \param	pVertex 足够大的顶点缓冲，可以为NULL  
	*   \param	pIndex 足够大的索引缓冲，可以为NULL  
	*   \param  nCircleDivCount xz水平圆周切分段数  
	*   \param  nHeightDivCount y方向垂直切分段数  
	*   \param  fRadiusY y轴半径  
	*   \param  fUpRadiusXZ,fDownRadiusXZ  如果fDownRadiusXZ < 0，则生成球体且fUpRadiusXZ为球体水平方向半径
	*										否则fUpRadiusXZ、fDownRadiusXZ分别为主体上下半径  
	*   \param  bTop 是否封顶  
	*   \param  bBottom 是否封底  
	*   \param  fUSectorStart 在水平圆周分段方向开始生成mesh的起点的U  
	*   \param  fUSectorEnd 在水平圆周分段方向开始生成mesh的终点的U  
	*   \param  fVSegmentStart y方向垂直分段方向开始生成mesh的起点的V  
	*   \param  fVSegmentEnd y方向垂直分段方向开始生成mesh的终点的V   
	*   \param  bWrapUV UV包装方式：true.沿水平方向包围，false.上下覆盖合围  
	*   \param  fSectorStart 在水平圆周分段方向开始生成mesh的起点  
	*   \param  fSectorEnd 在水平圆周分段方向开始生成mesh的终点  
	*   \param  fSegmentStart y方向垂直分段方向开始生成mesh的起点  
	*   \param  fSegmentEnd y方向垂直分段方向开始生成mesh的终点  
	*   \param  nVetexContext 传入顶点构造函数的上下文参数  
	*
	*   \return pair<uint32_t,uint32_t> 顶点数以及索引数  
	*   \sa     GetMinAxis
	***************************************************************************************************************/
	template<typename _VertexVec>
	inline std::pair<uint32_t,uint32_t> BuildSphere( _VertexVec* pVertex, uint16_t* pIndex,
		int32_t nCircleDivCount, int32_t nHeightDivCount, float fRadiusY, float fUpRadiusXZ, float fDownRadiusXZ = -1, bool bTop = false, bool bBottom = false,
		float fUSectorStart = 0.0f, float fUSectorEnd = 1.0f, float fVSegmentStart = 0.0f, float fVSegmentEnd = 1.0f, bool bWrapUV = true,
		float fSectorStart = 0.0f, float fSectorEnd = 1.0f, float fSegmentStart = 0.0f, float fSegmentEnd = 1.0f, uint32_t nVetexContext = 0 );

    /// 测试射线和三角形的相交  
    GAMMA_COMMON_API bool IsLineIntersectTriangle( CVector3f& vIntersect, const CVector3f& vStart, const CVector3f& vDir,
        const CVector3f& v0, const CVector3f& v1, const CVector3f& v2 );

	template<class T>
	inline bool Intersect2DLine( const TVector2<T>& a1, const TVector2<T>& a2, const TVector2<T>& b1, const TVector2<T>& b2 );

	template<class T>
	inline T Clamp( T num, T min, T max );

	template<class T>
	inline T SmoothDamp( T current, T target, T& currentVelocity, T smoothTime, T maxSpeed, float deltaTime );

	//=============================================================================================================================
	// 矩形拉伸填充
	// 外部保证参数合法，不越界
	//=============================================================================================================================
	template<typename _Elem, typename _IntType>
	inline void RectStretch( _Elem* pDes, _IntType nWidth, _IntType nHeight, const TRect<_IntType>& rtDes, 
		const _Elem* pSrc, _IntType nSrcWidth, _IntType nSrcHeight, const TRect<_IntType>& rtSrc );
}

#include "GammaCommon/GammaMath.inl"

#endif
