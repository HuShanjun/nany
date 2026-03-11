#pragma once
#include "GammaCommon/TCircleBuffer.h"
#include "GammaCommon/GammaTime.h"
#include "GammaCommon/CThread.h"

#include "GammaShm/CBaseShareMemory.h"
#include "GammaShm/IShareMemoryMgr.h"
#include "ShareMemoryHelp.h"

#include <list>
#include <set>

using namespace std;

namespace Gamma
{
	class CShareMemoryMgr 
		: public IShareMemoryMgr
		, public CTick
	{
		DEFAULT_METHOD( CShareMemoryMgr );
	public:
		CShareMemoryMgr( uint16_t nGasID, SShareCommonHead* pSrc,
			const char* szFileName, IShareMemoryHandler* pHandler,
			uint32_t nCommitInterval, bool bKeepShmFile, uint32_t nFlushInterval );
		virtual ~CShareMemoryMgr(void);

		SShareCommonHead*		GetShareMemory();
		void					Start();
		bool					IsReady();
		void					Check();
		void					SetCheckInterval( uint32_t nInterval );
		void					NotifyAppClose(){ m_bAppShutdown = true; };

		void					OnRebootFinish( bool bFailed );
		void					OnCommitFlagFinish( const SCommitFlag* pInfo, bool bFailed );
		void					OnCommitAllBlocksOKFinish( const SCommitFlag* pInfo, bool bFailed );
		void					OnCommitBlocksFinish( const SCommitBlocksInfo* pInfo, bool bFailed );

		///< 数据成员
	private:
		ILog*	m_pThreadLog;
		uint32_t	m_nSendCount;
		uint32_t	m_nStopTime;
		vector<uint32_t> m_vecNextQueryIndex;

		enum ECheckState
		{ 
			eCS_None,
			eCS_Create, 
			eCS_Reboot,
			eCS_Check, 
			eCS_Waiting,
			eCS_Normal 
		};

		struct SShareHead : public SShareCommonHead
		{
			SChunckTypeInfo		m_aryTrunkTypeInfo[0];
		};

		IShareMemoryHandler*	m_pHandler;
		uint16_t					m_nGasID;
		bool					m_bKeepShmFile;
		uint32_t					m_nCommitInterval;
		uint32_t					m_nFlushInterval;
		uint32_t					m_nCheckInterval;
		int64_t					m_nPreCheckTime;

		string					m_strShmFilePath;

		size_t					m_nMemorySize;
		HFILEMAP				m_hMemoryHandler;
		SShareHead*				m_pShareDesCommHead;
		SShareHead*				m_pShareSrcCommHead;
		bool					m_bAppShutdown;

		vector<SBlockInfo*>		m_vecCommitBlockInfo;
		string					m_szCommitBuf;

		//================================================================
		// 处理中的数据
		//================================================================
		string					m_szDiffData;
		vector<uint16_t>			m_vecDiffArrayIndex;
		uint16_t					m_nArrayIndexCount;
		uint32_t					m_nBlockCommitID;

		vector<STrunkTypeData>	m_vecTrunkTypeData;

		CCircleBuffer			m_QueryCmdBuffer;
		CCircleBuffer			m_FinishCmdBuffer;

		ECheckState				m_eCheckState;
		HTHREAD					m_hThreadCheck;

		void					OnFinished( ECmdType eType, bool bFailed, SSendBuf SendBuf );
		void					OnLoop();
		static void				RunCheck( void* pParam );

		// linux下需要手动更新到磁盘
#ifndef _WIN32 
		HTHREAD					m_hThreadSyn;
		bool					m_bSyn;
		static void				RunSync( void* pParam );
#endif

		void					PollInfo( uint32_t nTrunkType );
		size_t					GetCountInCommit( uint32_t nTrunkType, uint16_t nGasID );
		bool					HasOtherGasData( );

		CCommitMap&				GetCommitMap( uint32_t nTrunkType );
		void					FreeInfoOnEnd( uint32_t nTrunkType );
		bool					HasCommitID();

		uint32_t					BuildDiff( SBlockInfo* pBlockInfo );

		SChunckHead*			GetChunckHead(const SDBBlockInfo* pAnswer, bool bSucceeded, ECmdType eType );
		void					ProcessRebootGas();
		void					OnAnswerReboot( bool bSucceeded );
		void					OnAnswerCommitBlock( bool bSucceeded, const SCommitBlocksInfo* pAnswer );
		void					OnAnswerCommitFlg( bool bSucceeded, const SCommitFlag* pAnswer );

		void					CheckShareMemoryState();
		void					CheckResult();
		void					SendCmd( ECmdType eQueryType, uint16_t nGasID, int64_t uuid,
									const void* pData1, size_t nSize1, const void* pData2, size_t nSize2 );
	};
}