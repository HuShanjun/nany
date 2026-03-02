
#include "CDbsThreadMgr.h"
#include "CDbsQuery.h"

namespace Gamma
{
	CDbsQuery::CDbsQuery( CDbsThreadMgr* pDbsMgr, 
		IDbsQueryHandler* pHandler, uint32_t nQueryID )
		: m_pDbsMgr( pDbsMgr )
		, m_pHandler( pHandler )
		, m_nQueryID( nQueryID )
		, m_nRef( 1 )
	{
	}

	CDbsQuery::~CDbsQuery()
	{
	}

	void CDbsQuery::Release()
	{
		if( --m_nRef )
			return;
		m_pDbsMgr->FreeDbsQuery( this );
	}

	void CDbsQuery::SetQueryID( uint32_t nQueryID )
	{
		m_nQueryID = nQueryID;
	}

	void CDbsQuery::Query( uint32_t nChannel, const SSendBuf aryBuffer[], uint32_t nBufferCount )
	{
		m_pDbsMgr->Query( m_nQueryID, nChannel, aryBuffer, nBufferCount );
	}

	void CDbsQuery::Query( uint32_t nChannel, const void* pData1, size_t nSize1 )
	{
		SSendBuf SendBuff( pData1, nSize1 );
		m_pDbsMgr->Query( m_nQueryID, nChannel, &SendBuff, 1 );
	}

	void CDbsQuery::Record(const void* pData, size_t nSize)
	{
		SSendBuf SendBuff(pData, nSize);
		m_pDbsMgr->Record(&SendBuff, 1);
	}

}
