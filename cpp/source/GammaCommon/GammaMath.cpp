
#include "GammaCommon/GammaMath.h"
#include "GammaCommon/TVector3.h"
#include "GammaCommon/TRect.h"
#include "GammaCommon/CMatrix.h"
#include "GammaCommon/GammaCodeCvs.h"
#include "GammaCommon/CPlane.h"
#include <stdlib.h>

namespace Gamma
{
	const CMatrix CMatrix::IDENTITY;

	CTrigFunctionTable::CTrigFunctionTable()
	{
		for( int32 i = 0; i < eTableSize; i++ )
		{
			m_fSinBuf[i] = sin( i*GM_PI_2/eTableSize );
			m_fASinBuf[i] = asin( float(i)/eTableSize );
		}
	}

	const CTrigFunctionTable& CTrigFunctionTable::Instance()
	{
		static CTrigFunctionTable _instance;
		return _instance;
	};

    //------------------------------------------------------------
    // 3D碰撞测试
	//-------------------------------------------------------------
	int32 IsInTriangle( const CVector2f& v, const CVector2f& v0, const CVector2f& v1, const CVector2f& v2 )
	{
		CVector2f d0 = v0 - v;
		if( d0 == CVector2f() )
			return 0;

		CVector2f d1 = v1 - v;
		if( d1 == CVector2f() )
			return 0;

		CVector2f d2 = v2 - v;
		if( d2 == CVector2f() )
			return 0;

		float fCross0 = d0.x*d1.y - d0.y*d1.x;
		if( fCross0 == 0 )
			return 0;

		float fCross1 = d1.x*d2.y - d1.y*d2.x;
		if( fCross1 == 0 )
			return 0;

		float fCross2 = d2.x*d0.y - d2.y*d0.x;
		if( fCross2 == 0 )
			return 0;

		if( ( fCross0 < 0 && fCross1 < 0 && fCross2 < 0 ) ||
			( fCross0 > 0 && fCross1 > 0 && fCross2 > 0 ) )
			return 1;
		return -1;
	}

    int32 IsInTriangle( const CVector3f& v, const CVector3f& v0, const CVector3f& v1, const CVector3f& v2 )
    {
        CVector3f d0 = v0 - v;
		if( d0 == CVector3f() )
			return 0;

        CVector3f d1 = v1 - v;
		if( d1 == CVector3f() )
			return 0;

        CVector3f d2 = v2 - v;
		if( d2 == CVector3f() )
			return 0;

        CVector3f cross0 = d0.Cross(d1);
		if( cross0 == CVector3f() )
			return 0;

        CVector3f cross1 = d1.Cross(d2);
		if( cross1 == CVector3f() )
			return 0;

        CVector3f cross2 = d2.Cross(d0);
		if( cross2 == CVector3f() )
			return 0;

        cross0.Normalize();
        cross1.Normalize();
        cross2.Normalize();

        if( fabs(cross0.x - cross1.x) < 0.0001f && fabs(cross0.y - cross1.y) < 0.0001f && fabs(cross0.z - cross1.z) < 0.0001f &&
            fabs(cross2.x - cross1.x) < 0.0001f && fabs(cross2.y - cross1.y) < 0.0001f && fabs(cross2.z - cross1.z) < 0.0001f )
            return 1;
        else
            return -1;
    }

    CVector3f SceneToScreen( const CIRect& rtScreen, const CVector3f& vScene, const CMatrix& matViewProject )
    {
        CVector3f vProj = vScene * matViewProject;
        return CVector3f( vProj.x*rtScreen.Width()/2.0f + rtScreen.Width()/2.0f,
            vProj.y*rtScreen.Height()/(-2.0f) + rtScreen.Height()/2.0f,
            vProj.z );
	}

	CVector3f ScreenToScene( const CIRect& rtScreen, const CVector3f& vScreen, const CMatrix& matWorldView, const CMatrix& matProject )
	{
		float fWidth	= (float)rtScreen.Width();
		float fHeight	= (float)rtScreen.Height();
		CVector3f v		= CVector3f( ( vScreen.x*2 - fWidth )/fWidth, ( fHeight - vScreen.y*2 )/fHeight, vScreen.z );

		if( matProject._44 == 0 )
		{
			v.z	= matProject._43/( v.z - matProject._33 );
			v.x	= v.x*v.z/matProject._11;
			v.y	= v.y*v.z/matProject._22;
		}
		else
		{
			v.x	= v.x/matProject._11;
			v.y	= v.y/matProject._22;
			v.z	= ( v.z - matProject._43 )/matProject._33;
		}

		CMatrix matInvert = matWorldView;
		matInvert.Invert();
		return v*matInvert;
	}

    bool IsSelectTriangle( CPos posScreen, CIRect rtScreen, CVector3f v0, CVector3f v1, CVector3f v2, const CMatrix& matWorldViewProject )
    {
        CVector3f v[3] =
        {
            SceneToScreen( rtScreen, v0, matWorldViewProject ),
            SceneToScreen( rtScreen, v1, matWorldViewProject ),
            SceneToScreen( rtScreen, v2, matWorldViewProject )
        };

        //转成屏幕坐标
        for( int32 i = 0; i < 3; i++ )
            if( v[i].z < 0.0f || v[i].z > 1.0f )
                return false;

        CVector3f vPt( (float)posScreen.x, (float)posScreen.y, 0.0f );

        ////计算是否相交
        // By Kedazhao
        // 因为z值都为零，所以不必要进行完全的叉乘，而且z的正负已经足以判断是否选中，不必单位化
        //判断一个点是否在一个三角形内
        CVector3f d0 = v[0] - vPt;
        CVector3f d1 = v[1] - vPt;
        CVector3f d2 = v[2] - vPt;

        float z1 = d0.x*d1.y - d0.y*d1.x;
        float z2 = d1.x*d2.y - d1.y*d2.x;
        float z3 = d2.x*d0.y - d2.y*d0.x;

        if( z1 > 0 && z2 > 0 && z3 > 0 )
			return true; 
		if( z1 < 0 && z2 < 0 && z3 < 0 )
            return true; 

        return false;
    }

	bool DetectInBox( const CVector2f& vPosInProject, CVector3f vScale, CVector3f vOffset, const CMatrix& matWorldViewProject )
	{
		//              6__________4
		//             /|        /|
		//           0/_|______2/ |
		//            | |       | | 
		//            | |       | |
		//            | |7______|_|5
		//            | /       | /
		//           1|/________|/3


		float x = vScale.x / 2.0f;
		float y = vScale.y / 2.0f;
		float z = vScale.z / 2.0f;

		CVector3f av[8] = 
		{
			CVector3f( -x + vOffset.x,  y + vOffset.y, -z + vOffset.z ),
			CVector3f( -x + vOffset.x, -y + vOffset.y, -z + vOffset.z ),
			CVector3f(  x + vOffset.x,  y + vOffset.y, -z + vOffset.z ),
			CVector3f(  x + vOffset.x, -y + vOffset.y, -z + vOffset.z ),
			CVector3f(  x + vOffset.x,  y + vOffset.y,  z + vOffset.z ),
			CVector3f(  x + vOffset.x, -y + vOffset.y,  z + vOffset.z ),
			CVector3f( -x + vOffset.x,  y + vOffset.y,  z + vOffset.z ),
			CVector3f( -x + vOffset.x, -y + vOffset.y,  z + vOffset.z )
		};

		//转成屏幕坐标
		for( int i = 0; i < 8; i++ )
		{
			av[i] = av[i]*matWorldViewProject;
			if( av[i].z < 0.0f || av[i].z > 1.0f )
				return false;
		}

		CVector3f vPt( vPosInProject.x, vPosInProject.y, 0.0f );

		////计算是否相交
		// 因为z值都为零，所以不必要进行完全的叉乘，而且z的正负已经足以判断是否选中，不必单位化
		static int32 Index[12][3] =  
		{
			{ 0, 1, 2 },{ 1, 2, 3 },{ 2, 3, 4 },{ 3, 4, 5 },
			{ 4, 5, 6 },{ 5, 6, 7 },{ 6, 7, 0 },{ 7, 0, 1 },
			{ 1, 3, 7 },{ 3, 5, 7 },{ 0, 2, 4 },{ 0, 4, 6 },
		};

		//计算是否相交
		for( int i = 0; i < 12; i++ )
		{
			//判断一个点是否在一个三角形内
			CVector3f d0 = av[Index[i][0]] - vPt;
			CVector3f d1 = av[Index[i][1]] - vPt;
			CVector3f d2 = av[Index[i][2]] - vPt;

			float z1 = d0.x*d1.y - d0.y*d1.x;
			float z2 = d1.x*d2.y - d1.y*d2.x;
			float z3 = d2.x*d0.y - d2.y*d0.x;

			if( z1 == 0 )
				continue;
			if( z1 > 0 && ( z2 < 0 || z3 < 0 ) )
				continue; 
			if( z1 < 0 && ( z2 > 0 || z3 > 0 ) )
				continue;

			return true;
		}

		return false;
	}

	int32 GammaA2I( const wchar_t* szStr )
	{
		if( !szStr )
			return 0;
		char szBuf[256];
		UcsToUtf8( szBuf, 64, szStr );
		return (int32)strtol( szBuf, NULL, 0 );
	}

	int32 GammaA2I( const char* szStr )
	{
		if( !szStr )
			return 0;
		return (int32)strtol( szStr, NULL, 0 );
	}

	int64 GammaA2I64( const wchar_t* szStr )
	{
		if( !szStr )
			return 0;
		char szBuf[256];
		uint32 i = 0;
		while ( i < 128 && szStr[i] )
			szBuf[i] = (char)szStr[i];
		szBuf[i] = 0;
		return GammaA2I64( szBuf );
	}

	int64 GammaA2I64( const char* szStr )
	{
		if( !szStr )
			return 0;
#ifdef _WIN32 
		int64 nVal = _strtoi64( szStr, NULL, 0 );
#else
		int64 nVal = strtoll( szStr, NULL, 0 );		
#endif
		 return nVal;
	}

	double GammaA2F( const wchar_t* szStr )
	{
		char szBuf[256];
		UcsToUtf8( szBuf, 64, szStr );
		return atof( szBuf );
	}

	double GammaA2F( const char* szStr )
	{
		return atof( szStr );
	}

    bool IsLineIntersectTriangle( CVector3f& vIntersect, const CVector3f& vStart, const CVector3f& vDir, 
        const CVector3f& v0, const CVector3f& v1, const CVector3f& v2 )
    {
        CVector3f e1 = v1 - v0;
        CVector3f e2 = v2 - v0;
        CVector3f n = e1.Cross( e2 );
        n.NormalizeNoneZero();
        float d = n * v0;
        float a = e1 * e1;
        float b = e1 * e2;
        float c = e2 * e2;
        float D = a * c - b * b;
        float A = a / D;
        float B = b / D;
        float C = c / D;
        CVector3f Ub = C * e1 - B * e2;
        CVector3f Uy = A * e2 - B * e1;

        CPlane plane;
        plane.m_Normal = n;
        plane.m_Dist = -d;

        if ( !plane.IsRayIntersect( vStart, vDir, vIntersect ) )
        {
            return false;
        }

        CVector3f r = vIntersect - v0;
        float fBeta = Ub * r;
        if ( fBeta < 0 )
            return false;

        float fGamma = Uy * r;
        if ( fGamma < 0 )
            return false;

        float fAlpha = 1 - fBeta - fGamma;
        if ( fAlpha < 0 )
            return false;

        return true;
	}

	//========================================================================
	// 求整数平方根
	// 比sqrt快
	//=========================================================================
	uint16 sqrti( uint32 M )
	{
		if( M == 0 )                
			return 0;

		uint32 N        = 0;
		uint32 uTemp    = ( M >> 30 );					// 获取最高位：B[m-1]
		M               <<= 2;
		if( uTemp >= 1 )									// 最高位为1
		{
			N++;                                        // 结果当前位为1，否则为默认的0
			uTemp -= N;
		}

		for( uint32 i = 15; i > 0; i-- )				// 求剩余的15位
		{
			N            <<= 1;							// 左移一位
			uTemp        = ( uTemp << 2 ) + ( M >> 30 );
			uint32 ttp   = ( N<<1 ) + 1;
			M            <<= 2;

			if ( uTemp >= ttp )							// 假设成立
			{
				uTemp -= ttp;
				N ++;
			}
		}

		return (uint16)N;
	}

	GAMMA_COMMON_API uint32 sqrti64(uint64 M)
	{
		if (M == 0)
			return 0;

		uint64 N = 0;
		uint64 uTemp = (M >> 62);					// 获取最高位：B[m-1]
		M <<= 2;
		if (uTemp >= 1)									// 最高位为1
		{
			N++;                                        // 结果当前位为1，否则为默认的0
			uTemp -= N;
		}

		for (uint32 i = 31; i > 0; i--)				// 求剩余的15位
		{
			N <<= 1;							// 左移一位
			uTemp = (uTemp << 2) + (M >> 62);
			uint64 ttp = (N << 1) + 1;
			M <<= 2;

			if (uTemp >= ttp)							// 假设成立
			{
				uTemp -= ttp;
				N++;
			}
		}

		return (uint32)N;
	}

	//========================================================================
	// 不受浮点精度影响转换函数
	//========================================================================
	double u2d( uint32 uValue )
	{
#ifdef _WIN32
		double d;
		if( ((int32)uValue) >= 0 )
			d = (int)uValue;
		else
		{
			uint32* e = (uint32*)&d;
			e[1] = 0x41e00000 + ((uValue&0x7fffffff)>>11);
			e[0] = uValue<<21;
		}
		return d; 
#else
		return uValue;
#endif
	}

	//========================================================================
	// 极速浮点转整形函数
	// 比系统提供的快
	//=========================================================================
	int32 f2i( float f )
	{
		uint32 i = *(uint32*)&f;
		int32 e = ( (uint8)( i>>23 ) ) - 127 - 23;
		if( e < 0 )
			return (i&0x80000000) ? -(int32)(((i&0x7fffff)|0x800000)>>(-e)) : (int32)(((i&0x7fffff)|0x800000)>>(-e));
		return (i&0x80000000) ? -(int32)(((i&0x7fffff)|0x800000)<<e) : (int32)(((i&0x7fffff)|0x800000)<<e);
	}

	uint32 f2u( float f )
	{
		uint32 i = *(uint32*)&f;
		int32 e = ( (uint8)( i>>23 ) ) - 127 - 23;
		uint32 n = ( e < 0 ) ? (uint32)(((i&0x7fffff)|0x800000)>>(-e)) : (uint32)(((i&0x7fffff)|0x800000)<<e);
		return ( i&0x80000000 ) ? (uint32)( 0 - n ) : n;
	}
}
