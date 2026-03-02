//=====================================================================
// TVector3.h 
// 3D矢量模板及相关的各种运算, CVector3f代表的是单精度浮点矢量，常用类型
// 柯达昭
// 2007-08-30
//=======================================================================

#ifndef __GAMMA_VECTOR3_H__
#define __GAMMA_VECTOR3_H__

#include "GammaCommon/TVector2.h"
#pragma warning(disable: 4201)

namespace Gamma
{
    class CMatrix;
    template<class T> struct TVector3
    {
        union 
        {
            // This structure here is nameless, which really isn't 
            // legal C++... most compilers will deal with it,however.
            struct 
            {
                T x,y,z; // 3 real components of the vector
            };
            T    v[3]; // Array access useful in loops
		};

		enum 
		{
			eAxisZ = 2,
		};

		typedef T Elem;

        // =========================================================================
        // 构造函数  
        //==========================================================================
        TVector3() : x(), y(), z() {}
		TVector3( const T x, const T y, const T z ) : x(x), y(y), z(z) {}
        explicit TVector3( const T value) : x(value), y(value), z(value) {}
		TVector3( const TVector3& v ) : x(v.x), y(v.y), z(v.z) {}
		TVector3( const TVector2<T>& v, T z ) : x(v.x), y(v.y), z(z) {}
		TVector3( T x, const TVector2<T>& v ) : x(x), y(v.x), z(v.y) {}
		TVector3( const T f[3]){ v[0] = f[0]; v[1] = f[1]; v[2] = f[2]; }

		TVector2<T> xy()const{ return TVector2<T>( x, y ); }
		TVector2<T> xz()const{ return TVector2<T>( x, z ); }
		TVector2<T> yx()const{ return TVector2<T>( y, x ); }
		TVector2<T> yz()const{ return TVector2<T>( y, z ); }
		TVector2<T> zx()const{ return TVector2<T>( z, x ); }
		TVector2<T> zy()const{ return TVector2<T>( z, y ); }

		TVector3<T> xyz()const{ return TVector3<T>( x, y, z ); }
		TVector3<T> xzy()const{ return TVector3<T>( x, z, y ); }
		TVector3<T> yxz()const{ return TVector3<T>( y, x, z ); }
		TVector3<T> yzx()const{ return TVector3<T>( y, z, x ); }
		TVector3<T> zxy()const{ return TVector3<T>( z, x, y ); }
		TVector3<T> zyx()const{ return TVector3<T>( z, y, x ); }

		operator TVector2<T>() const { return xy(); }

        template<class Another>
		explicit TVector3( const TVector3<Another>& v) : x((T)v.x), y((T)v.y), z((T)v.z) {}

		template<class Another>
		Another To() const{ return Another( (typename Another::Elem)x, (typename Another::Elem)y, (typename Another::Elem)z ); }
		inline const TVector3& Assign(const TVector3& v){ return *this = v; }

        // =========================================================================
        // 矢量的基本运算  
        // =========================================================================        
        // 矢量长度的平方  
        T LenSqr() const;
        // 矢量的长度  
        T Len() const;
        // 矢量长度的快速计算  
        T LenFast() const;

		// 返回Floor矢量
		const TVector3 Floor() const;
        // 返回一个原矢量的单位矢量，原矢量不改变  
		const TVector3 Unit() const;
		// 返回一个原矢量的单位矢量，原矢量不改变,当为零向量时返回自身  
		const TVector3 UnitNoneZero() const;
        // 将自身单位化  
		const TVector3& Normalize();
		// 当为非零向量时将自身单位化  
		const TVector3& NormalizeNoneZero();
        // 计算与另一个矢量间的距离  
        float Dist( const TVector3& right ) const;
        // 计算与另一个矢量间的距离的平方  
        float DistSqr( const TVector3& right ) const;

        // =====================================================================
        // 矢量的运算  
		// =====================================================================
		const TVector3 Add(const TVector3& b) const;
		const TVector3 Sub(const TVector3& b) const;
		const TVector3 Mul(const T s) const;
		const TVector3 Div(const T s) const;
		const TVector3& Scale( T v );

        // 矢量的点积  
		T Dot( const TVector3& b ) const;
        // 矢量的叉积 this ^ b,注意顺序  
        TVector3 Cross( const TVector3& b ) const;
        // 到线段最短距离  
        TVector3 ClosestPointOnLine(const TVector3 &vA, const TVector3 &vB)const;
		// 分量相乘  
		TVector3 Multiply( const TVector3& b ) const;

        TVector3        GetMaxAxis()const;
        TVector3        GetMinAxis()const;
        const TVector3& Rotate( const CMatrix& Mat );
		const TVector3& FastTransform( const CMatrix& Mat );
		const TVector3& Transform( const CMatrix& Mat );

        // =====================================================================
        // 操作符重载  
        // =====================================================================
        const T& operator [] ( const int32 i ) const;
        T& operator [] ( const int32 i );

        const TVector3 operator + (const TVector3& b) const;
		const TVector3 operator - (const TVector3& b) const;
		const TVector3 operator + (const T b) const;
		const TVector3 operator - (const T b) const;
        const TVector3 operator * (const T s) const;
        const TVector3 operator / (const T s) const;
        const TVector3 operator - () const;

        friend inline const TVector3 operator * (const T s, const TVector3& v)
        {
            return v * s;
        }

        const TVector3& operator = ( const TVector3& b );
        const TVector3& operator += (const TVector3& b); 
		const TVector3& operator -= (const TVector3& b);
		const TVector3& operator += (const T b); 
		const TVector3& operator -= (const T b);
        const TVector3& operator *= (const T s);
        const TVector3& operator /= (const T s);

        // 点积   
        T operator * (const TVector3& v) const;    
        // 叉积  
        TVector3 operator ^ (const TVector3& v) const;   

        bool operator == ( const TVector3& b ) const;
        bool operator != ( const TVector3& b ) const;

        /// 近似的相等  
        bool Equal( const TVector3& rhs, float fExactness = 1e-5 ) const
        {
            return ((x >= 0 && rhs.x >= 0) || (x <= 0 && rhs.x <= 0))
                && ((y >= 0 && rhs.y >= 0) || (y <= 0 && rhs.y <= 0))
                && ((z >= 0 && rhs.z >= 0) || (z <= 0 && rhs.z <= 0))
            && GammaAbs(x - rhs.x) < fExactness && GammaAbs(y - rhs.y) < fExactness && GammaAbs(z - rhs.z) < fExactness;
        }
        
        /// 忽略符号的近似判断相等  
        bool EqualUnsigned( const TVector3& rhs, float fExactness = 1e-5 ) const
        {
            return GammaAbs(x - rhs.x) < fExactness && GammaAbs(y - rhs.y) < fExactness && GammaAbs(z - rhs.z) < fExactness;
		}

		inline TVector3 MidPoint( const TVector3& vec ) const
		{
			return TVector3(
				( x + vec.x ) * 0.5f,
				( y + vec.y ) * 0.5f,
				( z + vec.z ) * 0.5f );
		}

    };

    //========================================================================
    // 矢量的基本运算  
    //========================================================================
    // 矢量的长度  
    template<class T> inline T TVector3<T>::Len() const
    {
        return (T)sqrt( (T)( x*x + y*y + z*z ) );
    }

    // 矢量长度的平方  
    template<class T> inline T TVector3<T>::LenSqr() const
    {
        return x*x + y*y + z*z;
    }

    // 矢量长度的近似算法  
    template<class T> inline T TVector3<T>::LenFast () const
    {
        T   min, med, max;
        T   temp;

        max = fabsf(x);
        med = fabsf(y);
        min = fabsf(z);

        if (max < med)
        {
            temp = max;
            max = med;
            med = temp;
        }

        if (max < min)
        {
            temp = max;
            max = min;
            min = temp;
        }

        return max + ((med + min) * 0.25f);
	}

	// 返回Floor矢量
	template<class T> inline const TVector3<T> TVector3<T>::Floor() const
	{
		return TVector3<T>( floor( x ), floor( y ), floor( z ) );
	}

    // 返回一个原矢量的单位矢量，原矢量不改变  
    template<class T> inline const TVector3<T> TVector3<T>::Unit() const
    {
        T len = Len();
        GammaAst( len > 0 );
        return (*this) / len;
	}

	// 返回一个原矢量的单位矢量，原矢量不改变  
	template<class T> inline const TVector3<T> TVector3<T>::UnitNoneZero() const
	{
		T len = Len();
		return len > 0 ? (*this)/len : *this;
	}

    // 返回单位化矢量  
    template<class T> inline const TVector3<T>& TVector3<T>::Normalize()
    {
        T len = Len();
        GammaAst( len > 0 );
        (*this) /= len;
        return *this;
	} 

	// 当为非零向量时将自身单位化  
	template<class T> inline const TVector3<T>& TVector3<T>::NormalizeNoneZero()
	{
		T len = Len();
		if( len > 0 ) (*this) /= len;
		return *this;
	}

    // 计算与另一个矢量间的距离  
    template<class T> inline float TVector3<T>::Dist( const TVector3& right ) const
    {
        TVector3 DistVec( x - right.x, y - right.y, z - right.z );
        return DistVec.Len();
    }
    
    // 计算与另一个矢量间的距离的平方  
    template<class T> inline float TVector3<T>::DistSqr( const TVector3& right ) const
    {
        TVector3 DistVec( x - right.x, y - right.y, z - right.z );
        return DistVec.LenSqr();
    }

    //=================================================================================
    // 矢量的运算  
	//=================================================================================
	template<class T>
	inline const TVector3<T> TVector3<T>::Add( const TVector3& b ) const
	{
		return TVector3( x + b.x, y + b.y, z + b.z );
	}

	template<class T>
	inline const TVector3<T> TVector3<T>::Sub( const TVector3& b ) const
	{
		return TVector3( x - b.x, y - b.y, z - b.z );
	}

	template<class T>
	inline const TVector3<T> TVector3<T>::Mul( const T s ) const
	{
		return TVector3( x*s, y*s, z*s );
	}

	template<class T>
	inline const TVector3<T> TVector3<T>::Div( const T s ) const
	{
		return TVector3( x/s, y/s, z/s );
	}

	template<class T>
	inline const TVector3<T>& TVector3<T>::Scale( const T v )
	{
		x *= v;
		y *= v;
		z *= v;
		return *this;
	}

    // 矢量的点积  
    template<class T> inline T TVector3<T>::Dot( const TVector3& b ) const
    {
        return x*b.x + y*b.y + z*b.z;
    }

    // 矢量的叉积 this ^ b,注意顺序  
    template<class T> inline TVector3<T> TVector3<T>::Cross( const TVector3<T>& b ) const
    {
        return TVector3( y*b.z - z*b.y, z*b.x - x*b.z, x*b.y - y*b.x );
	}

	// 分量相乘  
	template<class T> inline TVector3<T> TVector3<T>::Multiply( const TVector3<T>& b ) const
	{
		return TVector3( x*b.x, y*b.y, z*b.z );
	}

    // 到线段最短距离  
    template<class T> inline TVector3<T> TVector3<T>::ClosestPointOnLine(const TVector3 &vA, const TVector3 &vB)const
    {
        TVector3 vDir = vB - vA;
        T d = vDir.Len();

        TVector3 vDelA = *this - vA;
        vDir /= d;
        T t = vDir.Dot( vDelA );

        if (t <= 0) 
            return vA;
        if (t >= d) 
            return vB;

        return vA + vDir * t;
    }

    //******************************************************************************
    /*! \fn     TVector3 TVector3<T>::GetMainAxis()const
    *   \brief  得到矢量的最大轴
    *           返回矢量中, x, y, z最大得对应得那个轴
    *   \return TVector3 得到得最大轴
    *   \sa     GetMinAxis
    *******************************************************************************/
    template<class T> inline TVector3<T> TVector3<T>::GetMaxAxis()const
    {
        if( x >= y )
			return x >= z ? TVector3( 1, 0, 0 ) : TVector3( 0, 0, 1 );
		else
			return y >= z ? TVector3( 0, 1, 0 ) : TVector3( 0, 0, 1 );
    }

    //******************************************************************************
    /*! \fn    TVector3 TVector3<T>::GetMinAxis()const
    *   \brief  得到矢量的最小轴
    *           返回矢量中, x, y, z最小得对应得那个轴
    *   \return TVector3 得到得最小轴
    *   \sa     GetMaxAxis
    *******************************************************************************/
    template<class T> inline TVector3<T> TVector3<T>::GetMinAxis()const
	{
		if( x < y )
			return x < z ? TVector3( 1, 0, 0 ) : TVector3( 0, 0, 1 );
		else
			return y < z ? TVector3( 0, 1, 0 ) : TVector3( 0, 0, 1 );
    }

    template<class T> inline const T& TVector3<T>::operator [] ( const int32 i ) const
    {
        return v[i];
    }

    template<class T> inline T& TVector3<T>::operator [] ( const int32 i )
    {
        return v[i];
    }

    template<class T> inline const TVector3<T> TVector3<T>::operator + (const TVector3& b) const
    {
        return TVector3(x + b.x, y + b.y, z + b.z);
    }

    template<class T> inline const TVector3<T> TVector3<T>::operator - (const TVector3& b) const
    {
        return TVector3(x - b.x, y - b.y, z - b.z);
	}

	template<class T> inline const TVector3<T> TVector3<T>::operator + (const T b) const
	{
		return TVector3(x + b, y + b, z + b);
	}

	template<class T> inline const TVector3<T> TVector3<T>::operator - (const T b) const
	{
		return TVector3(x - b, y - b, z - b);
	}

    template<class T> inline const TVector3<T> TVector3<T>::operator * (const T s) const
    {
        return TVector3(x*s, y*s, z*s);
    }


    template<class T> inline const TVector3<T> TVector3<T>::operator / (const T s) const
    {
        return TVector3(x/s, y/s, z/s);
    }

    template<class T> inline const TVector3<T>& TVector3<T>::operator = ( const TVector3& b )
    {
        x = b.x;
        y = b.y;
        z = b.z;

        return *this;
    }

    template<class T> inline const TVector3<T>& TVector3<T>::operator += (const TVector3& b) 
    {
        x += b.x;
        y += b.y;
        z += b.z;

        return *this;
    }

    template<class T> inline const TVector3<T>& TVector3<T>::operator -= (const TVector3& b) 
    {
        x -= b.x;
        y -= b.y;
        z -= b.z;

        return *this;
	}

	template<class T> inline const TVector3<T>& TVector3<T>::operator += (const T b) 
	{
		x += b;
		y += b;
		z += b;

		return *this;
	}

	template<class T> inline const TVector3<T>& TVector3<T>::operator -= (const T b) 
	{
		x -= b;
		y -= b;
		z -= b;

		return *this;
	}

    template<class T> inline const TVector3<T>& TVector3<T>::operator *= (const T s)
    {
        x *= s;
        y *= s;
        z *= s;

        return *this;
    }

    template<class T> inline const TVector3<T>& TVector3<T>::operator /= (const T s)
    {
        x /= s;
        y /= s;
        z /= s;

        return *this;
    }

    // 点积  
    template<class T> inline T TVector3<T>::operator * (const TVector3& b) const     
    { 
        return x*b.x + y*b.y + z*b.z; 
    }

    // 叉积  
    template<class T> inline TVector3<T> TVector3<T>::operator ^ (const TVector3& v) const     
    { 
        return TVector3( y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x ); 
    }


    template<class T> inline bool TVector3<T>::operator == ( const TVector3& b ) const
    {
        return b.x == x && b.y == y && b.z == z;
    }

    template<class T> inline bool TVector3<T>::operator != ( const TVector3& b ) const
    {
        return !( b == *this );
    }

    template<class T> inline const TVector3<T> TVector3<T>::operator - () const
    {
        return TVector3( -x, -y, -z );
    }

	typedef TVector3<int32>     CVector3;
	typedef TVector3<float>     CVector3f;
	typedef TVector3<float>     float3;
	typedef TVector3<double>    CVector3d;
	typedef TVector3<uint8>		CVector3u8;
	typedef TVector3<uint16>	CVector3u16;
	typedef TVector3<int8>		CVector3s8;
	typedef TVector3<int16>		CVector3s16;
} 

#endif
