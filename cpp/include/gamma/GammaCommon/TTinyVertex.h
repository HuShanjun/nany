//==============================================================
// TTinyVertex.h 
// 定义顶点压缩算法
// 柯达昭
// 2007-09-11
//==============================================================

#ifndef _GAMMA_TINYVERTEX_H_
#define _GAMMA_TINYVERTEX_H_

#include "GammaCommon/TBitSet.h"
#include "GammaCommon/TVector2.h"
#include "GammaCommon/TVector3.h"
#include "GammaCommon/TTinyNormal.h"

namespace Gamma
{
    //===============================================================
    // 顶点的物理坐标以及法线、贴图坐标
    // pb： 每个顶点其中一维所占用的位数
    // nb： 法线占用的位数
    // tb： 贴图坐标其中一维所占用的位数
    //===============================================================
    template< int32_t pb = 10, int32_t tb = 11, int32_t tc = 1 >
    class CTinyVertex : public TBitSet< pb*3 + 12/*nb*/ + tb*2*tc, uint32_t, true >
    {
        typedef TBitSet< pb*3 + 12 + tb*2*tc, uint32_t, true> TParent;
        enum { nb = 12 };
    public:

        CTinyVertex(){};

        CTinyVertex( uint16_t x, uint16_t y, uint16_t z, uint16_t n, uint16_t u[tc], uint16_t v[tc] )
        {
			TParent::SetBit( pb*0,			x,		pb );
			TParent::SetBit( pb*1,			y,		pb );
			TParent::SetBit( pb*2,			z,		pb );
			TParent::SetBit( pb*3,			n,		nb );

			for( int32_t i = 0, n = pb*3 + nb; i < tc; i++, n += tb*2 )
			{
				TParent::SetBit( n,			u[i],	tb );
				TParent::SetBit( n + tb,	v[i],	tb );
			}
        }

        uint16_t        GetX()				{ return (uint16_t)TParent::GetBit( 0, pb ); }
        uint16_t        GetY()				{ return (uint16_t)TParent::GetBit( pb, pb ); }
        uint16_t        GetZ()				{ return (uint16_t)TParent::GetBit( pb*2, pb ); }
        uint16_t        GetN()				{ return (uint16_t)TParent::GetBit( pb*3, nb ); }
        uint16_t        GetU( uint8_t n = 0 )	{ return (uint16_t)TParent::GetBit( pb*3 + nb + n*tb*2, tb ); }
        uint16_t        GetV( uint8_t n = 0 )	{ return (uint16_t)TParent::GetBit( pb*3 + nb + n*tb*2 + tb, tb ); }


        CTinyVertex( CVector3f v, CVector3f n, CVector2f t[] )
        {
			TParent::SetBit( pb*0,			uint16_t( v.x*4 + 0.5f ),				pb );
			TParent::SetBit( pb*1,			uint16_t( v.y*4 + 0.5f ),				pb );
			TParent::SetBit( pb*2,			uint16_t( v.z*4 + 0.5f ),				pb );
			TParent::SetBit( pb*3,			TTinyNormal<12>::Compress1( n ),	nb );

			for( int32_t i = 0, n = pb*3 + nb; i < tc; i++, n += tb*2 )
			{
				TParent::SetBit( n,			uint16_t( t[i].x*2000 + 0.5f ),		tb );
				TParent::SetBit( n + tb,	uint16_t( t[i].y*2000 + 0.5f ),		tb );
			}
        }

        CVector3f GetPos()				
		{ 
			return CVector3f( 
				TParent::GetBit( 0, pb )*0.25f, 
				TParent::GetBit( pb, pb )*0.25f, 
				TParent::GetBit( pb*2, pb )*0.25f );
		}

        CVector3f GetNor()
		{ 
			return TTinyNormal<12>::Decompress1( TParent::GetBit( pb*3, nb ) );
		}

        CVector2f GetTex( uint8_t n = 0 )	
		{ 
			return CVector2f( 
				TParent::GetBit( pb*3 + nb + n*tb*2, tb )*0.0005f, 
				TParent::GetBit( pb*3 + nb + n*tb*2 + tb, tb )*0.0005f );
		}
    };


    //===============================================================
    // 顶点的骨骼权重
    //===============================================================
    class CTinyWeight
    {
	public:
		uint8_t w[4];

		CTinyWeight(){}

		CTinyWeight( uint32_t w )
		{
			*reinterpret_cast<uint32_t*>( w ) = w;
		}

		CTinyWeight( uint8_t w0, uint8_t w1, uint8_t w2, uint8_t w3 )
		{
			w[0] = w0;
			w[1] = w1;
			w[2] = w2;
			w[3] = w3;
		}

        CTinyWeight( float w0, float w1, float w2, float w3 )
        {
			int32_t n = 255;
            w[0] = (uint8_t)( Limit<int32_t>( (int32_t)( w0*255 + 0.5f ), 0, n ) );  n -= w[0];
            w[1] = (uint8_t)( Limit<int32_t>( (int32_t)( w1*255 + 0.5f ), 0, n ) );  n -= w[1];
            w[2] = (uint8_t)( Limit<int32_t>( (int32_t)( w2*255 + 0.5f ), 0, n ) );  n -= w[2];
            w[3] = (uint8_t)n;
        }

        CTinyWeight( float* w )
        {
			*this = CTinyWeight( w[0], w[1], w[2], w[3] );
        }

		float GetWeight( uint32_t nIndex ) const { return w[nIndex]/255.0f;}
		uint8_t GetRawWeight( uint32_t nIndex ) const { return w[nIndex];}
    };
}

#endif
