
#include "GammaCommon/CThread.h"
#include "GammaCommon/GammaFileMap.h"
#include "GammaCommon/ILog.h"
#include "GammaCommon/CPkgFile.h"
#include "GammaCommon/TBitSet.h"
#include "GammaCommon/CPathMgr.h"
#include "CShareMemoryMgr.h"
#include <ctime>


#define CHECK_CREATE_INTERVAL		1000
#define CHECK_STATE_INTERVAL		33
const uint32 DEFAULT_COMMIT_INTERVER = 5000;

namespace Gamma
{
	IShareMemoryMgr* CreateShareMemoryMgr( uint16 nGasID,
		SShareCommonHead* pSrc, const char* szFileName, IShareMemoryHandler* pHandler,
		uint32 nCommitInterval, bool bKeepShmFile, uint32 nFlushInterval )
	{
		return new CShareMemoryMgr( nGasID, pSrc, szFileName, pHandler,
			nCommitInterval, bKeepShmFile, nFlushInterval );
	}

	CShareMemoryMgr::CShareMemoryMgr( uint16 nGasID, SShareCommonHead* pSrc, 
		const char* szFileName, IShareMemoryHandler* pHandler, 
		uint32 nCommitInterval, bool bKeepShmFile, uint32 nFlushInterval )
		: m_nRef( 1 )
		, m_nGasID( nGasID )
		, m_pHandler( pHandler )
		, m_bKeepShmFile( bKeepShmFile )
		, m_nCommitInterval( nCommitInterval )
		, m_nFlushInterval( nFlushInterval )
		, m_nCheckInterval( CHECK_STATE_INTERVAL )
		, m_pShareDesCommHead(NULL)
		, m_pShareSrcCommHead(NULL)
		, m_nMemorySize( 0 )
		, m_bAppShutdown( false )
		, m_vecDiffArrayIndex( 65535 )
		, m_nBlockCommitID(1)
		, m_eCheckState( eCS_None )
		, m_hThreadCheck( NULL )
		, m_nPreCheckTime( 0 )
		, m_nSendCount( 0 )
		, m_nStopTime( 0 )
#ifndef _WIN32 
		, m_bSyn(false)
		, m_hThreadSyn( NULL )
#endif
	{
		static int32 nThreadLogIndex = 0;
		char szLogFileName[256];
		gammasstream(szLogFileName) << "ShmThread_" << nThreadLogIndex++;
		m_pThreadLog = GetLogFile(szLogFileName, GammaGetCurrentProcessID(), eLPT_All);
		if (!m_pThreadLog)
			GammaThrow("Can not create thread log!!!!");

		m_eCheckState = eCS_Create;
		m_pShareSrcCommHead = (SShareHead*)pSrc;
		m_vecTrunkTypeData.resize( m_pShareSrcCommHead->m_nChunkTypeCount );
		m_vecNextQueryIndex.resize(m_pShareSrcCommHead->m_nChunkTypeCount);

		m_strShmFilePath = GammaString( szFileName );
		m_hMemoryHandler = GammaMemoryMap( szFileName, false, 0, INVALID_32BITID, 0 );
		if( !m_hMemoryHandler )
		{
			string strPath;
			string::size_type nPos = m_strShmFilePath.find_last_of( '/' );
			if( nPos != string::npos )
				strPath = m_strShmFilePath.substr( 0, nPos + 1 );
			CPathMgr::MakeDirectory( strPath.c_str() );

			uint32 nCreateSize = m_pShareSrcCommHead->m_nSizeOfShareFlie;
			m_hMemoryHandler = GammaMemoryMap( szFileName, false, 0, nCreateSize, nCreateSize );
			m_pShareDesCommHead	= (SShareHead*)GammaGetMapMemory( m_hMemoryHandler );
			memcpy( m_pShareDesCommHead, m_pShareSrcCommHead, nCreateSize );
		}
		else
		{
			//====================================================================================
			// 如果shm已经存在，那么需要检查shm版本是否和当前程序版本一致，如果不一致不允许启动
			//====================================================================================
			m_pShareDesCommHead	= (SShareHead*)GammaGetMapMemory( m_hMemoryHandler );
			if( m_pShareDesCommHead->m_nVersion != m_pShareSrcCommHead->m_nVersion )
				GammaThrow( "share memory version incorrect!!!!" );
			uint32 nOrgFileSize = GammaGetMapMemorySize( m_hMemoryHandler );
			if( m_pShareSrcCommHead->m_nSizeOfShareFlie != nOrgFileSize || 
				m_pShareDesCommHead->m_nSizeOfShareFlie != nOrgFileSize )
				GammaThrow( "share memory size incorrect!!!!" );
		}
	}

	CShareMemoryMgr::~CShareMemoryMgr(void)
	{
		m_bAppShutdown = true;
		if( m_hThreadCheck )
			GammaJoinThread( m_hThreadCheck );

		m_pShareSrcCommHead = NULL;
		if( !m_pShareDesCommHead )
			return;

		for( uint32 i = 0; i < m_vecTrunkTypeData.size(); i++ )
			FreeInfoOnEnd( i );

#ifndef _WIN32 
		if( m_hThreadSyn )
		{
			m_bSyn = false;
			GammaJoinThread( m_hThreadSyn );
		}
#endif

		GammaMemoryUnMap( m_hMemoryHandler );

		if( !m_bKeepShmFile )
			CPathMgr::DeleteFile( m_strShmFilePath.c_str() );


		uint32 nCount = 1;
		SSendBuf SendBuf;
		SShmCmdHead CmdHead;
		GammaAst( !m_QueryCmdBuffer.PopBuffer( CmdHead, &SendBuf, nCount ) );
		GammaAst( !m_FinishCmdBuffer.PopBuffer( CmdHead, &SendBuf, nCount ) );

		SAFE_RELEASE(m_pThreadLog);
	}

	SShareCommonHead* CShareMemoryMgr::GetShareMemory()
	{
		return m_pShareDesCommHead;
	}

	void CShareMemoryMgr::SetCheckInterval( uint32 nInterval )
	{
		m_nCheckInterval = nInterval;
	}

	void CShareMemoryMgr::Start()
	{
		GammaLog << "CShareMemoryMgr::Start" << endl;
		if( m_eCheckState != eCS_Create )
			return;

		uint32 nCommitDataNewCount = 0;
		for( uint32 nTrunkType = 0; nTrunkType < m_vecTrunkTypeData.size(); nTrunkType++ )
		{
			SChunckTypeInfo& TrunkTypeInfo = m_pShareDesCommHead->m_aryTrunkTypeInfo[nTrunkType];
			m_vecTrunkTypeData[nTrunkType].m_nDBCost = 0;
			m_vecNextQueryIndex[nTrunkType] = 0;

			tbyte* pInfoStart = (tbyte*)( &TrunkTypeInfo ) + TrunkTypeInfo.m_nChunkOffset;
			for ( uint32 m = 0; m < TrunkTypeInfo.m_nChunkCount; m++, pInfoStart += TrunkTypeInfo.m_nChunkSize )
			{
				SChunckHead* pTrunkHead = (SChunckHead*)pInfoStart;
				CCommitMap& mapCommit = GetCommitMap(nTrunkType);
				mapCommit[pTrunkHead] = 0;

				SBlockInfo* pBlockInfo	= (SBlockInfo*)( ((tbyte*)pTrunkHead) + pTrunkHead->m_nChunkHeadSize );
				for( uint32 i = 0; i < pTrunkHead->m_nBlockCount; i++, pBlockInfo++, nCommitDataNewCount++ )
				{
					uint32 nCommitDataSize = sizeof( SCommitData );
					if( pBlockInfo->m_nArrayElemSize )
					{
						uint32 nDataSize = pBlockInfo->m_nMaxBlockSize;
						uint32 nArrayElemSize = pBlockInfo->m_nArrayElemSize;
						uint32 nArrayOffset = pBlockInfo->m_nArrayStartOff;
						uint32 nArrayDataSize = nDataSize - nArrayOffset;
						uint32 nMaxElemCount = ( nArrayDataSize - sizeof(SSizeType) )/nArrayElemSize;
						uint32 nPreCommitFlagBufSize = ( nMaxElemCount - 1 )/8 + 1;
						nCommitDataSize += nArrayDataSize + nPreCommitFlagBufSize;
					}
					pBlockInfo->m_pContextData = new( new tbyte[nCommitDataSize] ) SCommitData();
				}
				if( pTrunkHead->m_nChunckID != INVALID_64BITID )
					pTrunkHead->m_nDataState = eSCT_UpdateTrunk;
			}
		}

		GammaLog << "CShareMemoryMgr CommitDataNewCount = " << nCommitDataNewCount << endl;
		m_eCheckState = eCS_Reboot;
		m_nPreCheckTime = 0;

		GammaCreateThread( &m_hThreadCheck, 1024, (THREADPROC)&CShareMemoryMgr::RunCheck, this );

#ifndef _WIN32
		if( m_nFlushInterval != INVALID_32BITID )
		{
			m_bSyn = true;
			GammaCreateThread( &m_hThreadSyn, 1024, (THREADPROC)&CShareMemoryMgr::RunSync, this );
		}
#endif

	}

	bool CShareMemoryMgr::IsReady()
	{
		for( uint32 i = 0; i < m_vecTrunkTypeData.size(); i++ )
		{
			SChunckTypeInfo& TrunkTypeInfo = m_pShareDesCommHead->m_aryTrunkTypeInfo[i];
			tbyte* pInfoStart = (tbyte*)( &TrunkTypeInfo ) + TrunkTypeInfo.m_nChunkOffset;
			for( uint32 n = 0; n < TrunkTypeInfo.m_nChunkCount; n++, pInfoStart += TrunkTypeInfo.m_nChunkSize )
			{
				SChunckHead* pTrunkHead = (SChunckHead*)pInfoStart;
				if( pTrunkHead->m_nChunckID != INVALID_64BITID )
					return false;
			}
		}
		return true;
	}

	void CShareMemoryMgr::RunCheck( void* pParam )
	{
		GammaSetThreadName("ShmCheck");
		CShareMemoryMgr* pMgr = static_cast<CShareMemoryMgr*>( pParam );
		while( pMgr->m_eCheckState )
		{
			int64 nStartTime = GetGammaTime();
			pMgr->OnLoop();

			int64 nDeltaTime = GetGammaTime() - nStartTime;
			if( nDeltaTime >= pMgr->m_nCheckInterval )
				continue;
			GammaSleep( (uint32)( pMgr->m_nCheckInterval - nDeltaTime ) );
		}
	}

	void CShareMemoryMgr::OnLoop()
	{
		switch( m_eCheckState )
		{
		case eCS_Reboot:
			{
				bool bNeedReboot = true;
				for( uint32 i = 0; i < m_vecTrunkTypeData.size(); i++ )
				{
					size_t nCount = GetCountInCommit( i, m_nGasID );
					if( !nCount )
						continue;
					GammaLog << "Type(" << i << ") In Commit:" << nCount << endl;
					bNeedReboot = false;
				}
				if( bNeedReboot )
				{
					ProcessRebootGas();
					m_eCheckState = eCS_Waiting;
				}
				else
					CheckShareMemoryState();
			}
			break;
		case eCS_Check:
			CheckShareMemoryState();
			break;
		default:
			break;
		}
		CheckResult();

		if( m_bAppShutdown )
		{
			for( uint32 i = 0; i < m_vecTrunkTypeData.size(); i++ )
			{
				size_t nCount = GetCountInCommit( i, m_nGasID );
				if( nCount )
					return;
			}
			m_eCheckState = eCS_None;
		}
	}

	void CShareMemoryMgr::CheckShareMemoryState()
	{
		// 单项提交，比如某个玩家数据改变
		for( uint32 i = 0; i < m_vecTrunkTypeData.size(); i++ )
			PollInfo( i );
	}

#ifndef _WIN32 
	void CShareMemoryMgr::RunSync( void *pParam )
	{
		GammaSetThreadName("ShmSync");
		CShareMemoryMgr* pConn = reinterpret_cast<CShareMemoryMgr*>( pParam );
		uint32 nFlushInterval = pConn->m_nFlushInterval;

		while( pConn->m_bSyn )
		{
			int64 nBeginTime = GetGammaTime();
			GammaMemoryMapSyn( pConn->m_hMemoryHandler, 0, pConn->m_nMemorySize, false );

			while( pConn->m_bSyn && GetGammaTime() - nBeginTime < nFlushInterval )
				GammaSleep( 100 );
		}
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	void CShareMemoryMgr::Check()
	{
		SSendBuf SendBuf;
		SShmCmdHead CmdHead;

		while( true )
		{
			uint32 nCount = 1;
			if( !m_QueryCmdBuffer.PopBuffer( CmdHead, &SendBuf, nCount ) )
				return;

			const void* pData = SendBuf.first;
			size_t nSize = SendBuf.second;
			if( CmdHead.m_nType == eQueryType_CommitAllBlocksOK )
				m_pHandler->OnCommitAllBlocksOK( (const SCommitFlag*)pData, nSize );
			else if( CmdHead.m_nType == eQueryType_CommitFlag )
				m_pHandler->OnCommitFlag( (const SCommitFlag*)pData, nSize );
			else if( CmdHead.m_nType == eQueryType_CommitBlock )
				m_pHandler->OnCommitBlocksInfo( (const SCommitBlocksInfo*)pData, nSize );
			else if( CmdHead.m_nType == eQueryType_ShmReboot )
				m_pHandler->OnReboot();
		}
	}

	void CShareMemoryMgr::OnRebootFinish( bool bFailed )
	{
		OnFinished( eQueryType_ShmReboot, bFailed, SSendBuf() );
		if( bFailed )
			return;
	}

	void CShareMemoryMgr::OnCommitFlagFinish( const SCommitFlag* pInfo, bool bFailed )
	{
		OnFinished( eQueryType_CommitFlag, bFailed, SSendBuf( pInfo, sizeof(SCommitFlag) ) );
	}

	void CShareMemoryMgr::OnCommitAllBlocksOKFinish( const SCommitFlag* pInfo, bool bFailed )
	{
		OnFinished( eQueryType_CommitAllBlocksOK, bFailed, SSendBuf( pInfo, sizeof(SCommitFlag) ) );
		if( bFailed )
			return;
		GammaLog << "****DB CommitFlag OK," << pInfo->m_nDBGasID << "," << pInfo->m_nTrunkID << endl;
	}

	void CShareMemoryMgr::OnCommitBlocksFinish( const SCommitBlocksInfo* pInfo, bool bFailed )
	{
		OnFinished( eQueryType_CommitBlock, bFailed, SSendBuf( pInfo, sizeof(SCommitBlocksInfo) ) );
		if (bFailed)
			return;
	}

	void CShareMemoryMgr::OnFinished( ECmdType eType, bool bFailed, SSendBuf SendBuf )
	{
		SShmCmdHead Head;
		Head.m_nType = (uint8)eType;
		Head.m_bFailed = bFailed;
		m_FinishCmdBuffer.PushBuffer( Head, &SendBuf, SendBuf.second ? 1 : 0, true );
	}

	void CShareMemoryMgr::CheckResult()
	{
		SSendBuf SendBuf;
		SShmCmdHead CmdHead;

		while( true )
		{
			uint32 nCount = 1;
			if( !m_FinishCmdBuffer.PopBuffer( CmdHead, &SendBuf, nCount ) )
				return;

			switch( CmdHead.m_nType )
			{
			case eQueryType_ShmReboot:
				OnAnswerReboot( !CmdHead.m_bFailed );
				break;
			case eQueryType_CommitBlock:
				OnAnswerCommitBlock( !CmdHead.m_bFailed, (const SCommitBlocksInfo*)SendBuf.first );
				break;
			case eQueryType_CommitFlag:
				OnAnswerCommitFlg( !CmdHead.m_bFailed, (const SCommitFlag*)SendBuf.first );
				break;
			default:
				break;
			}
		}
	}

	void CShareMemoryMgr::SendCmd( ECmdType eQueryType, uint16 nGasID, int64 uuid,
		const void* pData1, size_t nSize1, const void* pData2, size_t nSize2 )
	{
		SShmCmdHead Head;
		Head.m_nType = (uint8)eQueryType;
		Head.m_bFailed = false;
		SSendBuf arySendBuff[2] = { SSendBuf( pData1, nSize1 ), SSendBuf( pData2, nSize2 ) };
		m_QueryCmdBuffer.PushBuffer( Head, arySendBuff, 2, true );

		char	szErrorLog[255];
		gammasstream logstr(szErrorLog);
		logstr << GetProcessTime() << " [DBQuery]  ShmSendCmd, " << (uint32)eQueryType << "," << m_nSendCount++ << "," << uuid  << endl;
		m_pThreadLog->Write(szErrorLog, logstr.length());

	}

	//---------------------------------------------------------
	//
	//---------------------------------------------------------
	void CShareMemoryMgr::ProcessRebootGas()
	{
		GammaLog << "[shmthread] ProcessRebootGas"<<endl;
		SendCmd( eQueryType_ShmReboot, m_nGasID, 1, "", 0, "", 0 );
	}

	void CShareMemoryMgr::OnAnswerReboot( bool bSucceeded )
	{
		GammaLog << "[shmthread] OnAnswerReboot ok" << endl;
		m_eCheckState = eCS_Check;
	}

	SChunckHead* CShareMemoryMgr::GetChunckHead( const SDBBlockInfo* pAnswer, bool bSucceeded, ECmdType eType )
	{
		SChunckTypeInfo& TrunkTypeInfo = m_pShareDesCommHead->m_aryTrunkTypeInfo[pAnswer->m_nTrunkType];
		tbyte* pInfoStart = (tbyte*)(&TrunkTypeInfo) + TrunkTypeInfo.m_nChunkOffset;
		SChunckHead* pTrunkHead = (SChunckHead*)( pInfoStart + pAnswer->m_nTrunkIndex * TrunkTypeInfo.m_nChunkSize );
		
		uint32 nCurTime = GetProcessTime();
		uint32 nDBCost = nCurTime - pAnswer->m_nQueryTime;
		m_vecTrunkTypeData[pAnswer->m_nTrunkType].m_nDBCost = nDBCost;

		CCommitMap& mapCommit = GetCommitMap(pAnswer->m_nTrunkType);
		mapCommit[pTrunkHead] = m_nPreCheckTime;

		char	szErrorLog[512];
		gammasstream logstr(szErrorLog);
		
		if ( pTrunkHead->m_nChunckID == INVALID_64BITID
			|| pTrunkHead->m_nChunckID != pAnswer->m_nTrunkID
			|| pTrunkHead->m_nDBGasID != pAnswer->m_nDBGasID )
		{
			logstr << GetProcessTime() << " [shmthread] OnAnswerDB check error, " << pAnswer->m_nTrunkID << endl;
		}
		else
		{
			logstr << GetProcessTime() << " [shmthread] OnAnswerDB, result=" << (uint32)bSucceeded
				<< ",type=" << (uint32)eType << ",id=" << pAnswer->m_nTrunkID 
				<< ",costtime:" << nDBCost
				<< "=" << pAnswer->m_nMainThreadQueryTime - pAnswer->m_nQueryTime
				<< "+" << pAnswer->m_nDBStartTime - pAnswer->m_nMainThreadQueryTime
				<< "+" << pAnswer->m_nDBResultTime - pAnswer->m_nDBStartTime << "(" << pAnswer->m_nDBCost << ")"
				<< "+" << pAnswer->m_nMainThreadFinishTime - pAnswer->m_nDBResultTime
				<< "+" << nCurTime - pAnswer->m_nMainThreadFinishTime << endl;
		}
		m_pThreadLog->Write(szErrorLog, logstr.length());

		return bSucceeded ? pTrunkHead : nullptr;
	}

	void CShareMemoryMgr::OnAnswerCommitBlock( bool bSucceeded, const SCommitBlocksInfo* pAnswer )
	{

		SChunckHead* pTrunkHead = GetChunckHead(pAnswer, bSucceeded, eQueryType_CommitBlock);
		if (!pTrunkHead)
			return;

		CCommitMap& mapCommit = GetCommitMap( pAnswer->m_nTrunkType );

		if (pTrunkHead->m_nDataState == eSCT_UpdateFlg)
			mapCommit[pTrunkHead] = 0;
		else
			mapCommit[pTrunkHead] = m_nPreCheckTime;

		uint32 nCommitID = pAnswer->m_nCommitID;
		uint32 nDataVersion = pAnswer->m_nCurDataVersion;

		SBlockInfo* pBlockInfo = (SBlockInfo*)( ((tbyte*)pTrunkHead) + pTrunkHead->m_nChunkHeadSize );

		for( uint32 i = 0; i < pTrunkHead->m_nBlockCount; i++, pBlockInfo++ )
		{
			if( !pBlockInfo->m_pContextData )
				continue;

			SCommitData* pCommitData = (SCommitData*)( pBlockInfo->m_pContextData );
			SBlockData* pBlockData = (SBlockData*)( ( (tbyte*)pBlockInfo ) + pBlockInfo->m_nDataOffset );
			pBlockData->m_nAnswerDataVersion = nDataVersion;

			if( pCommitData->m_nBlockCommitID != nCommitID || !pCommitData->m_bDataInited )
				continue;

			if( !pBlockInfo->m_nArrayElemSize )
				continue;

			// 如果提交成功则将标志全部清空
			size_t nArrayDataSize = pBlockInfo->m_nMaxBlockSize - pBlockInfo->m_nArrayStartOff;
			size_t nMaxElemCount = ( nArrayDataSize - sizeof(SSizeType) )/pBlockInfo->m_nArrayElemSize;
			size_t nPreCommitFlagBufSize = ( nMaxElemCount - 1 )/8 + 1;
			tbyte* pPreData = (tbyte*)( pCommitData + 1 );
			memset( pPreData + nArrayDataSize, 0, nPreCommitFlagBufSize );
		}
	}

	void CShareMemoryMgr::OnAnswerCommitFlg( bool bSucceeded, const SCommitFlag* pAnswer )
	{

		SChunckHead* pTrunkHead = GetChunckHead(pAnswer, bSucceeded, eQueryType_CommitFlag);
		if (!pTrunkHead)
			return;

		if ( pTrunkHead->m_nDataState >= eSCT_UpdateFlg )
		{
			SBlockInfo* pBlockInfo	= (SBlockInfo*)( ((tbyte*)pTrunkHead) + pTrunkHead->m_nChunkHeadSize );
			for( uint32 i = 0; i < pTrunkHead->m_nBlockCount; i++, pBlockInfo++ )
				( (SCommitData*)pBlockInfo->m_pContextData )->m_bDataInited = false;
			pTrunkHead->m_nChunckID = INVALID_64BITID;
			pTrunkHead->m_nDataState = eSCT_Invalid;

			char	szErrorLog[256];
			gammasstream logstr(szErrorLog);
			logstr << GetProcessTime() << " [shmthread] CommitAllBlocksOK," << pAnswer->m_nTrunkID << endl;
			m_pThreadLog->Write(szErrorLog, logstr.length());

			SendCmd( eQueryType_CommitAllBlocksOK, pAnswer->m_nDBGasID, 
				(uint16)pAnswer->m_nTrunkID, pAnswer, sizeof(SCommitFlag), "", 0 );
			return;
		}
	}
	
	void CShareMemoryMgr::FreeInfoOnEnd( uint32 nTrunkType )
	{
		SChunckTypeInfo& TrunkTypeInfo = m_pShareDesCommHead->m_aryTrunkTypeInfo[nTrunkType];
		tbyte* pInfoStart = (tbyte*)( &TrunkTypeInfo ) + TrunkTypeInfo.m_nChunkOffset;
		for ( uint32 m = 0; m < TrunkTypeInfo.m_nChunkCount; m++, pInfoStart += TrunkTypeInfo.m_nChunkSize )
		{
			SChunckHead* pTrunkHead = (SChunckHead*)pInfoStart;
			SBlockInfo* pBlockInfo	= (SBlockInfo*)( ((tbyte*)pTrunkHead) + pTrunkHead->m_nChunkHeadSize );
			for( uint32 i = 0; i < pTrunkHead->m_nBlockCount; i++, pBlockInfo++ )
			{
				delete [] ( (tbyte*)pBlockInfo->m_pContextData );
				// 必须使用m_ptPoint，否则32位下面高位不会为0
				pBlockInfo->m_ptPoint = 0;
			}
		}
	}

	void CShareMemoryMgr::PollInfo( uint32 nTrunkType )
	{
		m_nPreCheckTime = GetGammaTime();

		const uint32 COUNT_PERLOOP = 20;
		uint32 nDBCost = m_vecTrunkTypeData[nTrunkType].m_nDBCost;

		SChunckTypeInfo& TrunkTypeInfo = m_pShareDesCommHead->m_aryTrunkTypeInfo[nTrunkType];
		// 检查定时提交
		tbyte* pInfoStart = (tbyte*)( &TrunkTypeInfo ) + TrunkTypeInfo.m_nChunkOffset;
		for( uint32 n = 0; n < TrunkTypeInfo.m_nChunkCount; n++, pInfoStart += TrunkTypeInfo.m_nChunkSize )
		{
			SChunckHead* pTrunkHead = (SChunckHead*)pInfoStart;
			// 如果是无效数据不用理会
			if( pTrunkHead->m_nChunckID == INVALID_64BITID )
				continue;

			if( pTrunkHead->m_nDataState == eSCT_Invalid )
				continue;

			CCommitMap& mapCommit = GetCommitMap( nTrunkType );
			uint32& nLastCommitTime = mapCommit[pTrunkHead];

			// 提交时间未到，不用检查
			uint32 nCheckInterval = pTrunkHead->m_nDataState == eSCT_UpdateBlock ? m_nCommitInterval + nDBCost : 1000;	//非UpdateBlock 
			if( int64( nLastCommitTime  + nCheckInterval ) > m_nPreCheckTime )
				continue;
		
			// 检查Gas是否正在写数据，如果是则返回，等下次再做Diff
			uint32 nGasCommitID = pTrunkHead->m_nCommitIDFirst;
			if( nGasCommitID != pTrunkHead->m_nCommitIDLast )
				continue;			

			size_t nBufferSize = 0;
			uint32 nCommitBlockInfoCount = 0;
			if( m_vecCommitBlockInfo.size() < pTrunkHead->m_nBlockCount )
				m_vecCommitBlockInfo.resize( pTrunkHead->m_nBlockCount );

			SBlockInfo* pBlockInfo = (SBlockInfo*)( ((tbyte*)pTrunkHead) + pTrunkHead->m_nChunkHeadSize );
			for( uint32 i = 0; i < pTrunkHead->m_nBlockCount; i++, pBlockInfo++ )
			{
				SBlockData* pBlockData = (SBlockData*)( ( (tbyte*)pBlockInfo ) + pBlockInfo->m_nDataOffset );
				uint32 nCurVersion = pBlockData->m_nQueryDataVersion;
				// 数据没有变化,不需要检查
				if( nCurVersion <= pBlockData->m_nAnswerDataVersion )
					continue;

				uint32 nDataSize = BuildDiff( pBlockInfo );
				if( nDataSize == INVALID_32BITID )
				{
					nBufferSize = INVALID_32BITID;
					break;
				}

				m_vecCommitBlockInfo[nCommitBlockInfoCount++] = pBlockInfo;
				uint32 nSize = 
					sizeof(uint16) + /*nType*/
					sizeof(uint16) + /*m_nArrayIndexCount*/
					m_nArrayIndexCount*sizeof(uint16) + /*nIndexArray*/
					sizeof(uint32) + /*nDataLen*/
					( nDataSize - sizeof(SBlockData) ); /*Data*/

				if( m_szCommitBuf.size() < nBufferSize + nSize )
					m_szCommitBuf.resize( nBufferSize + nSize );

				SBlockData* pNewBlockData = (SBlockData*)&m_szDiffData[0];
				CBufFileEx File( (tbyte*)&m_szCommitBuf[nBufferSize], nSize );
				File << (uint16)pBlockInfo->m_nType;
				File << m_nArrayIndexCount;
				File.Write( &m_vecDiffArrayIndex[0], m_nArrayIndexCount*sizeof(uint16) );
				File << (uint32)( nDataSize - sizeof(SBlockData) );
				File.Write( pNewBlockData + 1, ( nDataSize - sizeof(SBlockData) ) );
				nBufferSize += nSize;
			}

			// 检查Gas是否正在写数据，如果是则返回，等下次再做Diff
			if( nBufferSize == INVALID_32BITID )
				continue;
			// 检查Gas是否正在写数据，如果是则返回，等下次再做Diff
			if( nGasCommitID != pTrunkHead->m_nCommitIDFirst ||
				nGasCommitID != pTrunkHead->m_nCommitIDLast )
				continue;

			// 全部数据提交后要设置状态为eSCT_UpdateFlg
			if (pTrunkHead->m_nDataState == eSCT_UpdateTrunk)
			{
				char	szErrorLog[256];
				gammasstream logstr(szErrorLog);
				logstr << GetProcessTime() << " [shmthread] UpdateTrunk -> UpdateFlg, " << pTrunkHead->m_nChunckID << endl;
				m_pThreadLog->Write(szErrorLog, logstr.length());
				pTrunkHead->m_nDataState = eSCT_UpdateFlg;
			}

			// 发出去了一批，时间需要重置
			nLastCommitTime = m_nPreCheckTime;
			if( nCommitBlockInfoCount == 0 )
			{
				if( pTrunkHead->m_nDataState == eSCT_UpdateFlg 
					|| pTrunkHead->m_nDataState == eSCT_WaitingFlg )
				{
					char	szErrorLog[256];
					gammasstream logstr(szErrorLog);
					logstr << GetProcessTime() << " [shmthread] CommitBlock = 0, " << pTrunkHead->m_nChunckID << endl;
					m_pThreadLog->Write(szErrorLog, logstr.length());

					// 不需要处理提交标志的直接重置内存，否则需要到数据库提交标志
					if( TrunkTypeInfo.m_bCommitFlagRequire )
					{
						SCommitFlag Info;
						Info.m_nTrunkType = nTrunkType;
						Info.m_nTrunkID = pTrunkHead->m_nChunckID;
						Info.m_nDBGasID = pTrunkHead->m_nDBGasID;
						Info.m_nTrunkIndex = n;
						Info.m_nQueryTime = GetProcessTime();
						SendCmd( eQueryType_CommitFlag, Info.m_nDBGasID, 
							(uint16)Info.m_nTrunkID, &Info, sizeof(Info), "", 0 );
						pTrunkHead->m_nDataState = eSCT_WaitingFlg;
					}
					else
					{
						pTrunkHead->m_nChunckID = INVALID_64BITID;
						pTrunkHead->m_nDataState = eSCT_Invalid;
					}
				}
				continue;
			}

			for( size_t i = 0; i < nCommitBlockInfoCount; i++ )
			{
				SBlockInfo* pBlockInfo = m_vecCommitBlockInfo[i];
				SCommitData& CommitData	= *(SCommitData*)( pBlockInfo->m_pContextData );
				CommitData.m_nBlockCommitID = m_nBlockCommitID;
			}

			SCommitBlocksInfo Info;
			Info.m_nTrunkType = nTrunkType;
			Info.m_nTrunkID = pTrunkHead->m_nChunckID;
			Info.m_nDBGasID = pTrunkHead->m_nDBGasID;
			Info.m_nTrunkIndex = n;
			Info.m_nCommitID = m_nBlockCommitID++;
			Info.m_nCurDataVersion = pTrunkHead->m_nDataVersion;
			Info.m_nBlockCount = nCommitBlockInfoCount;
			Info.m_nQueryTime = GetProcessTime();
			SendCmd( eQueryType_CommitBlock, Info.m_nDBGasID, pTrunkHead->m_nChunckID,
				&Info, sizeof(Info), &m_szCommitBuf[0], nBufferSize );
		}

		m_vecNextQueryIndex[nTrunkType] += COUNT_PERLOOP; //一次循环最多处理20条
		if (m_vecNextQueryIndex[nTrunkType] >= TrunkTypeInfo.m_nChunkCount)
			m_vecNextQueryIndex[nTrunkType] = 0;
	}
	

	CCommitMap& CShareMemoryMgr::GetCommitMap( uint32 nTrunkType )
	{
		return m_vecTrunkTypeData[nTrunkType].m_mapCommitData;
	}
	
	bool CShareMemoryMgr::HasOtherGasData()
	{
		for( uint32 nTrunkType = 0; nTrunkType < m_vecTrunkTypeData.size(); nTrunkType++ )
		{
			SChunckTypeInfo& TrunkTypeInfo = m_pShareDesCommHead->m_aryTrunkTypeInfo[nTrunkType];
			tbyte* pInfoStart = (tbyte*)( &TrunkTypeInfo ) + TrunkTypeInfo.m_nChunkOffset;

			for ( uint32 n = 0; n < TrunkTypeInfo.m_nChunkCount; n++, pInfoStart += TrunkTypeInfo.m_nChunkSize )
			{
				SChunckHead* pTrunkHead = (SChunckHead*)pInfoStart;
				if ( pTrunkHead->m_nChunckID == INVALID_64BITID )
					continue;
				if( pTrunkHead->m_nDBGasID != m_nGasID )
					return true;
			}	

		}
		return false;
	}

	size_t CShareMemoryMgr::GetCountInCommit( uint32 nTrunkType, uint16 nGasID )
	{
		size_t nCount = 0;

		SChunckTypeInfo& TrunkTypeInfo = m_pShareDesCommHead->m_aryTrunkTypeInfo[nTrunkType];
		tbyte* pInfoStart = (tbyte*)( &TrunkTypeInfo ) + TrunkTypeInfo.m_nChunkOffset;

		for ( uint32 n = 0; n < TrunkTypeInfo.m_nChunkCount; n++, pInfoStart += TrunkTypeInfo.m_nChunkSize )
		{
			SChunckHead* pTrunkHead = (SChunckHead*)pInfoStart;
			if ( pTrunkHead->m_nChunckID == INVALID_64BITID )
				continue;
			if( pTrunkHead->m_nDBGasID == nGasID )
				nCount++;
		}	

		return nCount;
	}

	uint32 CShareMemoryMgr::BuildDiff( SBlockInfo* pBlockInfo )
	{
		SCommitData& CommitData	= *(SCommitData*)( pBlockInfo->m_pContextData );
		SBlockData* pBlockData	= (SBlockData*)( ( (tbyte*)pBlockInfo ) + pBlockInfo->m_nDataOffset );
		uint32 nDataSize		= pBlockInfo->m_nMaxBlockSize;
		if( m_szDiffData.size() < nDataSize )
			m_szDiffData.resize( nDataSize );

		// 检查Gas是否正在写数据，如果是则返回，等下次再做Diff
		uint32 nGasCommitID = pBlockData->m_nCommitIDFirst;
		if( nGasCommitID != pBlockData->m_nCommitIDLast )
			return INVALID_32BITID;

		memcpy( &m_szDiffData[0], pBlockData, nDataSize );

		// 检查Gas是否正在写数据，如果是则返回，等下次再做Diff
		if( nGasCommitID != pBlockData->m_nCommitIDFirst ||
			nGasCommitID != pBlockData->m_nCommitIDLast )
			return INVALID_32BITID;

		m_nArrayIndexCount = 0;
		if( pBlockInfo->m_nArrayElemSize )
		{
			size_t nArrayElemSize			= pBlockInfo->m_nArrayElemSize;
			size_t nArrayOffset				= pBlockInfo->m_nArrayStartOff;
			size_t nArrayDataSize			= nDataSize - nArrayOffset;
			size_t nMaxElemCount			= ( nArrayDataSize - sizeof(SSizeType) )/nArrayElemSize;
			size_t nPreCommitFlagBufSize	= ( nMaxElemCount - 1 )/8 + 1;
			tbyte* pPreData					= (tbyte*)( &CommitData + 1 );

			// 没有之前的对比数据时，需要全部都update
			// 注意：为了节省空间，对比数组只保存数组大小以及元素数据，不包括SBlockData头
			if( !CommitData.m_bDataInited )
			{
				SSizeType* pOldArySize		= (SSizeType*)pPreData;
				tbyte* pCommitFlagBuf		= pPreData + nArrayDataSize;
				memcpy( pPreData, &m_szDiffData[nArrayOffset], nArrayDataSize );
				memset( pCommitFlagBuf, 0, nPreCommitFlagBufSize );

				TBitSet<65536>* pBitSet = (TBitSet<65536>*)pCommitFlagBuf;
				GammaAst( nMaxElemCount < INVALID_16BITID );
				for( uint16 i = 0; i < nMaxElemCount; i++ )
				{
					m_vecDiffArrayIndex[m_nArrayIndexCount++] = (uint16)i;
					pBitSet->SetBit( i, 1 );
				}
				pOldArySize->m_nSize		= (uint32)nMaxElemCount;
				m_nArrayIndexCount			= (uint16)pOldArySize->m_nSize;
				CommitData.m_bDataInited	= true;
			}
			else
			{
				// 因为所有程序都是流氓帮成员，所以数组中记录的size不能信任，
				// 只能通过一个字节一个字节的检查实际的数组大小：（
				// 但是这里只能检查那些明确注册为数组的块，
				// 不能检查那些实际上是数组，但是没有注册成数组的数据块，
				// 这些没有注册成数组的数组数据块只能在gcc检查
				SSizeType* pNewArySize		= (SSizeType*)&m_szDiffData[nArrayOffset];
				SSizeType* pOldArySize		= (SSizeType*)pPreData;
				size_t nValidArrayDataSize	= nArrayDataSize - sizeof(SSizeType);

				char* pData = (char*)( pNewArySize + 1 );
				while( nValidArrayDataSize && !pData[ nValidArrayDataSize - 1 ] )
					nValidArrayDataSize--;

				pNewArySize->m_nSize = 0;
				if( nValidArrayDataSize )
					pNewArySize->m_nSize	= (uint32)( ( nValidArrayDataSize - 1 )/nArrayElemSize + 1 );

				tbyte* pCommitFlagBuf		= pPreData + nArrayDataSize;
				TBitSet<65536>* pBitSet		= (TBitSet<65536>*)pCommitFlagBuf;
				GammaAst( pNewArySize->m_nSize < INVALID_16BITID );

				tbyte* pNewArray			= (tbyte*)( pNewArySize + 1 );
				tbyte* pOldArray			= (tbyte*)( pOldArySize + 1 );
				uint32 nDiffCount			= 0;
				uint32 nElemSize			= pBlockInfo->m_nArrayElemSize;
				uint32 nCheckCount			= 0;
				uint32 nCheckOffset			= 0;
				uint32 nDiffOffset			= 0;

				// 新的数据与旧的数据对比，将新的数据不同于的部分放到提交缓冲区，
				// 比且将提交记录写到提交标志表中
				while( nCheckCount < pNewArySize->m_nSize )
				{
					if( pBitSet->GetBit( nCheckCount ) || memcmp( pOldArray + nCheckOffset, pNewArray + nCheckOffset, nElemSize ) )
					{
						memcpy( pOldArray + nCheckOffset, pNewArray + nCheckOffset, nElemSize );
						memcpy( pNewArray + nDiffOffset,  pNewArray + nCheckOffset, nElemSize );
						nDiffOffset += nElemSize;
						m_vecDiffArrayIndex[nDiffCount++] = (uint16)nCheckCount;
						pBitSet->SetBit( nCheckCount, 1 );
					}
					else
					{
						memcpy( pOldArray + nCheckOffset, pNewArray + nCheckOffset, nElemSize );
					}

					nCheckCount	 ++;
					nCheckOffset += nElemSize;
				}

				// 如果旧的数据比新的数据多，
				// 那么要提交空白数据用于删除。
				// 如果上一次提交到数据库失败，
				// 那么剩余的提交需要继续进行
				while( nCheckCount < nMaxElemCount )
				{
					if( pBitSet->GetBit( nCheckCount ) || 
						( nCheckCount < pOldArySize->m_nSize && !IsAllBeTheElem<tbyte>( pOldArray + nCheckOffset, nElemSize, 0 ) ) )
					{
						pBitSet->SetBit( nCheckCount, 1 );
						memset( pNewArray + nDiffOffset, 0, nElemSize );
						memset( pOldArray + nCheckOffset, 0, nElemSize );
						nDiffOffset += nElemSize;
						m_vecDiffArrayIndex[nDiffCount++] = (uint16)nCheckCount;
					}
					nCheckCount	 ++;
					nCheckOffset += nElemSize;
				}

				nDataSize				= pBlockInfo->m_nArrayStartOff + sizeof(SSizeType) + nDiffOffset;
				pOldArySize->m_nSize	= pNewArySize->m_nSize;
				pNewArySize->m_nSize	= nDiffCount;
				m_nArrayIndexCount		= (uint16)nDiffCount;
			}
		}

		char* pData	= &m_szDiffData[0];
		while( nDataSize > sizeof( SBlockData ) && !pData[ nDataSize - 1 ] )
			nDataSize--;
		return nDataSize;
	}

}