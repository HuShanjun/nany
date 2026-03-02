//=====================================================================
// TRect.h 
// 矩形的各种算法
// 柯达昭
// 2007-09-06
//=======================================================================

#ifndef _GAMMA_TRECT_H_
#define _GAMMA_TRECT_H_

#include "GammaCommon/TVector2.h"

namespace Gamma
{
    template <typename T>
    struct TRect
    {
        // Constructors
    public:
        // uninitialized rectangle
        TRect() : left( T() ),top( T() ),right( T() ),bottom( T() ){};

        // from left, top, right, and bottom
        TRect(T l, T t, T r, T b)
        {
            left	= l;
            top		= t;
            right	= r;
            bottom	= b;
        };

		template<typename T1>
        TRect( const TRect<T1>& srcRect )
        {
            left	= (T)srcRect.left;
            top		= (T)srcRect.top;
            right	= (T)srcRect.right;
            bottom	= (T)srcRect.bottom;
        };

		template<class T1>
		const TRect<T>& operator= ( const TRect<T1>& srcRect )
		{
			left	= (T)srcRect.left;
			top		= (T)srcRect.top;
			right	= (T)srcRect.right;
			bottom	= (T)srcRect.bottom;
			return *this;
		}

        TRect operator& ( const TRect& rt ) const
        {
            TRect o( Max( left, rt.left ), Max( top, rt.top ), Min( right, rt.right ), Min( bottom, rt.bottom ) );

			// 如果不存在交集，下面的代码会把矩形归零
            o.bottom = Max( o.top, o.bottom );
            o.right  = Max( o.left, o.right );

            return o;
        }

        TRect operator| ( const TRect& rt ) const
        {
            TRect o( Min( left, rt.left ), Min( top, rt.top ), Max( right, rt.right ), Max( bottom, rt.bottom ) );

			// 当矩形参数为正数时，任何情况下都可求并集，下面的代码防止矩形参数为负数时出现bug
            o.bottom = Max( o.top, o.bottom );
            o.right  = Max( o.left, o.right );

            return o;
        }

        TRect operator+ ( const TVector2<T> pos ) const
        {
            return TRect( left + pos.x, top + pos.y, right + pos.x, bottom + pos.y );
        }

        TRect operator- ( const TVector2<T> pos ) const
        {
            return TRect( left - pos.x, top - pos.y, right - pos.x, bottom - pos.y );
        }

        TRect operator+ ( const TRect<T> rt ) const
        {
            return TRect( left + rt.left, top + rt.top, right + rt.right, bottom + rt.bottom );
        }

        TRect operator- ( const TRect<T> rt ) const
        {
            return TRect( left - rt.left, top - rt.top, right - rt.right, bottom - rt.bottom );
        }

        TRect operator+= ( const TVector2<T> pos )
        {
            left	+= pos.x;
            top		+= pos.y;
            right	+= pos.x;
            bottom	+= pos.y;
            return *this;
        }

        TRect operator-= ( const TVector2<T> pos )
        {
            left    -= pos.x;
            top		-= pos.y;
            right	-= pos.x;
            bottom	-= pos.y;
            return *this;
        }

        TRect operator+= ( const TRect<T> rt )
        {
            left    += rt.left;
            top		+= rt.top;
            right	+= rt.right;
            bottom	+= rt.bottom;
            return *this;
        }

        TRect operator-= ( const TRect<T> rt )
        {
            left    -= rt.left;
            top		-= rt.top;
            right	-= rt.right;
            bottom	-= rt.bottom;
            return *this;
		}

		bool operator== ( const TRect<T> rt ) const
		{
			return left == rt.left && top == rt.top && right == rt.right && bottom == rt.bottom;
		}

		bool operator != ( const TRect<T> rt ) const
		{
			return !( *this == rt );
		}

        TRect operator* ( T n ) const
        {
            return TRect( left*n, top*n, right*n, bottom*n );
		}

		TRect operator/ ( T n ) const
		{
			return TRect( left/n, top/n, right/n, bottom/n );
		}

		TRect operator* ( const TVector2<T> pos ) const
		{
			return TRect( left*pos.x, top*pos.y, right*pos.x, bottom*pos.y );
		}

		TRect operator/ ( const TVector2<T> pos ) const
		{
			return TRect( left/pos.x, top/pos.y, right/pos.x, bottom/pos.y );
		}

        // Attributes (in addition to RECT members)

        // retrieves the width
        T Width() const     { return right - left; }
        // returns the height
		T Height() const     { return bottom - top; }
		// retrieves the horizontal center
		T HorCenter() const     { return ( right + left )/2; }
		// retrieves the vertical center
		T VerCenter() const     { return ( bottom + top )/2; }
		// retrieves center
		TVector2<T> Center() const   { return TVector2<T>( HorCenter(), VerCenter() ); }
		// retrieves scale
		TVector2<T> Scale() const   { return TVector2<T>( Width(), Height() ); }
        // point in rect
		bool IsPtInRect( const T& x, const T& y ) const { return x >= left && x < right && y >= top && y < bottom; }
		// lefttop
		TVector2<T> LeftTop() const { return TVector2<T>( left, top ); }
		// rightbottom
		TVector2<T> RightBottom() const { return TVector2<T>( right, bottom ); }
		// righttop
		TVector2<T> RightTop() const { return TVector2<T>( right, top ); }
		// leftbottom
		TVector2<T> LeftBottom() const { return TVector2<T>( left, bottom ); }

		bool IntersectRect( const TRect* pRt ) const
		{
			if( !pRt )
				return false;
			TRect o( Max( left, pRt->left ), Max( top, pRt->top ), Min( right, pRt->right ), Min( bottom, pRt->bottom ) );
			o.bottom = Max( o.top, o.bottom );
			o.right  = Max( o.left, o.right );
			return ( o.Width() != 0 && o.Height() != 0 );
		}

		TRect Add ( const TRect<T> rt ) const
		{
			return TRect( left + rt.left, top + rt.top, right + rt.right, bottom + rt.bottom );
		}

		TRect Sub ( const TRect<T> rt ) const
		{
			return TRect( left - rt.left, top - rt.top, right - rt.right, bottom - rt.bottom );
		}

		TRect Mul ( T n ) const
		{
			return TRect( left*n, top*n, right*n, bottom*n );
		}

		TRect Div ( T n ) const
		{
			return TRect( left/n, top/n, right/n, bottom/n );
		}

        T    left;
        T    top;
        T    right;
        T    bottom; 
	};

	typedef TRect<float>        CFRect;
	typedef TRect<int32_t>        CIRect;
	typedef TRect<uint32_t>       CURect;
	typedef TRect<int16_t>        CIRect16;
	typedef TRect<uint16_t>       CURect16;
	typedef TRect<int8_t>			CIRect8;
	typedef TRect<uint8_t>		CURect8;
}

#endif
