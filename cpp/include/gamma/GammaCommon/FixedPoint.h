#pragma once
#include "GammaCommon/TVector2.h"
#include <float.h>

#define REGION_SIZE	16
#define REGION_COORD_FROM_POSITION( v ) ((int64_t(v)) >> 4)
#define REGION_OFFSET_FROM_POSITION( v ) ((v) & 0xF)
#define POSITION_FROM_REGION_OFFSET( r, o ) ( ( r << 4 )|o )
#define ACTOR_RECIEVE_RANGE 1
#define SPAWNER_RECIEVE_RANGE 1

union UFixedPoint
{ 
private:
	struct  
	{
		uint16_t nFraction;
		int16_t  nInteger;
	};
public:
	int32_t nValue;

	UFixedPoint() :nValue(0) {}
	UFixedPoint(int32_t _value) : nValue(_value) {}
	UFixedPoint(float _value) : nValue((int32_t)(_value * 65536)) {}
	UFixedPoint(int16_t _Integer, uint16_t _Fraction) : nInteger(_Integer), nFraction(_Fraction) {}
	operator float() const { return nValue / 65536.0f; }
	UFixedPoint operator+ (UFixedPoint a) const { return UFixedPoint(nValue + a.nValue); }
	UFixedPoint operator- (UFixedPoint a) const { return UFixedPoint(nValue - a.nValue); }
	UFixedPoint operator* (UFixedPoint a) const { return UFixedPoint((int32_t)((nValue*(int64_t)a.nValue) >> 16)); }
	UFixedPoint operator/ (UFixedPoint a) const { return UFixedPoint((int32_t)((((int64_t)nValue) << 16)/a.nValue)); }
	bool operator == (UFixedPoint a) const { return nValue == a.nValue; }
	int16_t Integer() const { return nInteger; }
	uint16_t Fraction() const { return nFraction; }
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

