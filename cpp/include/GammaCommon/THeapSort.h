//=========================================================================================
// THeapSort.h
// 定义一个最小堆排序类
// 柯达昭
// 2017-09-19
//=========================================================================================
#ifndef __THEAP_SORT_H__
#define __THEAP_SORT_H__

#include "GammaCommon/GammaHelp.h"

namespace Gamma
{
	//===============================================================
	// 注：堆排序要求数组下标从1开始
	// 以下分别封装了对std::vector以及c数组的处理
	//===============================================================
	template<typename OneBaseArray>
	class THeapSort
	{
		typedef typename OneBaseArray::DataType DataType;
		OneBaseArray m_aryData;
	public:
		THeapSort()
		{
		}

		void CheckUp( size_t n )
		{
			DataType CheckNode = m_aryData.GetData( n );
			size_t nParent = n >> 1;
			while( nParent && CheckNode < m_aryData.GetData( nParent ) )
			{
				m_aryData.SetData( n, m_aryData.GetData( nParent ) );
				n = nParent;
				nParent = n >> 1;
			}
			m_aryData.SetData( n, CheckNode );
		}

		void CheckDown( size_t n )
		{
			DataType Last = m_aryData.GetData( n );
			size_t nSize = m_aryData.GetSize();			
			for( size_t nChild = n << 1; nChild <= nSize; nChild = n << 1 )
			{
				if( nChild + 1 <= nSize && 
					m_aryData.GetData( nChild + 1 ) < m_aryData.GetData( nChild ) )
					nChild++;
				if( !( m_aryData.GetData( nChild ) < Last ) )
					break;
				m_aryData.SetData( n, m_aryData.GetData( nChild ) );
				n = nChild;
			}
			m_aryData.SetData( n, Last );
		}

		void Insert( const DataType& New )
		{
			m_aryData.PushBack( New );
			CheckUp( m_aryData.GetSize() );
		}

		void SetValue( size_t n, const DataType& New )
		{
			if( m_aryData.GetData( n ) < New )
			{
				m_aryData.SetData( n, New );
				CheckDown( n );
			}
			else
			{
				m_aryData.SetData( n, New );
				CheckUp( n );
			}
		}

		DataType RemoveFront()
		{
			size_t nSize = m_aryData.GetSize();
			GammaAst( nSize );

			DataType Front = m_aryData.GetData( 1 );
			if( nSize == 1 )
			{
				m_aryData.PopBack();
				return Front;
			}

			m_aryData.SetData( 1, m_aryData.GetData( nSize ) );
			m_aryData.PopBack();
			CheckDown( 1 );
			return Front;
		}

		bool IsEmpty() const
		{
			return m_aryData.GetSize() == 0;
		}

		OneBaseArray& GetArrayData() 
		{ 
			return m_aryData;
		}
	};

	//===============================================================
	// 封装std::vector的处理
	//===============================================================
	template<typename _Elem, typename Parent_t = std::vector<_Elem> >
	class THeapSortVector : public Parent_t
	{
	public:
		typedef _Elem DataType;
		void Clear()								{ Parent_t::reserve( Parent_t::size() ); Parent_t::clear(); }
		const _Elem& GetData( size_t n ) const 		{ return ( *this )[ n - 1 ]; }
		void SetData( size_t n, const _Elem& e )	{ ( *this )[ n - 1 ] = e;	}
		size_t GetSize() const						{ return Parent_t::size(); }
		void PushBack( const _Elem& e )				{ Parent_t::push_back( e ); }
		void PopBack()								{ Parent_t::erase( --Parent_t::end() ); }
	};

	//===============================================================
	// 封装数组的处理
	//===============================================================
	template<typename _Elem, size_t nMax>
	class THeapSortArray
	{
	protected:
		tbyte	m_nBuffer[sizeof( _Elem ) * nMax];
		size_t	m_nCurPos;
		_Elem* GetElems() { return (_Elem*)m_nBuffer; }
		const _Elem* GetElems() const { return (const _Elem*)m_nBuffer; }
	public:
		typedef _Elem DataType;
		THeapSortArray() : m_nCurPos( 0 )			{}
		void Clear()								{ m_nCurPos = 0; }
		const _Elem& GetData( size_t n ) const 		{ GammaAst( n - 1 < m_nCurPos ); return GetElems()[ n - 1 ]; }
		void SetData( size_t n, const _Elem& e )	{ GammaAst( n - 1 < m_nCurPos ); GetElems()[ n - 1 ] = e; }
		size_t GetSize() const						{ return m_nCurPos; }
		void PushBack( const _Elem& e )				{ GammaAst( m_nCurPos < nMax );	GetElems()[m_nCurPos++] = e; }
		void PopBack()								{ GammaAst( m_nCurPos ); m_nCurPos--; }
	};
}

#endif