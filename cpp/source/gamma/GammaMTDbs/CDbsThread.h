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
		uint16_t					m_nDbsPort;
		std::string				m_szDataBase;
		std::string				m_szUser;
		std::string				m_szPassword;
		uint32_t					m_nDBId;
		bool					m_bUpdateAffectRowAsFound;
	};

	struct SQueryContext
	{
		uint32_t nQueryID;
		uint32_t nThreadID;
		uint32_t nPushTime;
		uint32_t nPopTime;
		uint32_t nOutDBTime;
		uint32_t nResultTime;
		bool bIsRecord;
	};

	typedef std::vector<SDbsThreadParam> CThreadParamList;
	typedef std::vector<IDbConnection*>	CDbConnectionList;
	typedef std::vector<std::string>	CResultBufferList;

	class CDbsThread : IResultHolder
	{
//debug
		uint32_t				m_nThreadIndex;
		uint32_t				m_nPushNum;
		uint32_t				m_nPopNum;

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
		volatile uint32_t		m_nHungryID;
		volatile uint32_t		m_nFeedID;

		uint32_t				m_nResultIndex;
		CResultBufferList	m_aryResultBuffer;
		bool				m_bIsRecord;

		static uint32_t		RunThread( void* pParam );
		uint32_t				Run();
		void				Execute();
		bool				ConnectDbs();

		void				Write( const void* pData, size_t nSize );
		void				Segment();
	public:
		CDbsThread( uint32_t nConnClassID, const SDbsCreateParam* aryParam, uint8_t nDbsCount, uint32_t nThreadIndex, bool bIsRecord = false );
		~CDbsThread(void);

		void				Querry( uint32_t nQueryID, const SSendBuf aryBuffer[], uint32_t nBufferCount );
		void				Record( const SSendBuf aryBuffer[], uint32_t nBufferCount );
		bool				GetResult(SQueryContext& context, SSendBuf aryBuffer[], uint32_t& nBufferCount );
		uint32_t				GetWaitingCmdCount() const;
		uint32_t				GetWaitingResultCount() const;
		void				CheckHungry();
	};
}
