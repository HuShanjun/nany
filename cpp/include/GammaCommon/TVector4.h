//=====================================================================
// TVector4.h 
// 4D矢量模板及相关的各种运算, CVector3f代表的是单精度浮点矢量，常用类型
// 柯达昭
// 2008-05-24
//=======================================================================

#ifndef __GAMMA_VECTOR4_H__
#define __GAMMA_VECTOR4_H__

#include "GammaCommon/TVector3.h"
#pragma warning(disable: 4201)

namespace Gamma
{
    class CMatrix;
    template<class T> struct TVector4
    {
        union 
        {
            // This structure here is nameless, which really isn't 
            // legal C++... most compilers will deal with it,however.
            struct 
            {
                T x,y,z,w; // 3 real components of the vector
            };
            T    v[4]; // Array access useful in loops
		};

		enum 
		{
			eAxisW = 3,
		};

		typedef T Elem;

        // =========================================================================
        // 构造函数  
        //==========================================================================
        TVector4() : x(), y(), z(), w() {}
        TVector4( const T x, const T y, const T z, const T w ) : x(x), y(y), z(z), w(w) {}
		TVector4( const TVector4& v ) : x(v.x), y(v.y), z(v.z), w(v.w) {}

		TVector4( const TVector3<T>& v, T w ) : x(v.x), y(v.y), z(v.z), w(w) {}
		TVector4( T x, const TVector3<T>& v ) : x(x), y(v.x), z(v.y), w(v.z) {}

		TVector4( const TVector2<T>& v1, const TVector2<T>& v2 ) : x(v1.x), y(v1.y), z(v2.x), w(v2.y) {}
		TVector4( const TVector2<T>& v, T z, T w ) : x(v.x), y(v.y), z(z), w(w) {}
		TVector4( T x, const TVector2<T>& v, T w ) : x(x), y(v.x), z(v.y), w(w) {}
		TVector4( T x, T y, const TVector2<T>& v ) : x(x), y(y), z(v.x), w(v.y) {}

		TVector4( const T f[4]){ v[0] = f[0]; v[1] = f[1]; v[2] = f[2]; v[3] = f[3]; }

		TVector2<T> xy()const{ return TVector2<T>( x, y ); }
		TVector2<T> xz()const{ return TVector2<T>( x, z ); }
		TVector2<T> xw()const{ return TVector2<T>( x, w ); }
		TVector2<T> yx()const{ return TVector2<T>( y, x ); }
		TVector2<T> yz()const{ return TVector2<T>( y, z ); }
		TVector2<T> yw()const{ return TVector2<T>( y, w ); }
		TVector2<T> zx()const{ return TVector2<T>( z, x ); }
		TVector2<T> zy()const{ return TVector2<T>( z, y ); }
		TVector2<T> zw()const{ return TVector2<T>( z, w ); }
		TVector2<T> wx()const{ return TVector2<T>( w, x ); }
		TVector2<T> wy()const{ return TVector2<T>( w, y ); }
		TVector2<T> wz()const{ return TVector2<T>( w, z ); }

		TVector3<T> xyz()const{ return TVector3<T>( x, y, z ); }
		TVector3<T> xyw()const{ return TVector3<T>( x, y, w ); }
		TVector3<T> xzy()const{ return TVector3<T>( x, z, y ); }
		TVector3<T> xzw()const{ return TVector3<T>( x, z, w ); }
		TVector3<T> xwy()const{ return TVector3<T>( x, w, y ); }
		TVector3<T> xwz()const{ return TVector3<T>( x, w, z ); }
		TVector3<T> yxz()const{ return TVector3<T>( y, x, z ); }
		TVector3<T> yxw()const{ return TVector3<T>( y, x, w ); }
		TVector3<T> yzx()const{ return TVector3<T>( y, z, x ); }
		TVector3<T> yzw()const{ return TVector3<T>( y, z, w ); }
		TVector3<T> ywx()const{ return TVector3<T>( y, w, x ); }
		TVector3<T> ywz()const{ return TVector3<T>( y, w, z ); }
		TVector3<T> zxy()const{ return TVector3<T>( z, x, y ); }
		TVector3<T> zxw()const{ return TVector3<T>( z, x, w ); }
		TVector3<T> zyx()const{ return TVector3<T>( z, y, x ); }
		TVector3<T> zyw()const{ return TVector3<T>( z, y, w ); }
		TVector3<T> zwx()const{ return TVector3<T>( z, w, x ); }
		TVector3<T> zwy()const{ return TVector3<T>( z, w, y ); }
		TVector3<T> wxy()const{ return TVector3<T>( w, x, y ); }
		TVector3<T> wxz()const{ return TVector3<T>( w, x, z ); }
		TVector3<T> wyx()const{ return TVector3<T>( w, y, x ); }
		TVector3<T> wyz()const{ return TVector3<T>( w, y, z ); }
		TVector3<T> wzx()const{ return TVector3<T>( w, z, x ); }
		TVector3<T> wzy()const{ return TVector3<T>( w, z, y ); }

		TVector4<T> xyzw()const{ return TVector4<T>( x, y, z, w ); }
		TVector4<T> xywz()const{ return TVector4<T>( x, y, w, z ); }
		TVector4<T> xzyw()const{ return TVector4<T>( x, z, y, w ); }
		TVector4<T> xzwy()const{ return TVector4<T>( x, z, w, y ); }
		TVector4<T> xwyz()const{ return TVector4<T>( x, w, y, z ); }
		TVector4<T> xwzy()const{ return TVector4<T>( x, w, z, y ); }
		TVector4<T> yxzw()const{ return TVector4<T>( y, x, z, w ); }
		TVector4<T> yxwz()const{ return TVector4<T>( y, x, w, z ); }
		TVector4<T> yzxw()const{ return TVector4<T>( y, z, x, w ); }
		TVector4<T> yzwx()const{ return TVector4<T>( y, z, w, x ); }
		TVector4<T> ywxz()const{ return TVector4<T>( y, w, x, z ); }
		TVector4<T> ywzx()const{ return TVector4<T>( y, w, z, x ); }
		TVector4<T> zxyw()const{ return TVector4<T>( z, x, y, w ); }
		TVector4<T> zxwy()const{ return TVector4<T>( z, x, w, y ); }
		TVector4<T> zyxw()const{ return TVector4<T>( z, y, x, w ); }
		TVector4<T> zywx()const{ return TVector4<T>( z, y, w, x ); }
		TVector4<T> zwxy()const{ return TVector4<T>( z, w, x, y ); }
		TVector4<T> zwyx()const{ return TVector4<T>( z, w, y, x ); }
		TVector4<T> wxyz()const{ return TVector4<T>( w, x, y, z ); }
		TVector4<T> wxzy()const{ return TVector4<T>( w, x, z, y ); }
		TVector4<T> wyxz()const{ return TVector4<T>( w, y, x, z ); }
		TVector4<T> wyzx()const{ return TVector4<T>( w, y, z, x ); }
		TVector4<T> wzxy()const{ return TVector4<T>( w, z, x, y ); }
		TVector4<T> wzyx()const{ return TVector4<T>( w, z, y, x ); }

		operator TVector2<T>() const { return xy(); }
		operator TVector3<T>() const { return xyz(); }

        template<class Another>
		explicit TVector4( const TVector4<Another>& v) : x((T)v.x), y((T)v.y), z((T)v.z), w((T)v.w) {}
		template<class Another>
		Another To() const
		{ 
			return Another( (typename Another::Elem)x, (typename Another::Elem)y,
				(typename Another::Elem)z, (typename Another::Elem)w );
		}
		inline const TVector4& Assign(const TVector4& v){ return *this = v; }

        // =========================================================================
        // 矢量的基本运算  
        // =========================================================================        
        // 矢量长度的平方  
        T LenSqr() const;
        // 矢量的长度  
        T Len() const;

		// 返回Floor矢量
		const TVector4 Floor() const;
        // 返回一个原矢量的单位矢量，原矢量不改变  
		const TVector4 Unit() const;
        // 将自身单位化  
        const TVector4& Normalize();
        // 计算与另一个矢量间的距离  
        float Dist( const TVector4& right ) const;
		// 与另一矢量乘  
		TVector4 Multiply( const TVector4& v ) const;

        // =====================================================================
        // 矢量的运算  
		// =====================================================================
		const TVector4 Add(const TVector4& b) const;
		const TVector4 Sub(const TVector4& b) const;
		const TVector4 Mul(const T s) const;
		const TVector4 Div(const T s) const;
		const TVector4& Scale( T v );

        // 矢量的点积  
        T Dot( const TVector4& b ) const;

        // =====================================================================
        // 操作符重载  
        // =====================================================================
        const T& operator [] ( const int32_t i ) const;
        T& operator [] ( const int32_t i );

        const TVector4 operator + (const TVector4& b) const;
		const TVector4 operator - (const TVector4& b) const;
		const TVector4 operator + (const T s) const;
		const TVector4 operator - (const T s) const;
        const TVector4 operator * (const T s) const;
        const TVector4 operator / (const T s) const;
        const TVector4 operator - () const;

        friend inline const TVector4 operator * (const T s, const TVector4& v)
        {
            return v * s;
        }

        const TVector4& operator = ( const TVector4& b );
        const TVector4& operator += (const TVector4& b); 
		const TVector4& operator -= (const TVector4& b) ;
		const TVector4& operator += (const T s);
		const TVector4& operator -= (const T s);
        const TVector4& operator *= (const T s);
        const TVector4& operator /= (const T s);

        // 点积  
        T operator * (const TVector4& v) const;    
        // 叉积  
        TVector4 operator ^ (const TVector4& v) const;   

        bool operator == ( const TVector4& b ) const;
        bool operator != ( const TVector4& b ) const;
    };

    //========================================================================
    // 矢量的基本运算  
    //========================================================================
    // 矢量的长度  
    template<class T> inline T TVector4<T>::Len() const
    {
        return (T)sqrtf( x*x + y*y + z*z + w*w );
    }

    // 矢量长度的平方  
    template<class T> inline T TVector4<T>::LenSqr() const
    {
        return x*x + y*y + z*z + w*w;
	}

	// 返回Floor矢量
	template<class T> inline const TVector4<T> TVector4<T>::Floor() const
	{
		return TVector4<T>( floor( x ), floor( y ), floor( z ), floor( w ) );
	}

    // 返回一个原矢量的单位矢量，原矢量不改变  
    template<class T> inline const TVector4<T> TVector4<T>::Unit() const
    {
        float fLen = Len();
        GammaAst( fLen > 0 );
        return (*this) / fLen;
	}
	
    // 返回单位化矢量  
    template<class T> inline const TVector4<T>& TVector4<T>::Normalize()
    {
        float fLen = Len();
        GammaAst( fLen > 0 );
        (*this) /= fLen;
        return *this;
    } 

    // 计算与另一个矢量间的距离  
    template<class T> inline float TVector4<T>::Dist( const TVector4& right ) const
    {
        TVector4 DistVec( x - right.x, y - right.y, z - right.z, w - right.w );
        return DistVec.Len();
	}

	template<class T> TVector4<T> TVector4<T>::Multiply( const TVector4& v ) const
	{
		return TVector4( x*v.x, y*v.y, z*v.z, w*v.w );
	}

    //=================================================================================
    // 矢量的运算  
	//=================================================================================
	template<class T>
	inline const TVector4<T> TVector4<T>::Add( const TVector4& b ) const
	{
		return TVector4( x + b.x, y + b.y, z + b.z, w + b.w );
	}

	template<class T>
	inline const TVector4<T> TVector4<T>::Sub( const TVector4& b ) const
	{
		return TVector4( x - b.x, y - b.y, z - b.z, w - b.w );
	}

	template<class T>
	inline const TVector4<T> TVector4<T>::Mul( const T s ) const
	{
		return TVector4( x*s, y*s, z*s, w*s );
	}

	template<class T>
	inline const TVector4<T> TVector4<T>::Div( const T s ) const
	{
		return TVector4( x/s, y/s, z/s, w/s );
	}

	template<class T>
	inline const TVector4<T>& TVector4<T>::Scale( const T v )
	{
		x *= v;
		y *= v;
		z *= v;
		w *= v;
		return *this;
	}

    // 矢量的点积  
    template<class T> inline T TVector4<T>::Dot( const TVector4& b ) const
    {
        return x*b.x + y*b.y + z*b.z + w*b.w;
    }

    template<class T> inline const T& TVector4<T>::operator [] ( const int32_t i ) const
    {
        return v[i];
    }

    template<class T> inline T& TVector4<T>::operator [] ( const int32_t i )
    {
        return v[i];
    }

    template<class T> inline const TVector4<T> TVector4<T>::operator + (const TVector4& b) const
    {
        return TVector4(x + b.x, y + b.y, z + b.z, w + b.w);
    }

    template<class T> inline const TVector4<T> TVector4<T>::operator - (const TVector4& b) const
    {
        return TVector4(x - b.x, y - b.y, z - b.z, w - b.w);
	}

	template<class T> inline const TVector4<T> TVector4<T>::operator + (const T s) const
	{
		return TVector4(x + s, y + s, z + s, w + s);
	}

	template<class T> inline const TVector4<T> TVector4<T>::operator - (const T s) const
	{
		return TVector4(x - s, y - s, z - s, w - s);
	}

    template<class T> inline const TVector4<T> TVector4<T>::operator * (const T s) const
    {
        return TVector4(x*s, y*s, z*s, w*s);
    }


    template<class T> inline const TVector4<T> TVector4<T>::operator / (const T s) const
    {
        return TVector4(x/s, y/s, z/s, w/s);
    }

    template<class T> inline const TVector4<T>& TVector4<T>::operator = ( const TVector4& b )
    {
        x = b.x;
		y = b.y;
		z = b.z;
		w = b.w;

        return *this;
    }

    template<class T> inline const TVector4<T>& TVector4<T>::operator += (const TVector4& b) 
    {
        x += b.x;
		y += b.y;
		z += b.z;
		w += b.w;

        return *this;
    }

    template<class T> inline const TVector4<T>& TVector4<T>::operator -= (const TVector4& b) 
    {
        x -= b.x;
		y -= b.y;
		z -= b.z;
		w -= b.w;

        return *this;
	}

	template<class T> inline const TVector4<T>& TVector4<T>::operator += (const T s) 
	{
		x += s;
		y += s;
		z += s;
		w += s;

		return *this;
	}

	template<class T> inline const TVector4<T>& TVector4<T>::operator -= (const T s) 
	{
		x -= s;
		y -= s;
		z -= s;
		w -= s;

		return *this;
	}

    template<class T> inline const TVector4<T>& TVector4<T>::operator *= (const T s)
    {
        x *= s;
		y *= s;
		z *= s;
		w *= s;

        return *this;
    }

    template<class T> inline const TVector4<T>& TVector4<T>::operator /= (const T s)
    {
        x /= s;
		y /= s;
		z /= s;
		w /= s;

        return *this;
    }

    // 点积  
    template<class T> inline T TVector4<T>::operator * (const TVector4& b) const     
    { 
        return x*b.x + y*b.y + z*b.z + w*b.w; 
    }

    template<class T> inline bool TVector4<T>::operator == ( const TVector4& b ) const
    {
        return b.x == x && b.y == y && b.z == z && b.w == w;
    }

    template<class T> inline bool TVector4<T>::operator != ( const TVector4& b ) const
    {
        return !( b == *this );
    }

    template<class T> inline const TVector4<T> TVector4<T>::operator - () const
    {
        return TVector4( -x, -y, -z, -w );
    }

	typedef TVector4<int32_t>     CVector4;
	typedef TVector4<float>     CVector4f;
	typedef TVector4<double>    CVector4d;
} 

typedef Gamma::TVector4<float>	float4;
#endif
