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
		uint32				m_nQueryID;
		CQueryList			m_listQueryList;
		CThreadList			m_vecThread;
		CThreadList			m_vecRecordThread;

	public:
		CDbsThreadMgr( uint32 nThreadCount, uint32 nDbsConnClassID, 
			const SDbsCreateParam* aryParam, uint8 nDbsCount,
			uint32 nRecordThreadCount,const Gamma::SDbsCreateParam* _aryRecordParam, uint8 _nRecordDbsCount);
		~CDbsThreadMgr(void);

		CDbsQuery*			GetQueryHandler( uint32 nID );
		void				FreeDbsQuery( CDbsQuery* pDbQuery );
		void				Query( uint32 nQueryID, uint32 nChannel, 
								const SSendBuf aryBuffer[], uint32 nBufferCount );
		void				Record(const SSendBuf aryBuffer[], uint32 nBufferCount);

		IDbsQuery*			CreateDbsQuery( IDbsQueryHandler* pHandler );
		void				Check();
		uint32				GetChannelCmdWaitingCount( uint32 nChannel );
		uint32				GetAllCmdWaitingCount();
		uint32				GetChannelResultWaitingCount( uint32 nChannel );
		uint32				GetAllResultWaitingCount();
	};

}
