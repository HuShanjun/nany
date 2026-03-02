//==============================================================
// TTinyNormal.h 
// 定义法线压缩算法
// 说明：	这里有两种压缩方法,两种压缩方法都对精度有所损失
//			压缩算法1的精度相对高些,但压缩和解压速度比较慢
//			压缩算法2的精度相对低些,但压缩和解压速度比较快
//			算法1解压的速度瓶颈在sin和cos的求值
//			算法2解压的速度瓶颈在sqrt
//			算法1的平均精度大约是算法2的十倍，最差精度是是算法2的两倍
//			对于CTinyNorm32:
//				算法1的平均误差大约是0.000021, 最大误差是0.00028
//				算法2的平均误差大约是0.000175, 最大误差是0.00045
//				算法2的解压速度大约是算法1的四倍
// 柯达昭
// 2007-09-11
//==============================================================

#ifndef __TINY_NORMAL_H__
#define __TINY_NORMAL_H__

#include "TVector3.h"
#include "GammaMath.h"

namespace Gamma
{
    template<int32 n>
    class TTinyNormal
    {
        enum EConstant
        { 
            eXSignedMask    = 1 << ( n - 1 ),
            eYSignedMask    = 1 << ( n - 2 ),

            //only for Compress1
			eLineCount      = TSqrt< 1 << ( n - 2 ) >::eValue,
            eEndLineLen     = ( eLineCount - 1 )*2 + 1,
            eIndexMask      = ( 1 << ( n - 2 ) ) - 1,

            //only for Compress2
            eZSignedMask    = 1 << ( n - 3 ),
            eMaxAxisMask    = 3 << ( n - 5 ),
            eMaxAxisIsX     = 0,
            eMaxAxisIsY     = 1 << ( n - 5 ),
            eMaxAxisIsZ     = 2 << ( n - 5 ),
            eFloatDefaul    = 0x800000,
            eFloatDBitCount = 24,
            eFloatEBitCount = 8,
            eFloatEDefault  = 0x3f800000,
            eFloatEBase     = ( 1 << ( eFloatEBitCount - 1 ) ) - 1,
            eXBitCount      = ( n - 5 )/2,
            eYBitCount      = n - 5 - eXBitCount,
            eXDelBitCount   = eFloatDBitCount - eXBitCount,
            eYDelBitCount   = eFloatDBitCount - eYBitCount,
            eXFloatMask     = ( ( 1 << eXBitCount ) - 1 ) << eXDelBitCount,
            eYFloatMask     = ( ( 1 << eYBitCount ) - 1 ) << eYDelBitCount,
            eXBitMask       = ( ( 1 << eXBitCount ) - 1 ) << eYBitCount,
            eYBitMask       = ( 1 << eYBitCount ) - 1,
            eXMaskShift     = eYBitCount > eXDelBitCount ? eYBitCount - eXDelBitCount : eXDelBitCount - eYBitCount,
        };

    public:

        //压缩Index排列
        //                         0
        //                      1  2  3
        //                   4  5  6  7  8
        //                9 10 11 12 13 14 15
        //            16 17 18 19 20 21 22 23 24
        //         25 26 27 28 29 30 31 32 33 34 35

        static uint32 Compress1( const CVector3f& vecNormal )
        {
            if( vecNormal.x == 0.0f && vecNormal.z == 0.0f )
                return vecNormal.y < 0 ? eYSignedMask : 0; 

            CVector3f vec		= vecNormal.Unit();							//矢量单位化
            const float fDel	= GM_PI_2/( eLineCount - 1 );				//纵向PI/2分段，每段角度大小
            float fYAngle		= acos( vec.y < 0 ? -vec.y : vec.y );					//计算Y方向角度
            float fXZ			= sqrt( vec.x*vec.x + vec.z*vec.z );        //计算XZ平面长度
            float fXZAngle		= acos( vec.z/fXZ );                        //计算XZ平面角度
            int32 nYaw			= (int32)( fYAngle/fDel + 0.5 );            //计算行号
            int32 nLineLen		= nYaw << 1;                                //计算当前行分割的段数
            int32 nPitch        = (int32)( fXZAngle*nLineLen/GM_PI + 0.5 );	//计算当前行位置
            int32 nIndex        = nYaw*nYaw + nPitch;						//合成Index

            return nIndex|( vecNormal.x < 0 ? eXSignedMask : 0 ) | ( vecNormal.y < 0 ? eYSignedMask : 0 );
        }

        static CVector3f Decompress1( uint32 uNormal )
        {
            int32 nIndex        = (int32)( uNormal&eIndexMask );            //取得Index
            if( nIndex == 0 )
                return CVector3f( 0.0f, eYSignedMask&uNormal ? -1.0f : 1.0f, 0.0f );

            const float fDel    = GM_PI_2/( eLineCount - 1 );				//纵向PI/2分段，每段角度大小
            int32 nYaw			= (int32)sqrt( (double)nIndex );					//取得行号
            int32 nLineLen		= nYaw << 1;                                //取得当前行分割的段数
            int32 nPitch        = nIndex - nYaw*nYaw;						//取得当前行位置
            float fYAngle		= nYaw*fDel;                                //计算Y方向角度
            float fXZAngle		= nPitch*GM_PI/nLineLen;                    //计算XZ平面角度
            float xz            = sin( fYAngle );

            return CVector3f( 
                eXSignedMask&uNormal ? -(float)( sin( fXZAngle )*xz )    : (float)( sin( fXZAngle )*xz ), 
                eYSignedMask&uNormal ? -(float)( cos( fYAngle ) )        : (float)( cos( fYAngle ) ), 
                (float)( cos( fXZAngle )*xz )
                );
        }

        static uint32 Compress2( const CVector3f& vecNormal )
        {
            if( vecNormal.x == 0.0f && vecNormal.z == 0.0f )
                return vecNormal.y < 0 ? eYSignedMask : 0; 
            CVector3f vec    = vecNormal.Unit();    

            uint32 uMaxAxis = eMaxAxisIsZ;
            CVector3f v		= vec;
			v.x				= v.x < 0 ? -v.x : v.x;
			v.y				= v.y < 0 ? -v.y : v.y;
			v.z				= v.z < 0 ? -v.z : v.z;			
            if( v.x >= v.y && v.x >= v.z )
            {
                vec.x		= vec.y;
                vec.y		= vec.z;
                uMaxAxis    = eMaxAxisIsX;
            }
            else if( v.y >= v.x && v.y >= v.z )
            {
                vec.y		= vec.z;
                uMaxAxis    = eMaxAxisIsY;
            }

            // 浮点格式
            // ＝ (-1)^s  * (1.d) * 2^(e - 127)
            struct _FLOAT  { uint32 d:23; uint32 e:8; uint32 s:1; };

            _FLOAT& fX		= *(_FLOAT*)&vec.x;
            uint32  nX		= *(uint32*)&vec.x ? ( ( ( fX.d|eFloatDefaul ) >> ( eFloatEBase - fX.e ) ) >> eXDelBitCount ) << eYBitCount : 0;
            _FLOAT& fY		= *(_FLOAT*)&vec.y;
            uint32  nY		= *(uint32*)&vec.y ? ( fY.d|eFloatDefaul ) >> ( eFloatEBase - fY.e + eYDelBitCount ) : 0;

            uint32  uIndex    = nX|nY|uMaxAxis;
            if( vecNormal.x < 0 )
                uIndex |= eXSignedMask;
            if( vecNormal.y < 0 )
                uIndex |= eYSignedMask;
            if( vecNormal.z < 0 )
                uIndex |= eZSignedMask;

            return uIndex;
        }

        static CVector3f Decompress2( uint32 uNormal )
        {
            if( !uNormal )
                return CVector3f( 0.0f, eYSignedMask&uNormal ? -1.0f : 1.0f, 0.0f );

            uint32 uXMask	= uNormal&eXBitMask;
            uint32 uYMask	= uNormal&eYBitMask;
            float fX        = 0;
            float fY        = 0;

            if( uXMask )
            {
                uint32 uDX	= eYBitCount > eXDelBitCount ? uXMask >> eXMaskShift : uXMask << eXMaskShift;
                uint32 uX	= uDX|eFloatEDefault;
                fX			= *(float*)&uX;
                if( uDX != eFloatDefaul )
                    fX       -= 1.0;
            }

            if( uYMask )
            {
                uint32 uDY	= uYMask << eYDelBitCount;
                uint32 uY	= uDY|eFloatEDefault;
                fY			= *(float*)&uY;
                if( uDY != eFloatDefaul )
                    fY		-= 1.0;
            }

            float fZ        = sqrt( 1 - fX*fX - fY*fY );

            if( ( eMaxAxisMask&uNormal ) == eMaxAxisIsX )
                return CVector3f( 
                eXSignedMask&uNormal ? -fZ : fZ,
                eYSignedMask&uNormal ? -fX : fX, 
                eZSignedMask&uNormal ? -fY : fY 
                );

            if( ( eMaxAxisMask&uNormal ) == eMaxAxisIsY )
                return CVector3f( 
                eXSignedMask&uNormal ? -fX : fX, 
                eYSignedMask&uNormal ? -fZ : fZ,
                eZSignedMask&uNormal ? -fY : fY 
                );

            return CVector3f( 
                eXSignedMask&uNormal ? -fX : fX, 
                eYSignedMask&uNormal ? -fY : fY, 
                eZSignedMask&uNormal ? -fZ : fZ
                );
        }
    };

	typedef TTinyNormal<8>  CTinyNorm08;
	typedef TTinyNormal<9>  CTinyNorm09;
	typedef TTinyNormal<10> CTinyNorm10;
	typedef TTinyNormal<11> CTinyNorm11;
	typedef TTinyNormal<12> CTinyNorm12;
	typedef TTinyNormal<13> CTinyNorm13;
	typedef TTinyNormal<14> CTinyNorm14;
	typedef TTinyNormal<15> CTinyNorm15;
	typedef TTinyNormal<16> CTinyNorm16;
	typedef TTinyNormal<17> CTinyNorm17;
	typedef TTinyNormal<18> CTinyNorm18;
	typedef TTinyNormal<19> CTinyNorm19;
	typedef TTinyNormal<20> CTinyNorm20;
	typedef TTinyNormal<21> CTinyNorm21;
	typedef TTinyNormal<22> CTinyNorm22;
	typedef TTinyNormal<23> CTinyNorm23;
	typedef TTinyNormal<24> CTinyNorm24;
	typedef TTinyNormal<25> CTinyNorm25;
	typedef TTinyNormal<26> CTinyNorm26;
	typedef TTinyNormal<27> CTinyNorm27;
	typedef TTinyNormal<28> CTinyNorm28;
	typedef TTinyNormal<29> CTinyNorm29;
	typedef TTinyNormal<30> CTinyNorm30;
	typedef TTinyNormal<31> CTinyNorm31;
	typedef TTinyNormal<32> CTinyNorm32;
};

#endif
