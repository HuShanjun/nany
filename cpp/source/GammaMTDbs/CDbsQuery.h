#pragma once
#include "GammaCommon/GammaCommonType.h"
#include "GammaCommon/TGammaRBTree.h"
#include "GammaMTDbs/IDbsQuery.h"

namespace Gamma
{
	class CDbsThreadMgr;
	class CQuerryCmdValid;

	class CDbsQuery 
		: public IDbsQuery
		, public TGammaRBTree<CDbsQuery>::CGammaRBTreeNode
	{
		uint32				m_nQueryID;
		uint32				m_nRef;
		IDbsQueryHandler*	m_pHandler;
		CDbsThreadMgr*		m_pDbsMgr;
	public:
		CDbsQuery( CDbsThreadMgr* pDbsMgr, 
			IDbsQueryHandler* pHandler, uint32 nQueryID );
		~CDbsQuery();
		operator			uint32() const { return m_nQueryID; }
		IDbsQueryHandler*	GetHandler() const { return m_pHandler; }
		uint32				GetQueryID() const { return m_nQueryID; }
		void				SetQueryID( uint32 nQueryID );
		uint32				GetRef() { return m_nRef; } 
		void				AddRef() { ++m_nRef; }
		void				Release();

		virtual void		Query( uint32 nChannel, const SSendBuf aryBuffer[], uint32 nBufferCount );
		virtual void		Query(uint32 nChannel, const void* pData1, size_t nSize1);
		virtual void		Record(const void* pData, size_t nSize);

	};
}
