
#include "GammaCommon/CGammaRand.h"
#include "CDbsThreadMgr.h"
#include "CDbsQuery.h"

namespace Gamma
{
	GAMMA_MTDBS_API IDbsThreadMgr* CreateDbsThreadMgr( uint32_t nThreadCount, 
		uint32_t nDbsConnClassID, const SDbsCreateParam* aryParam, uint8_t nDbsCount,
		uint32_t nRecordThreadCount, const Gamma::SDbsCreateParam* _aryRecordParam, uint8_t _nRecordDbsCount)
	{
		GammaAst( nDbsCount );
		return new CDbsThreadMgr( nThreadCount, nDbsConnClassID, aryParam, nDbsCount, nRecordThreadCount, _aryRecordParam, _nRecordDbsCount);
	}

	GAMMA_MTDBS_API IDbsThreadMgr* CreateDbsThreadMgr( 
		uint32_t nThreadCount, uint32_t nDbsConnClassID, 
		const char* szDbsHost, uint16_t nDbsPort, const char* szDatabase, 
		const char* szUser, const char* szPassword, bool bFoundAsUpdateRow )
	{
		SDbsCreateParam Param;
		Param.szDbsHost = szDbsHost;
		Param.nDbsPort = nDbsPort;
		Param.szDataBase = szDatabase;
		Param.szUser = szUser;
		Param.szPassword = szPassword;
		Param.bFoundAsUpdateRow = bFoundAsUpdateRow;
		return new CDbsThreadMgr( nThreadCount, nDbsConnClassID, &Param, 1, 0, nullptr, 0 );
	}

	CDbsThreadMgr::CDbsThreadMgr( uint32_t nThreadCount, 
		uint32_t nDbsConnClassID, const SDbsCreateParam* aryParam, uint8_t nDbsCount,
		uint32_t nRecordThreadCount, const Gamma::SDbsCreateParam* _aryRecordParam, uint8_t _nRecordDbsCount)
		: m_pFreeQueryBuffer( NULL )
		, m_nRef( 1 )
		, m_nQueryID( 1 )
	{
		m_vecThread.resize( nThreadCount );
		for( size_t i = 0; i < m_vecThread.size(); i++ )
			m_vecThread[i] = new CDbsThread(nDbsConnClassID, aryParam, nDbsCount, i);
		// 初始化uuid时 会跟连接顺序相关
		m_vecRecordThread.resize(nRecordThreadCount);
		for (size_t i = 0; i < m_vecRecordThread.size(); i++)
			m_vecRecordThread[i] = new CDbsThread(nDbsConnClassID, _aryRecordParam, _nRecordDbsCount, i, true);
	}

	CDbsThreadMgr::~CDbsThreadMgr(void)
	{
		Check();
		for( uint32_t i = 0; i < (uint32_t)m_vecThread.size(); i++ )
			delete m_vecThread[i];
		m_vecThread.clear();
		for (uint32_t i = 0; i < (uint32_t)m_vecRecordThread.size(); i++)
			delete m_vecRecordThread[i];
		m_vecRecordThread.clear();
	}
	
	CDbsQuery* CDbsThreadMgr::GetQueryHandler( uint32_t nID )
	{
		return m_listQueryList.Find( nID );
	}

	void CDbsThreadMgr::Query( uint32_t nQueryID, uint32_t nChannel, 
		const SSendBuf aryBuffer[], uint32_t nBufferCount )
	{
		nChannel = nChannel%m_vecThread.size();
		m_vecThread[nChannel]->Querry( nQueryID, aryBuffer, nBufferCount );
	}

	void CDbsThreadMgr::Record(const SSendBuf aryBuffer[], uint32_t nBufferCount)
	{
		m_vecRecordThread[CGammaRand::Rand(size_t(0), m_vecRecordThread.size())]->Record(aryBuffer, nBufferCount);
	}

	void CDbsThreadMgr::Check()
	{
		for( uint32_t i = 0; i < (uint32_t)m_vecThread.size(); i++ )
		{
			CDbsThread* pThread = m_vecThread[i];
			pThread->CheckHungry();

			while( true )
			{
				uint32_t nResultCount = ELEM_COUNT( m_aryResult );
				SQueryContext context;
				if( !pThread->GetResult(context, m_aryResult, nResultCount ) )
					break;
				context.nResultTime = GetProcessTime();
				if( !nResultCount )
					continue;
				uint32_t nCheck = context.nResultTime - context.nPushTime;
				if(nCheck > 2000 )
					GammaLog << "[CCharDataMgr] todo:" << nCheck << std::endl;

				CDbsQuery* pQuery = GetQueryHandler(context.nQueryID);
				if( pQuery )
				{
					IDbsQueryHandler* pHandler = pQuery->GetHandler();
					for( uint32_t i = 0; i < nResultCount && pHandler; ++i )
					{
						if( m_aryResult[i].second == 0 )
							continue;
						pHandler->OnQueryResult( m_aryResult[i].first, m_aryResult[i].second );
					}
				}
			}
		}
	}

	IDbsQuery* CDbsThreadMgr::CreateDbsQuery( IDbsQueryHandler* pHandler )
	{
		uint32_t nCurID = m_nQueryID++;
		void* pBuffer = m_pFreeQueryBuffer;
		uint32_t nSize = TAligenUp<sizeof( CDbsQuery ), sizeof(void*)>::eValue;
		if( pBuffer )
			m_pFreeQueryBuffer = *(tbyte**)( m_pFreeQueryBuffer + nSize - sizeof(void*) );
		else
			pBuffer = new tbyte[sizeof( CDbsQuery )];

		CDbsQuery* pQuery = new( pBuffer ) CDbsQuery( this, pHandler, nCurID );
		m_listQueryList.Insert( *pQuery );
		return pQuery;
	}

	void CDbsThreadMgr::FreeDbsQuery( CDbsQuery* pDbQuery )
	{
		pDbQuery->~CDbsQuery();
		uint32_t nSize = TAligenUp<sizeof( CDbsQuery ), sizeof(void*)>::eValue;
		*(tbyte**)( ( (tbyte*)pDbQuery ) + nSize - sizeof(void*) ) = m_pFreeQueryBuffer;
		m_pFreeQueryBuffer = (tbyte*)pDbQuery;
	}

	uint32_t CDbsThreadMgr::GetChannelCmdWaitingCount( uint32_t nChannel )
	{
		nChannel = nChannel%m_vecThread.size();
		return m_vecThread[nChannel]->GetWaitingCmdCount();
	}

	uint32_t CDbsThreadMgr::GetAllCmdWaitingCount()
	{
		uint32_t nCount = 0;
		for( uint32_t i = 0; i < m_vecThread.size(); i++ )
			nCount += m_vecThread[i]->GetWaitingCmdCount();
		return nCount;
	}

	uint32_t CDbsThreadMgr::GetChannelResultWaitingCount( uint32_t nChannel )
	{
		nChannel = nChannel%m_vecThread.size();
		return m_vecThread[nChannel]->GetWaitingResultCount();
	}

	uint32_t CDbsThreadMgr::GetAllResultWaitingCount()
	{
		uint32_t nCount = 0;
		for( uint32_t i = 0; i < m_vecThread.size(); i++ )
			nCount += m_vecThread[i]->GetWaitingResultCount();
		return nCount;
	}
}
