#pragma once
#include "GammaCommon/CThread.h"
#include "GammaCommon/TCircleBuffer.h"
#include "GammaDatabase/IDatabase.h"
#include "GammaMTDbs/IResultHolder.h"
#include <string>

namespace Gamma
{
	#define MAX_SEGMENT			256

	class CBaseDbsConn;
	struct SDbsCreateParam;

	struct SDbsThreadParam
	{
		std::string				m_szDbsHost;
		uint16					m_nDbsPort;
		std::string				m_szDataBase;
		std::string				m_szUser;
		std::string				m_szPassword;
		uint32					m_nDBId;
		bool					m_bUpdateAffectRowAsFound;
	};

	struct SQueryContext
	{
		uint32 nQueryID;
		uint32 nThreadID;
		uint32 nPushTime;
		uint32 nPopTime;
		uint32 nOutDBTime;
		uint32 nResultTime;
		bool bIsRecord;
	};

	typedef std::vector<SDbsThreadParam> CThreadParamList;
	typedef std::vector<IDbConnection*>	CDbConnectionList;
	typedef std::vector<std::string>	CResultBufferList;

	class CDbsThread : IResultHolder
	{
//debug
		uint32				m_nThreadIndex;
		uint32				m_nPushNum;
		uint32				m_nPopNum;

		HTHREAD				m_hThread;
		CThreadParamList	m_listParam;

		CBaseDbsConn*		m_pHandler;
		bool				m_bCreated;
		bool				m_bQuit;

		ILog*				m_pThreadLog;
		CDbConnectionList	m_vecDbsConnect;		

		CCircleBuffer		m_CmdBuffer;
		CCircleBuffer		m_ResultBuffer;

		HSEMAPHORE			m_hReadSemaphore;
		volatile uint32		m_nHungryID;
		volatile uint32		m_nFeedID;

		uint32				m_nResultIndex;
		CResultBufferList	m_aryResultBuffer;
		bool				m_bIsRecord;

		static uint32		RunThread( void* pParam );
		uint32				Run();
		void				Execute();
		bool				ConnectDbs();

		void				Write( const void* pData, size_t nSize );
		void				Segment();
	public:
		CDbsThread( uint32 nConnClassID, const SDbsCreateParam* aryParam, uint8 nDbsCount, uint32 nThreadIndex, bool bIsRecord = false );
		~CDbsThread(void);

		void				Querry( uint32 nQueryID, const SSendBuf aryBuffer[], uint32 nBufferCount );
		void				Record( const SSendBuf aryBuffer[], uint32 nBufferCount );
		bool				GetResult(SQueryContext& context, SSendBuf aryBuffer[], uint32& nBufferCount );
		uint32				GetWaitingCmdCount() const;
		uint32				GetWaitingResultCount() const;
		void				CheckHungry();
	};
}
