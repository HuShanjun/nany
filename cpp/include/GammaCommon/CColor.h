//=====================================================================
// CColor.h 
// 32位颜色结构及相关的各种运算
// 柯达昭
// 2007-09-06
//=======================================================================

#ifndef _GAMMA_COLOR_H_
#define _GAMMA_COLOR_H_

#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/GammaMath.h"
#include "GammaCommon/TVector4.h"

namespace Gamma
{
	//颜色结构
	struct CColor
	{
		union
		{
			uint32   dwColor;
			uint8    byColor[4];
		};

		CColor()
		{
			dwColor = 0;
		}

		CColor( uint32 c )
		{
			dwColor = c;
		}

		CColor( uint8 a, uint8 r, uint8 g, uint8 b )
		{
			byColor[0] = b;
			byColor[1] = g;
			byColor[2] = r;
			byColor[3] = a;
		}

		CColor( float a, float r, float g, float b )
		{
			byColor[0] = (uint8)( Limit( b*255.0f, 0.0f, 255.0f ) + 0.5f );
			byColor[1] = (uint8)( Limit( g*255.0f, 0.0f, 255.0f ) + 0.5f );
			byColor[2] = (uint8)( Limit( r*255.0f, 0.0f, 255.0f ) + 0.5f );
			byColor[3] = (uint8)( Limit( a*255.0f, 0.0f, 255.0f ) + 0.5f );
		}

		CColor( CVector4f vColor )
		{
			byColor[0] = (uint8)( Limit( vColor.z*255.0f, 0.0f, 255.0f ) + 0.5f );
			byColor[1] = (uint8)( Limit( vColor.y*255.0f, 0.0f, 255.0f ) + 0.5f );
			byColor[2] = (uint8)( Limit( vColor.x*255.0f, 0.0f, 255.0f ) + 0.5f );
			byColor[3] = (uint8)( Limit( vColor.w*255.0f, 0.0f, 255.0f ) + 0.5f );
		}

		uint8 operator[] ( size_t nIndex ) const
		{
			return byColor[nIndex];
		}

		uint8& operator[]( size_t nIndex )
		{
			return byColor[nIndex];
		}

		operator uint32() const
		{
			return dwColor;
		}

		operator CVector4f() const
		{
			return CVector4f( r(), g(), b(), a() )/255.0f;
		}

		operator TVector4<uint8>() const
		{
			return TVector4<uint8>( r(), g(), b(), a() );
		}

		const CColor& operator= ( const uint32& arg )
		{
			dwColor = arg;
			return *this;
		}

		const CColor& operator= ( const CColor& arg )
		{
			dwColor = arg.dwColor;
			return *this;
		}

		CColor FormatFrom565( uint16 nR5G6B5 )
		{
			byColor[0] = uint8( ( nR5G6B5&0x001f ) << 3 ); byColor[0] |= byColor[0] ? 0x7 : 0;
			byColor[1] = uint8( ( nR5G6B5&0x07e0 ) >> 3 ); byColor[1] |= byColor[1] ? 0x3 : 0;
			byColor[2] = uint8( ( nR5G6B5&0xf800 ) >> 8 ); byColor[2] |= byColor[2] ? 0x7 : 0;
			byColor[3] = 255;
			return *this;
		}

		uint16 FormatTo565() const
		{
			return (uint16)( 
				( uint32( byColor[2] >> 3 )<<11 ) | 
				( uint32( byColor[1] >> 2 )<<5 ) | 
				( uint32( byColor[0] >> 3 ) ) );
		}

		CColor FormatFrom4444( uint16 n4444 )
		{
			uint8* pBuf = (uint8*)&n4444;
			byColor[0] = ( pBuf[0]&0x0f ) << 4;			byColor[0] |= byColor[0] ? 0xf : 0;
			byColor[1] = pBuf[0]&0xf0;					byColor[1] |= byColor[1] ? 0xf : 0;
			byColor[2] = ( pBuf[1]&0x0f ) << 4;			byColor[2] |= byColor[2] ? 0xf : 0;
			byColor[3] = pBuf[1]&0xf0;					byColor[3] |= byColor[3] ? 0xf : 0;
			return *this;
		}

		uint16 FormatTo4444() const
		{
			if( !dwColor )
				return 0;
			return (uint16)( 
				( uint32( byColor[3] >> 4 ) << 12 ) | 
				( uint32( byColor[2] >> 4 ) << 8 ) | 
				( uint32( byColor[1] >> 4 ) << 4 ) | 
				( uint32( byColor[0] >> 4 ) ) );
		}

		CColor Interpolate( const CColor& c, float fPercent ) const
		{
			int32& i = *(int32*)&fPercent;
			if( i <= 0 )
				return c;
			int32 e = 127 + 23 - 8 - ( i>>23 );
			if( e >= 24 )
				return c;
			int32 nPercent1 = (int32)( ( (i&0x7fffff)|0x800000 ) >> e );
			int32 nPercent2 = 256 - nPercent1;

			if( nPercent2 < 2 )
				return *this;

			uint16 nColor1[4] = {   byColor[0],   byColor[1],   byColor[2],   byColor[3] };
			uint16 nColor2[4] = { c.byColor[0], c.byColor[1], c.byColor[2], c.byColor[3] };
			uint64 nColor     = ( ( *(uint64*)( nColor1 ) )*nPercent1 + ( *(uint64*)( nColor2 ) )*nPercent2 ) >> 8;
			uint16* pColor    = (uint16*)&nColor;
			return CColor( (uint8)pColor[3], (uint8)pColor[2], (uint8)pColor[1], (uint8)pColor[0] );
		}

		const CColor operator* ( float arg ) const
		{
			int32& i = *(int32*)&arg;
			if( i <= 0 )
				return 0;
			int32 e = 127 + 23 - 9 - ( i>>23 );
			if( e >= 24 )
				return 0;
			int32 nPercent = (int32)( ( (i&0x7fffff)|0x800000 ) >> e );
			nPercent = ( nPercent >> 1 ) + ( nPercent&1 );

			if( nPercent == 0 )
				return 0;
			if( nPercent >= 255 )
				return *this;

			return
				( ( ( ( dwColor&0x00ff00ff )*nPercent ) >> 8 )&0x00ff00ff ) | 
				( ( ( ( dwColor&0xff00ff00 ) >> 8 )*nPercent )&0xff00ff00 );
		}

		const CColor operator* (const CColor& arg) const
		{
			return CColor( (uint8)(byColor[3]* arg.byColor[3]/255.0f), (uint8)(byColor[2]* arg.byColor[2]/255.0f),
				(uint8)(byColor[1]* arg.byColor[1]/255.0f), (uint8)(byColor[0]* arg.byColor[0]/255.0f) );
		};

		const CColor operator+ ( const CColor& arg ) const
		{
			return CColor( (uint8)Min( 255, (int)byColor[3] + arg.byColor[3] ), (uint8)Min( 255, (int)byColor[2] + arg.byColor[2] ),
				(uint8)Min( 255, (int)byColor[1] + arg.byColor[1] ), (uint8)Min( 255, (int)byColor[0] + arg.byColor[0] ) );
		}

		const CColor operator- ( const CColor& arg ) const
		{
			return CColor( (uint8)Max( 0, (int)byColor[3] - arg.byColor[3] ), (uint8)Max( 0, (int)byColor[2] - arg.byColor[2] ),
				(uint8)Max( 0, (int)byColor[1] - arg.byColor[1] ), (uint8)Max( 0, (int)byColor[0] - arg.byColor[0] ) );
		}

		const CColor operator! () const
		{
			return CColor( byColor[3], byColor[0], byColor[1], byColor[2] );
		}

		uint8 a() const
		{
			return byColor[3];
		}

		uint8 r() const
		{
			return byColor[2];
		}

		uint8 g() const
		{
			return byColor[1];
		}

		uint8 b() const
		{
			return byColor[0];
		}

		uint8& a()
		{
			return byColor[3];
		}

		uint8& r()
		{
			return byColor[2];
		}

		uint8& g()
		{
			return byColor[1];
		}

		uint8& b()
		{
			return byColor[0];
		}

		CColor SwapRB()
		{
			return CColor( a(), b(), g(), r() );
		}

		float GetBrightness()
		{
			return ( r()*0.30f + g()*0.59f + b()*0.11f )/255.0f;
		}

		//* RGB 转换为 HSL 色彩空间算法，
		void RGB2HSL( float &H,float &S,float &L)
		{
			float fR, fG, fB, fMax, fMin;
			float fDelR, fDelG, fDelB, fDelMax;
			fR = (float)( r() / 255.0 );       //Where RGB values = 0 ÷ 255
			fG = (float)( g() / 255.0 );
			fB = (float)( b() / 255.0 );

			fMin = Min(fR, Min(fG, fB));    //Min. value of RGB
			fMax = Max(fR, Max(fG, fB));    //Max. value of RGB
			fDelMax = fMax - fMin;        //Delta RGB value

			L = (float)( (fMax + fMin) / 2.0 );

			if( fDelMax == 0 )           //This is a gray, no chroma...
			{
				//H = 2.0/3.0;          //Windows下S值为0时，H值始终为160（2/3*240）
				H = 0;                  //HSL results = 0 ÷ 1
				S = 0;
			}
			else                        //Chromatic data...
			{
				if (L < 0.5) S = fDelMax / (fMax + fMin);
				else         S = fDelMax / (2 - fMax - fMin);

				fDelR = (float)(  (((fMax - fR) / 6.0) + (fDelMax / 2.0)) / fDelMax );
				fDelG = (float)(  (((fMax - fG) / 6.0) + (fDelMax / 2.0)) / fDelMax );
				fDelB = (float)(  (((fMax - fB) / 6.0) + (fDelMax / 2.0)) / fDelMax );

				if      (fR == fMax) H = fDelB - fDelG;
				else if (fG == fMax) H = (float)(  (1.0 / 3.0) + fDelR - fDelB );
				else if (fB == fMax) H = (float)(  (2.0 / 3.0) + fDelG - fDelR );

				if (H < 0)  H += 1;
				if (H > 1)  H -= 1;
			}
		}

		//* HSL 转换为 RGB色彩空间算法

		CColor HSL2RGB(float H,float S,float L)
		{
			struct _
			{
				static float Hue2RGB(float v1, float v2, float vH) 
				{ 
					if (vH < 0) vH += 1; 
					if (vH > 1) vH -= 1; 
					if (6.0f * vH < 1) return v1 + (v2 - v1) * 6.0f * vH; 
					if (2.0f * vH < 1) return v2; 
					if (3.0f * vH < 2) return v1 + (v2 - v1) * ((2.0f / 3.0f) - vH) * 6.0f; 
					return (v1); 
				} 
			}; 

			float R,G,B;
			float var_1, var_2;
			if (S == 0)                       //HSL values = 0 ÷ 1
			{
				R = L * 255.0f;                   //RGB results = 0 ÷ 255
				G = L * 255.0f;
				B = L * 255.0f;
			}
			else
			{
				if (L < 0.5f) var_2 = L * (1 + S);
				else         var_2 = (L + S) - (S * L);

				var_1 = 2.0f * L - var_2;

				R = (float)( 255.0f * _::Hue2RGB(var_1, var_2, H + (1.0f / 3.0f)) );
				G = (float)( 255.0f * _::Hue2RGB(var_1, var_2, H) );
				B = (float)( 255.0f * _::Hue2RGB(var_1, var_2, H - (1.0f / 3.0f)) );
			}

			byColor[0] = (uint8)B;
			byColor[1] = (uint8)G;
			byColor[2] = (uint8)R;

			return *this;
		}

		static CColor FormatFrom565Fast( uint16 nR5G6B5 )
		{
			return 0xff000000|( ( nR5G6B5&0xf800 ) << 8 )|( ( nR5G6B5&0x07e0 ) << 5 )|( ( nR5G6B5&0x001f ) << 3 );
		}

		static CVector3f BuildHueTransform( float fHue /*[-1, 1]*/ )
		{
			float fRot = fHue*GM_PI;
			float fSin = sinf( fRot );
			float fCos = cosf( fRot );
			float fCoff = ( 1 - fCos ) * 0.333333f/*0.57735*0.0.57735*/;
			CVector3f vHue;
			vHue.x = fCoff + fCos;
			vHue.y = fCoff + 0.57735f*fSin;
			vHue.z = fCoff - 0.57735f*fSin;
			return vHue;
		}

		static CMatrix BuildColorTransform( CVector3f vHue, float fSat /*[0, 1]*/ )
		{
			float fGray  = 1 - fSat;
			float fGrayR = 0.30f*fGray;
			float fGrayG = 0.59f*fGray;
			float fGrayB = 0.11f*fGray;

			CVector3f vSatTransform0 = CVector3f( fSat + fGrayR, fGrayR, fGrayR );
			CVector3f vSatTransform1 = CVector3f( fGrayG, fSat + fGrayG, fGrayG );
			CVector3f vSatTransform2 = CVector3f( fGrayB, fGrayB, fSat + fGrayB );

			CMatrix matColor;
			matColor._11 = vSatTransform0.Dot( vHue.xyz() );
			matColor._21 = vSatTransform1.Dot( vHue.xyz() );
			matColor._31 = vSatTransform2.Dot( vHue.xyz() );

			matColor._12 = vSatTransform0.Dot( vHue.zxy() );
			matColor._22 = vSatTransform1.Dot( vHue.zxy() );
			matColor._32 = vSatTransform2.Dot( vHue.zxy() );

			matColor._13 = vSatTransform0.Dot( vHue.yzx() );
			matColor._23 = vSatTransform1.Dot( vHue.yzx() );
			matColor._33 = vSatTransform2.Dot( vHue.yzx() );
			return matColor;
		}

		static CMatrix BuildColorTransform( float fHue /*[-1, 1]*/, float fSat /*[0, 1]*/ )
		{
			return BuildColorTransform( BuildHueTransform( fHue ), fSat );
		}
	};
};

#endif
