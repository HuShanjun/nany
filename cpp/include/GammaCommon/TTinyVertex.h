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
    template< int32 pb = 10, int32 tb = 11, int32 tc = 1 >
    class CTinyVertex : public TBitSet< pb*3 + 12/*nb*/ + tb*2*tc, uint32, true >
    {
        typedef TBitSet< pb*3 + 12 + tb*2*tc, uint32, true> TParent;
        enum { nb = 12 };
    public:

        CTinyVertex(){};

        CTinyVertex( uint16 x, uint16 y, uint16 z, uint16 n, uint16 u[tc], uint16 v[tc] )
        {
			TParent::SetBit( pb*0,			x,		pb );
			TParent::SetBit( pb*1,			y,		pb );
			TParent::SetBit( pb*2,			z,		pb );
			TParent::SetBit( pb*3,			n,		nb );

			for( int32 i = 0, n = pb*3 + nb; i < tc; i++, n += tb*2 )
			{
				TParent::SetBit( n,			u[i],	tb );
				TParent::SetBit( n + tb,	v[i],	tb );
			}
        }

        uint16        GetX()				{ return (uint16)TParent::GetBit( 0, pb ); }
        uint16        GetY()				{ return (uint16)TParent::GetBit( pb, pb ); }
        uint16        GetZ()				{ return (uint16)TParent::GetBit( pb*2, pb ); }
        uint16        GetN()				{ return (uint16)TParent::GetBit( pb*3, nb ); }
        uint16        GetU( uint8 n = 0 )	{ return (uint16)TParent::GetBit( pb*3 + nb + n*tb*2, tb ); }
        uint16        GetV( uint8 n = 0 )	{ return (uint16)TParent::GetBit( pb*3 + nb + n*tb*2 + tb, tb ); }


        CTinyVertex( CVector3f v, CVector3f n, CVector2f t[] )
        {
			TParent::SetBit( pb*0,			uint16( v.x*4 + 0.5f ),				pb );
			TParent::SetBit( pb*1,			uint16( v.y*4 + 0.5f ),				pb );
			TParent::SetBit( pb*2,			uint16( v.z*4 + 0.5f ),				pb );
			TParent::SetBit( pb*3,			TTinyNormal<12>::Compress1( n ),	nb );

			for( int32 i = 0, n = pb*3 + nb; i < tc; i++, n += tb*2 )
			{
				TParent::SetBit( n,			uint16( t[i].x*2000 + 0.5f ),		tb );
				TParent::SetBit( n + tb,	uint16( t[i].y*2000 + 0.5f ),		tb );
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

        CVector2f GetTex( uint8 n = 0 )	
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
		uint8 w[4];

		CTinyWeight(){}

		CTinyWeight( uint32 w )
		{
			*reinterpret_cast<uint32*>( w ) = w;
		}

		CTinyWeight( uint8 w0, uint8 w1, uint8 w2, uint8 w3 )
		{
			w[0] = w0;
			w[1] = w1;
			w[2] = w2;
			w[3] = w3;
		}

        CTinyWeight( float w0, float w1, float w2, float w3 )
        {
			int32 n = 255;
            w[0] = (uint8)( Limit<int32>( (int32)( w0*255 + 0.5f ), 0, n ) );  n -= w[0];
            w[1] = (uint8)( Limit<int32>( (int32)( w1*255 + 0.5f ), 0, n ) );  n -= w[1];
            w[2] = (uint8)( Limit<int32>( (int32)( w2*255 + 0.5f ), 0, n ) );  n -= w[2];
            w[3] = (uint8)n;
        }

        CTinyWeight( float* w )
        {
			*this = CTinyWeight( w[0], w[1], w[2], w[3] );
        }

		float GetWeight( uint32 nIndex ) const { return w[nIndex]/255.0f;}
		uint8 GetRawWeight( uint32 nIndex ) const { return w[nIndex];}
    };
}

#endif
