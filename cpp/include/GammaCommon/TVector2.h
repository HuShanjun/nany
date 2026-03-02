//=====================================================================
// TVector2.h 
// 2D矢量模板及相关的各种运算, CVector3f代表的是单精度浮点矢量，常用类型
// 柯达昭
// 2007-09-06
//=======================================================================

#ifndef _GAMMA_VECTOR2_H_
#define _GAMMA_VECTOR2_H_

#pragma warning(disable: 4201)
#include <math.h>
#include "GammaCommon/GammaHelp.h"

namespace Gamma
{
	class CDir;
    template<class T> struct TVector2
    {
        union 
        {
            struct 
            {
                T    x,y; // 3 real components of the vector
            };

            T    v[2]; // Array access useful in loops
        };

		enum 
		{
			eAxisX = 0,
			eAxisY = 1,
		};

		typedef T Elem;

        // =========================================================================
        // 构造函数  
		//==========================================================================
		explicit TVector2( const T value ) : x(value), y(value) {}
		TVector2( const T x, const T y ) : x(x), y(y) {}
		TVector2( const T f[2]){ v[0] = f[0]; v[1] = f[1]; }
		TVector2() : x(), y(){}
		TVector2( const CDir& rhs );
		TVector2<T> yx(){ return TVector2<T>( y, x ); }
        
        template<class Another>
		explicit TVector2( const TVector2<Another>& v) : x((T)v.x), y((T)v.y) {}

		template<class Another>
		Another To() const{ return Another( (typename Another::Elem)x, (typename Another::Elem)y ); }
        // =========================================================================
        // 初始化函数  
        // =========================================================================
        inline void Init( T a = T(), T b = T());
        inline void Init(const TVector2& v);
		inline void Init(const T f[2]);
		inline const TVector2& Assign(const TVector2& v){ return *this = v; }

        // =========================================================================
        // 矢量的基本运算  
        // =========================================================================        
        // 矢量长度的平方  
		T LenSqr() const;
		// 矢量的长度  
		T Len() const;
		// 矢量的长度快速近似计算  
		T LenFast() const;

		// 返回Floor矢量
		const TVector2 Floor() const;
        // 返回一个原矢量的单位矢量，原矢量不改变  
		const TVector2 Unit() const;
        // 将自身单位化  
        const TVector2& Normalize();
        // 当为非零向量时将自身单位化  
        const TVector2& NormalizeNoneZero();

        // 
        void Norm( T nVal = (T)1 );
        // 矢量单位化的快速计算  
        void NormFast( T nVal = (T)1 );

        // 判断一个矢量是否等于零(在指定的精度内) ，fTol为比较的精度  
        bool IsZero( float fTol = 0 )    const;
        // 判断是否等于另一个矢量(在指定的精度内) ，fTol为比较的精度  
        bool Equal( const TVector2& b, const T r = T() ) const;
        // 计算与另一个矢量间的距离  
        T Dist( const TVector2& right ) const;
		// 分量相乘  
		TVector2 Multiply( const TVector2& b ) const;
		// 返回方向  
		uint8 GetDir() const;
		
        // =====================================================================
        // 矢量的运算  
		const TVector2 Add(const TVector2& b) const;
		const TVector2 Sub(const TVector2& b) const;
		const TVector2 Mul(const T s) const;
		const TVector2 Div(const T s) const;
		const TVector2& Scale( T v );

        // =====================================================================
        // 矢量的点积  
        T Dot( const TVector2& b ) const;

        TVector2    GetMaxAxis()const;
        TVector2    GetMinAxis()const;

        // =====================================================================
        // 操作符重载  
        // =====================================================================
        const T& operator [] ( const int32 i ) const;
        T& operator [] ( const int32 i );

        const TVector2 operator + (const TVector2& b) const;
		const TVector2 operator - (const TVector2& b) const;
		const TVector2 operator + (const T s) const;
		const TVector2 operator - (const T s) const;
        const TVector2 operator * (const T s) const;
        const TVector2 operator / (const T s) const;
        const TVector2 operator - () const;

        friend inline const TVector2 operator * (const T s, const TVector2& v)
        {
            return v * s;
        }

        const TVector2& operator = ( const TVector2& b );
        const TVector2& operator += (const TVector2& b); 
		const TVector2& operator -= (const TVector2& b) ;
		const TVector2& operator += (const T s); 
		const TVector2& operator -= (const T s) ;
        const TVector2& operator *= (const T s);
        const TVector2& operator /= (const T s);

        // 点积  
        T operator * (const TVector2& v) const;    
        // 叉积  
        TVector2 operator ^ (const TVector2& v) const;   

        bool operator == ( const TVector2& b ) const;
        bool operator != ( const TVector2& b ) const;

        bool operator > (const TVector2 &other) const;
        bool operator < (const TVector2 &other) const; 
        bool operator >= (const TVector2 &other) const; 
        bool operator <= (const TVector2 &other) const; 
    };

	template<class T>
	inline const TVector2<T> TVector2<T>::Add( const TVector2& b ) const
	{
		return TVector2(x + b.x, y + b.y);
	}

	template<class T>
	inline const TVector2<T> TVector2<T>::Sub( const TVector2& b ) const
	{
		return TVector2(x - b.x, y - b.y);
	}

	template<class T>
	inline const TVector2<T> TVector2<T>::Mul( const T s ) const
	{
		return TVector2(x*s, y*s);
	}

	template<class T>
	inline const TVector2<T> TVector2<T>::Div( const T s ) const
	{
		return TVector2(x/s, y/s);
	}

	template<class T> inline const TVector2<T>& TVector2<T>::Scale( T v )
	{
		x *= v;
		y *= v;
		return *this;
	}

    //====================================================================
    // 初始化函数  
    //====================================================================
    template<class T> inline void TVector2<T>::Init( T a, T b)
    {
        x = a;
        y = b;
    }

    template<class T> inline void TVector2<T>::Init(const TVector2& v)
    {
        x = v.x;
        y = v.y;
    }

    template<class T> inline void TVector2<T>::Init(const T f[2])
    {
        v[0] = f[0];
        v[1] = f[1];
    }

    //========================================================================
    // 矢量的基本运算  
    //========================================================================
    // 矢量的长度  
    template<class T> inline T TVector2<T>::Len() const
    {
        return (T)sqrtf( (float)( x*x + y*y ) );
    }

    // 矢量长度的平方  
    template<class T> inline T TVector2<T>::LenSqr() const
    {
        return x*x + y*y;
	}

	// 矢量长度的近似算法  
	template<class T> inline T TVector2<T>::LenFast() const
	{
		T xTemp = x < 0 ? -x : x;
		T yTemp = y < 0 ? -y : y;
		T min, max;
		if( xTemp < yTemp )
		{
			min = xTemp;
			max = yTemp;
		}
		else
		{
			min = yTemp;
			max = xTemp;
		}

		return max + min/2;
	}
	
	// 返回Floor矢量
	template<class T> inline const TVector2<T> TVector2<T>::Floor() const
	{
		return TVector2<T>( floor( x ), floor( y ) );
	}

    // 返回一个原矢量的单位矢量，原矢量不改变  
    template<class T> inline const TVector2<T> TVector2<T>::Unit() const
    {
        T fLen = Len();
        GammaAst( fLen > 0 );
        return (*this) / fLen;
    }

    // 将自身单位化  
    template<class T> inline const TVector2<T>& TVector2<T>::Normalize()
    {
        T fLen = Len();
        GammaAst( fLen > 0 );
        (*this ) /= fLen;
        return *this;
    }

    template<class T> inline const TVector2<T>& TVector2<T>::NormalizeNoneZero()
    {
        T len = Len();
        if( len > 0 ) (*this) /= len;
        return *this;
    }

    // 判断一个矢量是否等于零(在指定的精度内) ，fTol为比较的精度  
    template<class T> inline bool TVector2<T>::IsZero(float fTol)    const
    {
        if( fabs( x ) <= fTol && fabs( y ) <= fTol )
            return true;
        else
            return false;
    }

    // 判断是否等于另一个矢量(在指定的精度内) ，fTol为比较的精度  
    template<class T> inline bool TVector2<T>::Equal(const TVector2& b, const T r) const
    {
        //within a tolerance
        const TVector2 t = *this - b;//difference

        return t.Dot(t) <= r*r;//radius
    }

    // 计算与另一个矢量间的距离  
    template<class T> inline T TVector2<T>::Dist( const TVector2& right ) const
    {
        TVector2<T> DistVec( x - right.x, y - right.y );
        return DistVec.Len();
    }

    //=================================================================================
    // 矢量的运算  
    //=================================================================================
    // 矢量的点积  
    template<class T> inline T TVector2<T>::Dot( const TVector2& b ) const
    {
        return x*b.x + y*b.y;
	}

	// 分量相乘  
	template<class T> inline TVector2<T> TVector2<T>::Multiply( const TVector2<T>& b ) const
	{
		return TVector2( x*b.x, y*b.y );
	}

    //******************************************************************************
    /*! \fn     TVector2 TVector2<T>::GetMainAxis()const
    *   \brief  得到矢量的最大轴
    *           返回矢量中, x, y, z最大得对应得那个轴
    *   \return TVector2 得到得最大轴
    *   \sa     GetMinAxis
    *******************************************************************************/
    template<class T> inline TVector2<T> TVector2<T>::GetMaxAxis()const
    {
        if (x >= y)
            return TVector2(1, 0);
        else
            return TVector2(0, 1);
    }

    //******************************************************************************
    /*! \fn    TVector2 TVector2<T>::GetMinAxis()const
    *   \brief  得到矢量的最小轴
    *           返回矢量中, x, y, z最小得对应得那个轴
    *   \return TVector2 得到得最小轴
    *   \sa     GetMaxAxis
    *******************************************************************************/
    template<class T> inline TVector2<T> TVector2<T>::GetMinAxis()const
    {
        if (x < y)
            return TVector2(1, 0);
        else
            return TVector2(0, 1);
    }

    template<class T> inline const T& TVector2<T>::operator [] ( const int32 i ) const
    {
        return v[i];
    }

    template<class T> inline T& TVector2<T>::operator [] ( const int32 i )
    {
        return v[i];
    }

    template<class T> inline const TVector2<T> TVector2<T>::operator + (const TVector2& b) const
    {
        return TVector2(x + b.x, y + b.y);
    }

    template<class T> inline const TVector2<T> TVector2<T>::operator - (const TVector2& b) const
    {
        return TVector2(x - b.x, y - b.y);
	}

	template<class T> inline const TVector2<T> TVector2<T>::operator + (const T s) const
	{
		return TVector2(x + s, y + s);
	}

	template<class T> inline const TVector2<T> TVector2<T>::operator - (const T s) const
	{
		return TVector2(x - s, y - s);
	}

    template<class T> inline const TVector2<T> TVector2<T>::operator * (const T s) const
    {
        return TVector2(x*s, y*s);
    }


    template<class T> inline const TVector2<T> TVector2<T>::operator / (const T s) const
    {
        return TVector2(x/s, y/s);
    }

    template<class T> inline const TVector2<T>& TVector2<T>::operator = ( const TVector2& b )
    {
        x = b.x;
        y = b.y;
        return *this;
    }

    template<class T> inline const TVector2<T>& TVector2<T>::operator += (const TVector2& b) 
    {
        x += b.x;
        y += b.y;
        return *this;
    }

    template<class T> inline const TVector2<T>& TVector2<T>::operator -= (const TVector2& b) 
    {
        x -= b.x;
        y -= b.y;
        return *this;
	}

	template<class T> inline const TVector2<T>& TVector2<T>::operator += (const T s) 
	{
		x += s;
		y += s;
		return *this;
	}

	template<class T> inline const TVector2<T>& TVector2<T>::operator -= (const T s) 
	{
		x -= s;
		y -= s;
		return *this;
	}

    template<class T> inline const TVector2<T>& TVector2<T>::operator *= (const T s)
    {
        x *= s;
        y *= s;
        return *this;
    }

    template<class T> inline const TVector2<T>& TVector2<T>::operator /= (const T s)
    {
        x /= s;
        y /= s;
        return *this;
    }


    // 点积  
    template<class T> inline T TVector2<T>::operator * (const TVector2& b) const     
    { 
        return x*b.x + y*b.y; 
    }

    template<class T> inline bool TVector2<T>::operator == ( const TVector2& b ) const
    {
        return (b.x==x && b.y==y);
    }

    template<class T> inline bool TVector2<T>::operator != ( const TVector2& b ) const
    {
        return !(b == *this);
    }

    template<class T> inline const TVector2<T> TVector2<T>::operator - () const
    {
        return TVector2( -x, -y );
    }


    template<class T> inline bool TVector2<T>::operator > (const TVector2 &other) const 
    {
        return ((x>other.x) && (y>other.y)); 
    }

    template<class T> inline bool TVector2<T>::operator < (const TVector2 &other) const 
    { 
        return ((x<other.x) && (y<other.y)); 
    }

    template<class T> inline bool TVector2<T>::operator >= (const TVector2 &other) const 
    { 
        return ((x>=other.x) && (y>=other.y)); 
    }

    template<class T> inline bool TVector2<T>::operator <= (const TVector2 &other) const 
    { 
        return ((x<=other.x) && (y<=other.y)); 
    }

	typedef TVector2<int32>     CVector2;
	typedef TVector2<float>		CVector2f;
	typedef TVector2<float>     float2;
	typedef TVector2<int32>		CPos;
	typedef TVector2<int16>		CPos16;
	typedef TVector2<int8>		CPos8;
	typedef TVector2<uint32>	CPos32u;
	typedef TVector2<uint16>	CPos16u;
	typedef TVector2<uint8>		CPos8u;

	typedef TVector2<uint32>	CSize32;
}

#endif
