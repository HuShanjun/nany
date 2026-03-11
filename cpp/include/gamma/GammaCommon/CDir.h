/*
*	2D的方向，0~255对应着0~2PI,y轴的正方向值为0，负方向为128，
    x的正方向为64，负方向为192
*/

#ifndef _GAMMA_DIR_H_
#define _GAMMA_DIR_H_

#include "GammaCommon/TVector2.h"
#include "GammaCommon/GammaMath.h"

namespace Gamma
{
    class CDir 
    {
    public:
        uint8_t	uDir;

        CDir( uint8_t nDir = 0 ) : uDir(nDir){}
        CDir( const CVector2f& vecDir ){ Set( vecDir ); }

        void Get( CVector2f& vecDir )const	{ vecDir = CVector2f(*this); }
        void Set( const CVector2f& vecDir )	{ *this = vecDir; }

        //得到方向
        operator CVector2f() const
        {
			float fAng = GetRadian();
            return CVector2f( sinf( fAng ), cosf( fAng ) );
        }

		float GetRadian() const
		{
			return (float)( uDir* GM_PI *2/256 );
		}

        const CDir& operator = ( const CVector2f& vecDir )
		{
			float fLen = vecDir.Len();
            if( fLen == 0 )
			{
                uDir = 0;
				return *this;
			}

			float fDir = acosf( vecDir.y/fLen )*256/( GM_PI * 2 );
			if( vecDir.x < 0 )
				fDir = 256.0f - fDir;
			uDir = (uint8_t)( fDir + 0.5f );            
			return *this;
        }

		operator uint8_t() const { return uDir; }
    };

	template<class T>
	inline TVector2<T>::TVector2( const CDir& rhs )
	{
		float fAng = (float)( rhs.uDir* GM_PI *2.0f/256.0f );
		x = sinf( fAng );
		y = cosf( fAng );
	}

	template<class T> 
	inline uint8_t TVector2<T>::GetDir() const
	{
		return CDir(*this).uDir;
	}
}// end of namespace Gamma

#endif

