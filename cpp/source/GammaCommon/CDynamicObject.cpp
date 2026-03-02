
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
		uint32					m_nClassID;
		uint32					m_nObjectSize;
		CreateDynamicObjFun		m_pCreateFun;
		DestroyDynamicObjFun	m_pDestroyFun;

	public:
		CDynamicObjectAlloc( 
			uint32 nClassID, uint32 nObjectSize, uint32 nObjectPoolSize,
			CreateDynamicObjFun pCreateFun, DestroyDynamicObjFun pDestroyFun );
		operator uint32() const { return m_nClassID; }

		CDynamicObject*	Alloc( const void* pContext );
		void			Free( CDynamicObject* );
		uint32			GetUsedCount();
		uint32			GetFreeCount();
		uint32			GetClassSize();
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
		void					UnregisterClass( uint32 nClassID );
		CDynamicObjectAlloc*	GetRegisterClassAlloc( uint32 nClassID );
		CDynamicObject*			CreateObject( uint32 nClassID, const void* pContext );
		void					DestroyObject( CDynamicObject* pBaseObject );
		size_t					GetClassInstanceUsedCount( uint32 nClassID );
		uint32					GetAllocSize();
		void					DumpAllocSize();
	};

	/************************************************************************/
	/*  标准分配器                                                          */
	/************************************************************************/
	CDynamicObjectAlloc::CDynamicObjectAlloc( 
		uint32 nClassID, uint32 nObjectSize, uint32 nObjectPoolSize,
		CreateDynamicObjFun pCreateFun, DestroyDynamicObjFun pDestroyFun )
		: m_nClassID( nClassID )
		, m_nObjectSize( nObjectSize )
		, m_pCreateFun( pCreateFun )
		, m_pDestroyFun( pDestroyFun )
		, CClassPool( Max<uint32>( nObjectSize, sizeof(void*)*2 ), nObjectPoolSize )
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

	uint32 CDynamicObjectAlloc::GetUsedCount()
	{
		return (uint32)( CClassPool::GetTotalBlockCount() - CClassPool::GetFreeBlockCount() );
	}

	uint32 CDynamicObjectAlloc::GetFreeCount()
	{
		return (uint32)CClassPool::GetFreeBlockCount();
	}

	uint32 CDynamicObjectAlloc::GetClassSize()
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
		if( m_AllocList.Find( (uint32)*pAlloc ) == NULL )
			m_AllocList.Insert( *pAlloc );
	}

	void CBaseClassMgr::UnregisterClass( uint32 nClassID )
	{		
		CDynamicObjectAlloc* pAllock = m_AllocList.Find( nClassID );
		if( pAllock == NULL )
			return;
		pAllock->Remove();
		SAFE_DELETE( pAllock );
	}

	CDynamicObjectAlloc* CBaseClassMgr::GetRegisterClassAlloc( uint32 nClassID )
	{
		return m_AllocList.Find( nClassID );
	}

	CDynamicObject* CBaseClassMgr::CreateObject( uint32 nClassID, const void* pContext )
	{
		CDynamicObjectAlloc* pNode = m_AllocList.Find( nClassID );
		if( pNode == NULL )
			return NULL;
		return pNode->Alloc(pContext);
	}

	void CBaseClassMgr::DestroyObject( CDynamicObject* pBaseObject )
	{
		uint32 nClassID = pBaseObject->GetClassID();
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

	size_t CBaseClassMgr::GetClassInstanceUsedCount(  uint32 nClassID )
	{
		CDynamicObjectAlloc* pNode = m_AllocList.Find( nClassID );
		if( pNode == NULL )
			return 0;
		return pNode->GetUsedCount();
	}

	uint32 CBaseClassMgr::GetAllocSize()
	{
		uint32 nAllocSize = 0;
		for( CDynamicObjectAlloc* pNode = m_AllocList.GetFirst(); pNode; pNode = pNode->GetNext() )
		{
			uint32 nTotalCount = pNode->GetFreeCount() + pNode->GetUsedCount();
			nAllocSize += nTotalCount*pNode->GetClassSize();
		}
		return nAllocSize;
	}

	void CBaseClassMgr::DumpAllocSize()
	{
		FILE* fpM = fopen("DynamicObject.txt", "a");
		uint32 nTotalAllocSize = 0;
		for (CDynamicObjectAlloc* pNode = m_AllocList.GetFirst(); pNode; pNode = pNode->GetNext())
		{
			uint32 nTotalCount = pNode->GetFreeCount() + pNode->GetUsedCount();
			uint32 nAllocSize = nTotalCount * pNode->GetClassSize();
			fprintf(fpM, "Class(%d) AllocSize(%d) TotalCount(%d) FreeCount(%d) UsedCount(%d) ClassSiez(%d)\n", (uint32)(*pNode), nAllocSize, nTotalCount, pNode->GetFreeCount(), pNode->GetUsedCount(), pNode->GetClassSize());
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
	void CDynamicObject::RegisterClass( uint32 nClassID, uint32 nObjectSize, 
		uint32 nObjectPoolSize, CreateDynamicObjFun pCreateFun, DestroyDynamicObjFun pDestroyFun )
	{
		return CBaseClassMgr::Instance().RegisterClass( 
			new CDynamicObjectAlloc( nClassID, nObjectSize, nObjectPoolSize, pCreateFun, pDestroyFun ) );
	}

	void CDynamicObject::UnregisterClass( uint32 nClassID )
	{
		return CBaseClassMgr::Instance().UnregisterClass( nClassID );
	}

	CDynamicObject* CDynamicObject::CreateInstance( uint32 nClassID, const void* pContext )
	{
		return CBaseClassMgr::Instance().CreateObject( nClassID, pContext );
	}

	void CDynamicObject::DestroyInstance( CDynamicObject* pInstance )
	{
		return CBaseClassMgr::Instance().DestroyObject( pInstance );
	}

	size_t CDynamicObject::GetClassInstanceUsedCount( uint32 nClassID )
	{
		return CBaseClassMgr::Instance().GetClassInstanceUsedCount( nClassID );
	}	

	uint32 CDynamicObject::GetAllocSize()
	{
		return CBaseClassMgr::Instance().GetAllocSize();
	}

	void CDynamicObject::DumpAllocSize()
	{
		CBaseClassMgr::Instance().DumpAllocSize();
	}
}
