
#include <stdlib.h>
#include "GammaCommon/CDynamicObject.h"
#include "GammaCommon/TGammaRBTree.h"
#include "GammaCommon/CAlloc.h"

namespace Gamma
{
	class CDynamicObjectAlloc;
	typedef TFixSizeAlloc<0, 0, false, false> CClassPool;
	typedef TGammaRBTree<CDynamicObjectAlloc> CAllocList;

	class CDynamicObjectAlloc 
		: public CClassPool
		, public CAllocList::CGammaRBTreeNode
	{
		uint32_t					m_nClassID;
		uint32_t					m_nObjectSize;
		CreateDynamicObjFun		m_pCreateFun;
		DestroyDynamicObjFun	m_pDestroyFun;

	public:
		CDynamicObjectAlloc( 
			uint32_t nClassID, uint32_t nObjectSize, uint32_t nObjectPoolSize,
			CreateDynamicObjFun pCreateFun, DestroyDynamicObjFun pDestroyFun );
		operator uint32_t() const { return m_nClassID; }

		CDynamicObject*	Alloc( const void* pContext );
		void			Free( CDynamicObject* );
		uint32_t			GetUsedCount();
		uint32_t			GetFreeCount();
		uint32_t			GetClassSize();
	};

	/************************************************************************/
	/*  CBaseClassMgr里不能使用new来分配空间，不然自己了断。。。            */
	/************************************************************************/
	class CBaseClassMgr
	{	
		CAllocList				m_AllocList;

		CBaseClassMgr();
		~CBaseClassMgr();
	public:

		static CBaseClassMgr&	Instance();

		void					RegisterClass( CDynamicObjectAlloc* pAlloc );
		void					UnregisterClass( uint32_t nClassID );
		CDynamicObjectAlloc*	GetRegisterClassAlloc( uint32_t nClassID );
		CDynamicObject*			CreateObject( uint32_t nClassID, const void* pContext );
		void					DestroyObject( CDynamicObject* pBaseObject );
		size_t					GetClassInstanceUsedCount( uint32_t nClassID );
		uint32_t					GetAllocSize();
		void					DumpAllocSize();
	};

	/************************************************************************/
	/*  标准分配器                                                          */
	/************************************************************************/
	CDynamicObjectAlloc::CDynamicObjectAlloc( 
		uint32_t nClassID, uint32_t nObjectSize, uint32_t nObjectPoolSize,
		CreateDynamicObjFun pCreateFun, DestroyDynamicObjFun pDestroyFun )
		: m_nClassID( nClassID )
		, m_nObjectSize( nObjectSize )
		, m_pCreateFun( pCreateFun )
		, m_pDestroyFun( pDestroyFun )
		, CClassPool( Max<uint32_t>( nObjectSize, sizeof(void*)*2 ), nObjectPoolSize )
	{
	}

	CDynamicObject* CDynamicObjectAlloc::Alloc( const void* pContext )
	{
		void* pBuf = CClassPool::Alloc();
		return m_pCreateFun( pBuf, pContext );
	}

	void CDynamicObjectAlloc::Free( CDynamicObject* pBaseObject )
	{
		CClassPool::Free( m_pDestroyFun( pBaseObject ) );
	}

	uint32_t CDynamicObjectAlloc::GetUsedCount()
	{
		return (uint32_t)( CClassPool::GetTotalBlockCount() - CClassPool::GetFreeBlockCount() );
	}

	uint32_t CDynamicObjectAlloc::GetFreeCount()
	{
		return (uint32_t)CClassPool::GetFreeBlockCount();
	}

	uint32_t CDynamicObjectAlloc::GetClassSize()
	{
		return m_nObjectSize;
	}

	CBaseClassMgr& CBaseClassMgr::Instance()
	{
		static CBaseClassMgr _instance;
		return _instance;
	}

	CBaseClassMgr::CBaseClassMgr()
	{	
	}

	CBaseClassMgr::~CBaseClassMgr()
	{
		try
		{
			while( m_AllocList.GetFirst() )
			{
				CDynamicObjectAlloc* pAlloc = m_AllocList.GetFirst();
				pAlloc->Remove();
				SAFE_DELETE( pAlloc );
			}
		}
		catch(...)
		{
			// TODO:lye
		}
	}

	void CBaseClassMgr::RegisterClass( CDynamicObjectAlloc* pAlloc )
	{
		if( m_AllocList.Find( (uint32_t)*pAlloc ) == NULL )
			m_AllocList.Insert( *pAlloc );
	}

	void CBaseClassMgr::UnregisterClass( uint32_t nClassID )
	{		
		CDynamicObjectAlloc* pAllock = m_AllocList.Find( nClassID );
		if( pAllock == NULL )
			return;
		pAllock->Remove();
		SAFE_DELETE( pAllock );
	}

	CDynamicObjectAlloc* CBaseClassMgr::GetRegisterClassAlloc( uint32_t nClassID )
	{
		return m_AllocList.Find( nClassID );
	}

	CDynamicObject* CBaseClassMgr::CreateObject( uint32_t nClassID, const void* pContext )
	{
		CDynamicObjectAlloc* pNode = m_AllocList.Find( nClassID );
		if( pNode == NULL )
			return NULL;
		return pNode->Alloc(pContext);
	}

	void CBaseClassMgr::DestroyObject( CDynamicObject* pBaseObject )
	{
		uint32_t nClassID = pBaseObject->GetClassID();
		CDynamicObjectAlloc* pNode = m_AllocList.Find( nClassID );

		if( !pNode )
		{
			GammaErr << " can't find class ID: " << nClassID << std::endl;
			return;
		}

		pNode->Free( pBaseObject );
		// 将虚表置空，表示对象已经失效
		*(void**)pBaseObject = nullptr;
	}

	size_t CBaseClassMgr::GetClassInstanceUsedCount(  uint32_t nClassID )
	{
		CDynamicObjectAlloc* pNode = m_AllocList.Find( nClassID );
		if( pNode == NULL )
			return 0;
		return pNode->GetUsedCount();
	}

	uint32_t CBaseClassMgr::GetAllocSize()
	{
		uint32_t nAllocSize = 0;
		for( CDynamicObjectAlloc* pNode = m_AllocList.GetFirst(); pNode; pNode = pNode->GetNext() )
		{
			uint32_t nTotalCount = pNode->GetFreeCount() + pNode->GetUsedCount();
			nAllocSize += nTotalCount*pNode->GetClassSize();
		}
		return nAllocSize;
	}

	void CBaseClassMgr::DumpAllocSize()
	{
		FILE* fpM = fopen("DynamicObject.txt", "a");
		uint32_t nTotalAllocSize = 0;
		for (CDynamicObjectAlloc* pNode = m_AllocList.GetFirst(); pNode; pNode = pNode->GetNext())
		{
			uint32_t nTotalCount = pNode->GetFreeCount() + pNode->GetUsedCount();
			uint32_t nAllocSize = nTotalCount * pNode->GetClassSize();
			fprintf(fpM, "Class(%d) AllocSize(%d) TotalCount(%d) FreeCount(%d) UsedCount(%d) ClassSiez(%d)\n", (uint32_t)(*pNode), nAllocSize, nTotalCount, pNode->GetFreeCount(), pNode->GetUsedCount(), pNode->GetClassSize());
			nTotalAllocSize += nAllocSize;
		}
		fprintf(fpM, "==============TotalSize(%d)==============\n", nTotalAllocSize);
		if (fpM)
			fclose(fpM);
	}

	bool CDynamicObject::IsValid() const
	{
		return this && *(const void**)this != nullptr;
	}

	//====================================================================
	//
	//====================================================================
	void CDynamicObject::RegisterClass( uint32_t nClassID, uint32_t nObjectSize, 
		uint32_t nObjectPoolSize, CreateDynamicObjFun pCreateFun, DestroyDynamicObjFun pDestroyFun )
	{
		return CBaseClassMgr::Instance().RegisterClass( 
			new CDynamicObjectAlloc( nClassID, nObjectSize, nObjectPoolSize, pCreateFun, pDestroyFun ) );
	}

	void CDynamicObject::UnregisterClass( uint32_t nClassID )
	{
		return CBaseClassMgr::Instance().UnregisterClass( nClassID );
	}

	CDynamicObject* CDynamicObject::CreateInstance( uint32_t nClassID, const void* pContext )
	{
		return CBaseClassMgr::Instance().CreateObject( nClassID, pContext );
	}

	void CDynamicObject::DestroyInstance( CDynamicObject* pInstance )
	{
		return CBaseClassMgr::Instance().DestroyObject( pInstance );
	}

	size_t CDynamicObject::GetClassInstanceUsedCount( uint32_t nClassID )
	{
		return CBaseClassMgr::Instance().GetClassInstanceUsedCount( nClassID );
	}	

	uint32_t CDynamicObject::GetAllocSize()
	{
		return CBaseClassMgr::Instance().GetAllocSize();
	}

	void CDynamicObject::DumpAllocSize()
	{
		CBaseClassMgr::Instance().DumpAllocSize();
	}
}
