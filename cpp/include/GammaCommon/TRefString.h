//===============================================
// GammaStrStream.h 
// 定义字符串构造类，替代swprintf以及sprintf
// 柯达昭
// 2007-09-07
//===============================================

#ifndef __GAMMA_TREFSTRING_H__
#define __GAMMA_TREFSTRING_H__

#include "GammaCommon/GammaCommonType.h"
#include "GammaCommon/TSmartPtr.h"
#include <string>

namespace Gamma
{
	template<typename _Elem, typename StringType = std::basic_string<_Elem>>
	class TRefString : public StringType
	{
		uint32_t	m_nRef;
	public:
		TRefString() : m_nRef(0)
		{
		}

		TRefString( const StringType& _Right ) : m_nRef(0)
		{	
			this->assign(_Right);
		}

		TRefString( const _Elem *_Ptr ) : m_nRef(0)
		{	
			this->assign(_Ptr);
		}

		TRefString( const _Elem *_Ptr, size_t n ) : m_nRef(0)
		{	
			this->assign( _Ptr, n );
		}

		TRefString(_Elem _Ch ) : m_nRef(0)
		{	
			this->assign( 1, _Ch );
		}

		TRefString& operator=( const StringType& _Right )
		{	
			this->assign( _Right );
			return *this;
		}

		TRefString& operator=( const _Elem *_Ptr )
		{	
			this->assign( _Ptr );
			return *this;
		}

		TRefString& operator=(_Elem _Ch )
		{	
			this->assign( 1, _Ch );
			return *this;
		}

		void AddRef()
		{ 
			++m_nRef; 
		}

		void Release()
		{ 
			if( --m_nRef == 0 )
				delete this;
		}

		uint32_t GetRef()
		{
			return m_nRef;
		}
	};

	typedef TRefString<char>					CRefString;
	typedef TRefString<wchar_t>					CRefWString;
	typedef TSmartPtr< TRefString<char> >		CRefStringPtr;
	typedef TSmartPtr< TRefString<wchar_t> >	CRefWStringPtr;
}

#endif