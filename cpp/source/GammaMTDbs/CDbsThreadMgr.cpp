
#include "GammaCommon/CGammaRand.h"
#include "CDbsThreadMgr.h"
#include "CDbsQuery.h"

namespace Gamma
{
	GAMMA_MTDBS_API IDbsThreadMgr* CreateDbsThreadMgr( uint32 nThreadCount, 
		uint32 nDbsConnClassID, const SDbsCreateParam* aryParam, uint8 nDbsCount,
		uint32 nRecordThreadCount, const Gamma::SDbsCreateParam* _aryRecordParam, uint8 _nRecordDbsCount)
	{
		GammaAst( nDbsCount );
		return new CDbsThreadMgr( nThreadCount, nDbsConnClassID, aryParam, nDbsCount, nRecordThreadCount, _aryRecordParam, _nRecordDbsCount);
	}

	GAMMA_MTDBS_API IDbsThreadMgr* CreateDbsThreadMgr( 
		uint32 nThreadCount, uint32 nDbsConnClassID, 
		const char* szDbsHost, uint16 nDbsPort, const char* szDatabase, 
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

	CDbsThreadMgr::CDbsThreadMgr( uint32 nThreadCount, 
		uint32 nDbsConnClassID, const SDbsCreateParam* aryParam, uint8 nDbsCount,
		uint32 nRecordThreadCount, const Gamma::SDbsCreateParam* _aryRecordParam, uint8 _nRecordDbsCount)
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
		for( uint32 i = 0; i < (uint32)m_vecThread.size(); i++ )
			delete m_vecThread[i];
		m_vecThread.clear();
		for (uint32 i = 0; i < (uint32)m_vecRecordThread.size(); i++)
			delete m_vecRecordThread[i];
		m_vecRecordThread.clear();
	}
	
	CDbsQuery* CDbsThreadMgr::GetQueryHandler( uint32 nID )
	{
		return m_listQueryList.Find( nID );
	}

	void CDbsThreadMgr::Query( uint32 nQueryID, uint32 nChannel, 
		const SSendBuf aryBuffer[], uint32 nBufferCount )
	{
		nChannel = nChannel%m_vecThread.size();
		m_vecThread[nChannel]->Querry( nQueryID, aryBuffer, nBufferCount );
	}

	void CDbsThreadMgr::Record(const SSendBuf aryBuffer[], uint32 nBufferCount)
	{
		m_vecRecordThread[CGammaRand::Rand(size_t(0), m_vecRecordThread.size())]->Record(aryBuffer, nBufferCount);
	}

	void CDbsThreadMgr::Check()
	{
		for( uint32 i = 0; i < (uint32)m_vecThread.size(); i++ )
		{
			CDbsThread* pThread = m_vecThread[i];
			pThread->CheckHungry();

			while( true )
			{
				uint32 nResultCount = ELEM_COUNT( m_aryResult );
				SQueryContext context;
				if( !pThread->GetResult(context, m_aryResult, nResultCount ) )
					break;
				context.nResultTime = GetProcessTime();
				if( !nResultCount )
					continue;
				uint32 nCheck = context.nResultTime - context.nPushTime;
				if(nCheck > 2000 )
					GammaLog << "[CCharDataMgr] todo:" << nCheck << std::endl;

				CDbsQuery* pQuery = GetQueryHandler(context.nQueryID);
				if( pQuery )
				{
					IDbsQueryHandler* pHandler = pQuery->GetHandler();
					for( uint32 i = 0; i < nResultCount && pHandler; ++i )
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
		uint32 nCurID = m_nQueryID++;
		void* pBuffer = m_pFreeQueryBuffer;
		uint32 nSize = TAligenUp<sizeof( CDbsQuery ), sizeof(void*)>::eValue;
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
		uint32 nSize = TAligenUp<sizeof( CDbsQuery ), sizeof(void*)>::eValue;
		*(tbyte**)( ( (tbyte*)pDbQuery ) + nSize - sizeof(void*) ) = m_pFreeQueryBuffer;
		m_pFreeQueryBuffer = (tbyte*)pDbQuery;
	}

	uint32 CDbsThreadMgr::GetChannelCmdWaitingCount( uint32 nChannel )
	{
		nChannel = nChannel%m_vecThread.size();
		return m_vecThread[nChannel]->GetWaitingCmdCount();
	}

	uint32 CDbsThreadMgr::GetAllCmdWaitingCount()
	{
		uint32 nCount = 0;
		for( uint32 i = 0; i < m_vecThread.size(); i++ )
			nCount += m_vecThread[i]->GetWaitingCmdCount();
		return nCount;
	}

	uint32 CDbsThreadMgr::GetChannelResultWaitingCount( uint32 nChannel )
	{
		nChannel = nChannel%m_vecThread.size();
		return m_vecThread[nChannel]->GetWaitingResultCount();
	}

	uint32 CDbsThreadMgr::GetAllResultWaitingCount()
	{
		uint32 nCount = 0;
		for( uint32 i = 0; i < m_vecThread.size(); i++ )
			nCount += m_vecThread[i]->GetWaitingResultCount();
		return nCount;
	}
}
