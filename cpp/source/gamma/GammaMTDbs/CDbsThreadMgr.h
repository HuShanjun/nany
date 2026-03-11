#pragma once
#include "GammaCommon/CAlloc.h"
#include "GammaCommon/GammaTime.h"
#include "GammaCommon/TGammaRBTree.h"
#include "GammaMTDbs/IDbsThreadMgr.h"
#include "GammaMTDbs/IDbsQuery.h"
#include "CDbsThread.h"
#include <vector>
#include <map>

namespace Gamma
{
	class CDbsQuery;
	class CQuerryCmd;
	typedef std::vector<CDbsThread*>	CThreadList;
	typedef TGammaRBTree<CDbsQuery>		CQueryList;
	typedef TFixSizeAlloc<0,0,false>	CQueryAlloc;

	class CDbsThreadMgr : public IDbsThreadMgr
	{
		DEFAULT_METHOD( CDbsThreadMgr );

		SSendBuf			m_aryResult[256];
		tbyte*				m_pFreeQueryBuffer;
		uint32_t				m_nQueryID;
		CQueryList			m_listQueryList;
		CThreadList			m_vecThread;
		CThreadList			m_vecRecordThread;

	public:
		CDbsThreadMgr( uint32_t nThreadCount, uint32_t nDbsConnClassID, 
			const SDbsCreateParam* aryParam, uint8_t nDbsCount,
			uint32_t nRecordThreadCount,const Gamma::SDbsCreateParam* _aryRecordParam, uint8_t _nRecordDbsCount);
		~CDbsThreadMgr(void);

		CDbsQuery*			GetQueryHandler( uint32_t nID );
		void				FreeDbsQuery( CDbsQuery* pDbQuery );
		void				Query( uint32_t nQueryID, uint32_t nChannel, 
								const SSendBuf aryBuffer[], uint32_t nBufferCount );
		void				Record(const SSendBuf aryBuffer[], uint32_t nBufferCount);

		IDbsQuery*			CreateDbsQuery( IDbsQueryHandler* pHandler );
		void				Check();
		uint32_t				GetChannelCmdWaitingCount( uint32_t nChannel );
		uint32_t				GetAllCmdWaitingCount();
		uint32_t				GetChannelResultWaitingCount( uint32_t nChannel );
		uint32_t				GetAllResultWaitingCount();
	};

}
