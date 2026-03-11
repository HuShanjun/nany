#pragma once
#include <float.h>
#include "GammaCommon/GammaCommonType.h"
#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/TVector2.h"
#include "GammaCommon/CGammaRand.h"

namespace Gamma
{    
	//===================================================================================
	// 多边形分解为三角形
	// ITriangleReciver { void AddTriangle( const Vector2& v0, const Vector2& v1, const Vector2& v2 ); }
	// aryPoint要求顺时针排列
	//===================================================================================
	template<class Iterator, class DataType, class ITriangleReciver>
	inline void TriangleDecomposition( Iterator itBegin, Iterator itEnd, ITriangleReciver& Reciver );

	//===================================================================================
	// 水平垂直边的多边形分解为矩形
	// ITriangleReciver { void AddRectangle( xMin, yMin, xMax, yMax ); }
	// aryPoint要求顺时针排列
	//===================================================================================
	template<class Iterator, class DataType, class IRectangleReciver>
	inline void RectangleDecomposition( Iterator itBegin, Iterator itEnd, IRectangleReciver& Reciver );
}

#include "GammaCommon/ConvexDecomposition.inl"