//=====================================================================
// TPolar2.h 
// 2D矢量模板及相关的各种运算, CVector3f代表的是单精度浮点矢量，常用类型
// 柯达昭
// 2007-09-06
//=======================================================================

#ifndef _GAMMA_POLAR2_H_
#define _GAMMA_POLAR2_H_

#pragma warning(disable: 4201)
#include <math.h>
#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/CDir.h"

namespace Gamma
{
	class CDir;
    template<class T> struct TPolar2
    {
        union 
        {
            struct 
            {
                T    r, a; // 2 real components of the vector
            };

            T    v[2]; // Array access useful in loops
        };

		typedef T Elem;

        // =========================================================================
        // 构造函数
		//==========================================================================
		TPolar2( const T r, const T a ) : r(r), a(a) {}
		TPolar2( const T f[2]){ r[0] = f[0]; a[1] = f[1]; }
		TPolar2() : r(), a(){}
		TPolar2( const CDir& rhs ) : r( 1 ) { a = rhs.uDir*GM_2PI; }
		TPolar2( const CVector2f& rhs ) : r( rhs.Len() ) { SetAngle( rhs ); }
        
        template<class Another>
		explicit TPolar2( const TPolar2<Another>& v) : r((T)v.r), a((T)v.a) {}

		template<class Another>
		Another To() const{ return Another( (typename Another::Elem)r, (typename Another::Elem)a ); }

		operator TVector2<T>() const
		{
			return CVector2f( sinf( a )*r, cosf( a )*r );
		}

		CDir GetDir() const
		{
			int32 nDir = (int32)( 256*a/GM_2PI + 0.5 );
			return CDir( (uint8)nDir );
		}

		void SetAngle( CVector2f vecDir )
		{
			float fLen = vecDir.Len();
			if( fLen < 0.00001f )
				a = 0;
			else
			{
				vecDir = vecDir/fLen;
				a = vecDir.x < 0 ? -acosf( vecDir.y ) : acosf( vecDir.y );
			}
		}

		// =====================================================================
		// 操作符重载
		// =====================================================================
		const TPolar2 operator + (const TPolar2& b) const;
		const TPolar2 operator - (const TPolar2& b) const;
		const TPolar2 operator - () const;

		const TPolar2& operator = ( const TPolar2& b );
		const TPolar2& operator += (const TPolar2& b); 
		const TPolar2& operator -= (const TPolar2& b) ;

		const TPolar2 operator * (const T s) const;
		const TPolar2 operator / (const T s) const;
		const TPolar2 operator *= (const T s) const;
		const TPolar2 operator /= (const T s) const;

		bool operator == ( const TPolar2& b ) const;
		bool operator != ( const TPolar2& b ) const;
    };

	template<class T>
	inline bool TPolar2<T>::operator != ( const TPolar2& b ) const
	{
		return r != b.r || a != b.a;
	}

	template<class T>
	inline bool TPolar2<T>::operator==( const TPolar2& b ) const
	{
		return r == b.r && a == b.a;
	}

	template<class T>
	inline const TPolar2<T>& TPolar2<T>::operator -= ( const TPolar2& b )
	{
		r -= b.r;
		a -= b.a;
		return this; 
	}

	template<class T>
	inline const TPolar2<T>& TPolar2<T>::operator += ( const TPolar2& b )
	{
		r += b.r;
		a += b.a;
		return this; 
	}

	template<class T>
	inline const TPolar2<T>& TPolar2<T>::operator = ( const TPolar2& b )
	{
		r = b.r;
		a = b.a;
		return *this; 
	}

	template<class T>
	inline const TPolar2<T> TPolar2<T>::operator - () const
	{
		return TPolar2( -r, -a );
	}

	template<class T>
	inline const TPolar2<T> TPolar2<T>::operator - ( const TPolar2& b ) const
	{
		return TPolar2( r - b.r, a - b.a );
	}

	template<class T>
	inline const TPolar2<T> TPolar2<T>::operator + ( const TPolar2& b ) const
	{
		return TPolar2( r + b.r, a + b.a );
	}

	template<class T>
	const TPolar2<T> TPolar2<T>::operator /= ( const T s ) const
	{
		r /= s;
		a /= s;
		return *this;
	}

	template<class T>
	const TPolar2<T> TPolar2<T>::operator *= ( const T s ) const
	{
		r *= s;
		a *= s;
		return *this;
	}

	template<class T>
	const TPolar2<T> TPolar2<T>::operator / ( const T s ) const
	{
		return TPolar2( r / s, a / s );
	}

	template<class T>
	const TPolar2<T> TPolar2<T>::operator * ( const T s ) const
	{
		return TPolar2( r * s, a * s );
	}

	typedef TPolar2<float> CPolar2f;
}

#endif
