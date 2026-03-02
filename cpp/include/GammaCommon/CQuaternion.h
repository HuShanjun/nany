//==============================================================================
//! \file       CQuaternion.h
//  \brief      四元数类的定义
//
//                定义各种可能的构造四元数的方法,和相关的四元数操作
//  \author        柯达昭
//  \version    1.0
//  \date        04-8-2 11:42
//  \todo        还没有做完整的测试及代码的优化
//==============================================================================

#ifndef __GAMMA_MATH_QUATERNION__
#define __GAMMA_MATH_QUATERNION__

#include "GammaCommon/CMatrix.h"
#include "GammaCommon/TVector4.h"
#include "GammaCommon/GammaMath.h"

namespace Gamma
{
    class CQuaternion : public CVector4f
    {
    public:

        //constructor
		CQuaternion() : CVector4f( 0, 0, 0, 1 ) {}
		CQuaternion( const float* pfv ){ x = pfv[0]; y = pfv[1]; z = pfv[2]; w = pfv[3]; }
		CQuaternion( const CVector4f& v ) : CVector4f( v ){};
        CQuaternion( float x, float y, float z, float w ) : CVector4f( x, y, z, w ){};
        CQuaternion( const CMatrix& matOrg, CVector3f* pScale = NULL );
		
        // assignment operators
        CQuaternion& operator += ( const CQuaternion& );
        CQuaternion& operator -= ( const CQuaternion& );
        CQuaternion& operator *= ( const CQuaternion& );
        CQuaternion& operator *= ( float );
        CQuaternion& operator /= ( float );

        // unary operators
        CQuaternion  operator + () const;
        CQuaternion  operator - () const;

        // binary operators
        CQuaternion operator + ( const CQuaternion& ) const;
        CQuaternion operator - ( const CQuaternion& ) const;
        CQuaternion operator * ( const CQuaternion& ) const;
        CQuaternion operator * ( float ) const;
        CQuaternion operator / ( float ) const;

        friend CQuaternion operator * (float, const CQuaternion& );

        bool operator == ( const CQuaternion& ) const;
        bool operator != ( const CQuaternion& ) const;

        operator CMatrix() const;
        const CQuaternion& operator = ( const CMatrix& mat );

		CQuaternion Slerp( float, const CQuaternion& ) const;
	    const CQuaternion& Identity();
		bool IsIdentity() const;
		void FromEulerAngles( const CVector3f& vEuler );
		CVector3f ToEulerAngles();
    };

    /**
    * 功能：    从矩阵获取四元数
    * 描述：    只能正确分解scale*rotate方式的矩阵
	* @param	matOrg 传入的矩阵
	* @param	pScale 返回矩阵的缩放系数
    */
    inline CQuaternion::CQuaternion( const CMatrix& matOrg, CVector3f* pScale )
	{
		// Normalize axises
		CVector3f vXAxis = matOrg.GetAxis( 0 );
		CVector3f vYAxis = matOrg.GetAxis( 1 );
		CVector3f vZAxis = matOrg.GetAxis( 2 );
		float fScaleX = vXAxis.Len();
		float fScaleY = vYAxis.Len();
		float fScaleZ = vZAxis.Len();

		if( pScale )
			*pScale = CVector3f( fScaleX, fScaleY, fScaleZ );

		if( fScaleX < GM_MATH_EPSILON || 
			fScaleY < GM_MATH_EPSILON || 
			fScaleZ < GM_MATH_EPSILON )
		{
			x = y = z = 0;
			w = 1;
			return;
		}

		vXAxis *= 1.0f/fScaleX;
		vYAxis *= 1.0f/fScaleY;
		vZAxis *= 1.0f/fScaleZ;

		// Use tq to store the largest trace
		float tq[4];
		tq[0] = 1 + vXAxis[0] + vYAxis[1] + vZAxis[2];
		tq[1] = 1 + vXAxis[0] - vYAxis[1] - vZAxis[2];
		tq[2] = 1 - vXAxis[0] + vYAxis[1] - vZAxis[2];
		tq[3] = 1 - vXAxis[0] - vYAxis[1] + vZAxis[2];

		// Find the maximum (could also use stacked if's later)
		int32 j = 0;
		for( int32 i = 1; i < 4; i++ ) 
			j = ( tq[i] > tq[j] ) ? i : j;

		// check the diagonal
		if( j == 0 )
		{
			/* perform instant calculation */
			w = tq[0];
			x = vYAxis[2] - vZAxis[1];
			y = vZAxis[0] - vXAxis[2];
			z = vXAxis[1] - vYAxis[0];
		}
		else if( j == 1 )
		{
			w = vYAxis[2] - vZAxis[1];
			x = tq[1];
			y = vXAxis[1] + vYAxis[0];
			z = vZAxis[0] + vXAxis[2];
		}
		else if( j == 2 )
		{
			w = vZAxis[0] - vXAxis[2];
			x = vXAxis[1] + vYAxis[0];
			y = tq[2];
			z = vYAxis[2] + vZAxis[1];
		}
		else /* if( j == 3 ) */
		{
			w = vXAxis[1] - vYAxis[0];
			x = vZAxis[0] + vXAxis[2];
			y = vYAxis[2] + vZAxis[1];
			z = tq[3];
		}

		*this *= (float)sqrt( 0.25/tq[j] );
    }

    // assignment operators
    inline CQuaternion& CQuaternion::operator += ( const CQuaternion& Quat )
    {
        x += Quat.x;
        y += Quat.y;
        z += Quat.z;
        w += Quat.w;

        return *this;
    }

    inline CQuaternion& CQuaternion::operator -= ( const CQuaternion& Quat )
    {
        x -= Quat.x;
        y -= Quat.y;
        z -= Quat.z;
        w -= Quat.w;

        return *this;
    }

    inline CQuaternion& CQuaternion::operator *= ( const CQuaternion& Quat )
    {
        float tx = Quat.x*w + x*Quat.w + y*Quat.z - z*Quat.y;
        float ty = Quat.y*w + y*Quat.w + z*Quat.x - x*Quat.z;
        float tz = Quat.z*w + z*Quat.w + x*Quat.y - y*Quat.x;

        w = Quat.w*w - x*Quat.x - y*Quat.y - z*Quat.z;
        x = tx;
        y = ty;
        z = tz;

        return *this;
    }

    inline CQuaternion& CQuaternion::operator *= ( float fv )
    {
        x *= fv;
        y *= fv;
        z *= fv;
        w *= fv;

        return *this;
    }

    inline CQuaternion& CQuaternion::operator /= ( float fv )
    {
        x /= fv;
        y /= fv;
        z /= fv;
        w /= fv;

        return *this;
    }

    // unary operators
    inline CQuaternion  CQuaternion::operator + () const
    {
        return *this;
    }

    inline CQuaternion  CQuaternion::operator - () const
    {
        return CQuaternion( -x, -y, -z, -w );
    }

    // binary operators
    inline CQuaternion CQuaternion::operator + ( const CQuaternion& Quat ) const
    {
        return CQuaternion( x + Quat.x, y + Quat.y, z + Quat.z, w + Quat.w );
    }

    inline CQuaternion CQuaternion::operator - ( const CQuaternion& Quat ) const
    {
        return CQuaternion( x - Quat.x, y - Quat.y, z - Quat.z, w - Quat.w );
    }

    inline CQuaternion CQuaternion::operator * ( const CQuaternion& Quat ) const
    {
        return CQuaternion( Quat.x*w + x*Quat.w + y*Quat.z - z*Quat.y, 
            Quat.y*w + y*Quat.w + z*Quat.x - x*Quat.z, 
            Quat.z*w + z*Quat.w + x*Quat.y - y*Quat.x, 
            Quat.w*w - x*Quat.x - y*Quat.y - z*Quat.z );
    }

    inline CQuaternion CQuaternion::operator * ( float fv ) const
    {
        return CQuaternion( x*fv, y*fv, z*fv, w*fv );
    }

    inline CQuaternion CQuaternion::operator / ( float fv ) const
    {
        return CQuaternion( x/fv, y/fv, z/fv, w/fv );
    }


    inline CQuaternion operator * ( float fv, const CQuaternion& Quat )
    {
        return CQuaternion( Quat.x/fv, Quat.y/fv, Quat.z/fv, Quat.w/fv );
    }


    inline bool CQuaternion::operator == ( const CQuaternion& Quat ) const
    {
        return x == Quat.x && y == Quat.y && z == Quat.z && w == Quat.w;
    }

    inline bool CQuaternion::operator != ( const CQuaternion& Quat ) const
    {
        return x != Quat.x || y != Quat.y || z != Quat.z || w != Quat.w;
    }

    inline CQuaternion::operator CMatrix() const
    {
        float x2 = x + x;
        float y2 = y + y;
        float z2 = z + z;
        float xx = x * x2;
        float xy = x * y2;
        float xz = x * z2;
        float yy = y * y2;
        float yz = y * z2;
        float zz = z * z2;
        float wx = w * x2;
        float wy = w * y2;
        float wz = w * z2;
		float ls = LenSqr();

        return CMatrix(
            ls - ( yy + zz ),	xy + wz,            xz - wy,            0,
            xy - wz,            ls - ( xx + zz ),	yz + wx,            0,
            xz + wy,            yz - wx,            ls - ( xx + yy ), 0,
            0,                  0,                  0,                  1 );
    }

    inline const CQuaternion& CQuaternion::operator = ( const CMatrix& mat )
    {
        return *this = CQuaternion( mat );
    }

    inline CQuaternion CQuaternion::Slerp( float s, const CQuaternion& Quat ) const
    {
        CQuaternion Out;

        // calc cosine
        double cosom = x*Quat.x + y*Quat.y + z*Quat.z + w*Quat.w;
        // adjust signs (if necessary)
        if ( cosom < 0.0 )
        { 
            cosom = -cosom; 
            Out = -Quat;
        } 
        else
            Out = Quat;

        double scale0 = 1.0 - s;
        double scale1 = s;
        // calculate coefficients
        if ( 1.0 - cosom > 0.00001f ) 
        {
            // standard case (slerp)
            double omega = acos(cosom);
            double sinom = sin(omega);
            scale0 = sin( ( 1.0 - s ) * omega) / sinom;
            scale1 = sin( s * omega ) / sinom;
        } 

        // calculate final values
        Out.x = (float)( scale0 * x + scale1 * Out.x );
        Out.y = (float)( scale0 * y + scale1 * Out.y );
        Out.z = (float)( scale0 * z + scale1 * Out.z );
        Out.w = (float)( scale0 * w + scale1 * Out.w );

        return Out;
	}

    inline const CQuaternion& CQuaternion::Identity()
    {
        x = y = z = 0.0f;
        w = 1.0f;
        return *this;
    }

    inline bool CQuaternion::IsIdentity() const
    {
        return x == 0.0f && y == 0.0f && z == 0.0f && w == 1.0f;
	}

	/**
	* Fills the quaternion object with values representing the given euler rotation.
	*
	* @param	vEuler	The angle in radians of the rotation.
	*/
	inline void CQuaternion::FromEulerAngles( const CVector3f& vEuler )
	{
		CVector3f vHalf = vEuler*0.5f;
		float cosX = cosf( vHalf.x ), sinX = sinf( vHalf.x );
		float cosY = cosf( vHalf.y ), sinY = sinf( vHalf.y );
		float cosZ = cosf( vHalf.z ), sinZ = sinf( vHalf.z );

		w = cosX*cosY*cosZ + sinX*sinY*sinZ;
		x = sinX*cosY*cosZ - cosX*sinY*sinZ;
		y = cosX*sinY*cosZ + sinX*cosY*sinZ;
		z = cosX*cosY*sinZ - sinX*sinY*cosZ;
	}

	/**
	* Fills a target Vector3D object with the Euler angles that form the rotation represented by this quaternion.
	* @param target An optional Vector3D object to contain the Euler angles. If not provided, a new object is created.
	* @return The Vector3D containing the Euler angles.
	*/
	inline CVector3f CQuaternion::ToEulerAngles()
	{
		CVector3f target;
		target.x = atan2(2 * (w * x + y * z), 1 - 2 * (x * x + y * y));
		target.y = asin(2 * (w * y - z * x));
		target.z = atan2(2 * (w * z + x * y), 1 - 2 * (y * y + z * z));
		return target;
	}
};

#endif
