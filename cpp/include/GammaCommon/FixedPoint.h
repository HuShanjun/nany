#pragma once
#include "GammaCommon/TVector2.h"
#include <float.h>

#define REGION_SIZE	16
#define REGION_COORD_FROM_POSITION( v ) ((int64(v)) >> 4)
#define REGION_OFFSET_FROM_POSITION( v ) ((v) & 0xF)
#define POSITION_FROM_REGION_OFFSET( r, o ) ( ( r << 4 )|o )
#define ACTOR_RECIEVE_RANGE 1
#define SPAWNER_RECIEVE_RANGE 1

union UFixedPoint
{ 
private:
	struct  
	{
		uint16 nFraction;
		int16  nInteger;
	};
public:
	int32 nValue;

	UFixedPoint() :nValue(0) {}
	UFixedPoint(int32 _value) : nValue(_value) {}
	UFixedPoint(float _value) : nValue((int32)(_value * 65536)) {}
	UFixedPoint(int16 _Integer, uint16 _Fraction) : nInteger(_Integer), nFraction(_Fraction) {}
	operator float() const { return nValue / 65536.0f; }
	UFixedPoint operator+ (UFixedPoint a) const { return UFixedPoint(nValue + a.nValue); }
	UFixedPoint operator- (UFixedPoint a) const { return UFixedPoint(nValue - a.nValue); }
	UFixedPoint operator* (UFixedPoint a) const { return UFixedPoint((int32)((nValue*(int64)a.nValue) >> 16)); }
	UFixedPoint operator/ (UFixedPoint a) const { return UFixedPoint((int32)((((int64)nValue) << 16)/a.nValue)); }
	bool operator == (UFixedPoint a) const { return nValue == a.nValue; }
	int16 Integer() const { return nInteger; }
	uint16 Fraction() const { return nFraction; }
};

typedef Gamma::TVector2<UFixedPoint> CVector2x;

class CBox2D
{
public:
	CBox2D()
		: m_vMin( FLT_MAX, FLT_MAX )
		, m_vMax( -FLT_MAX, -FLT_MAX )
	{}

	CBox2D( const CVector2x& _vMin, const CVector2x& _vMax )
		: m_vMin( _vMin )
		, m_vMax( _vMax )
	{}

	bool Intersect( const CBox2D& otherBox ) const
	{
		if( m_vMax.x < otherBox.m_vMin.x || m_vMin.x > otherBox.m_vMax.x )
			return false;
		if( m_vMax.y < otherBox.m_vMin.y || m_vMin.y > otherBox.m_vMax.y )
			return false;
		return true;
	}

	CVector2x GetMin() const { return m_vMin; }

	CVector2x GetMax() const { return m_vMax; }

private:
	CVector2x m_vMax;
	CVector2x m_vMin;
};

