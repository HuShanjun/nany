//===============================================
// TSmartPtr.h
// 定义智能指针
// 柯达昭
// 2013-04-11
//===============================================

#pragma once

namespace Gamma
{
	template<class T> 
	class TSmartPtr
	{
	public:
		TSmartPtr() 
			: m_pObject( NULL )
		{
		}

		~TSmartPtr()
		{ 
			if( !m_pObject )
				return;
			m_pObject->Release();
		}

		TSmartPtr( const TSmartPtr<T>& rhs ) 
		{
			T* pObj = rhs.ptr(); 
			if( pObj ) 
				pObj->AddRef(); 
			m_pObject = pObj;
		}

		TSmartPtr( T* pOther )
		{
			if ( pOther ) 
				pOther->AddRef(); 
			m_pObject = pOther;
		}

		TSmartPtr<T>& operator = ( const TSmartPtr<T>& rhs )
		{
			T* pObj = rhs.ptr(); 
			if( pObj ) 
				pObj->AddRef(); 
			if( m_pObject )
				m_pObject->Release();
			m_pObject = pObj;
			return * this;
		}

		bool operator == ( const T* pObject ) const
		{
			return m_pObject == pObject;
		}

		bool operator == ( const TSmartPtr<T>& rhs ) const
		{
			return m_pObject == rhs.m_pObject;
		}

		bool operator != ( const TSmartPtr<T>& rhs ) const
		{
			return m_pObject != rhs.m_pObject;
		}

		T& operator * () const
		{
			return *m_pObject;
		}

		T* operator -> () const
		{
			return m_pObject;
		}

		operator T * () const
		{
			return m_pObject;
		}

		T * ptr() const
		{
			return m_pObject;
		}

		// for handwork grab;
		uint32 GetRef() 
		{
			if( !m_pObject )
				return 0;
			return m_pObject->GetRef();
		}

		// for handwork grab;
		void AddRef() 
		{
			if( !m_pObject )
				return;
			m_pObject->AddRef();
		}

		// for handwork grab;
		void Release() 
		{
			if( !m_pObject )
				return;
			m_pObject->Release();
		}

	private:
		T* m_pObject;
	};
}
