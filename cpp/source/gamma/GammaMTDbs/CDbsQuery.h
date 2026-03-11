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
		uint32_t				m_nQueryID;
		uint32_t				m_nRef;
		IDbsQueryHandler*	m_pHandler;
		CDbsThreadMgr*		m_pDbsMgr;
	public:
		CDbsQuery( CDbsThreadMgr* pDbsMgr, 
			IDbsQueryHandler* pHandler, uint32_t nQueryID );
		~CDbsQuery();
		operator			uint32_t() const { return m_nQueryID; }
		IDbsQueryHandler*	GetHandler() const { return m_pHandler; }
		uint32_t				GetQueryID() const { return m_nQueryID; }
		void				SetQueryID( uint32_t nQueryID );
		uint32_t				GetRef() { return m_nRef; } 
		void				AddRef() { ++m_nRef; }
		void				Release();

		virtual void		Query( uint32_t nChannel, const SSendBuf aryBuffer[], uint32_t nBufferCount );
		virtual void		Query(uint32_t nChannel, const void* pData1, size_t nSize1);
		virtual void		Record(const void* pData, size_t nSize);

	};
}
