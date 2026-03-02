
#include "GammaCommon/ILog.h"
#include "GammaCommon/TGammaStrStream.h"
#include "GammaCommon/CDynamicObject.h"
#include "GammaCommon/GammaCodeCvs.h"
#include "GammaMTDbs/CBaseDbsConn.h"
#include "CDbsThread.h"
#include "CDbsThreadMgr.h"
using namespace std;

namespace Gamma
{
	CDbsThread::CDbsThread( uint32 nConnClassID,
		const SDbsCreateParam* aryParam, uint8 nDbsCount, uint32 nThreadIndex, bool bIsRecord)
		: m_bQuit(false)
		, m_bCreated( false )
		, m_vecDbsConnect( nDbsCount )
		, m_nHungryID( 0 )
		, m_nFeedID( 0 )
		, m_hReadSemaphore( NULL )  
		, m_nResultIndex( 0 )
		, m_aryResultBuffer( 1 )
		, m_nThreadIndex( nThreadIndex )
		, m_nPushNum(0)
		, m_nPopNum(0)
		, m_bIsRecord(bIsRecord)
	{
		static int32 nThreadLogIndex = 0;
		CDynamicObject* pObject = CDynamicObject::CreateInstance( nConnClassID );
		m_pHandler = static_cast<CBaseDbsConn*>( pObject );

		m_listParam.resize( nDbsCount );
		for( uint8 i = 0 ; i < nDbsCount; i++ )
		{
			m_listParam[i].m_szDbsHost = aryParam[i].szDbsHost;
			m_listParam[i].m_nDbsPort = aryParam[i].nDbsPort;
			m_listParam[i].m_szDataBase = aryParam[i].szDataBase;
			m_listParam[i].m_szUser = aryParam[i].szUser;
			m_listParam[i].m_szPassword = aryParam[i].szPassword;
			m_listParam[i].m_nDBId = aryParam[i].nDBId;
			m_listParam[i].m_bUpdateAffectRowAsFound = aryParam[i].bFoundAsUpdateRow;
		}

		char szLogFileName[256];
		gammasstream( szLogFileName ) << "CDbsThread_" << nThreadLogIndex++; 
		m_pThreadLog = GetLogFile( szLogFileName, GammaGetCurrentProcessID(), eLPT_All );
		if( !m_pThreadLog )
			GammaThrow( "Can not create thread log!!!!" );

		m_hReadSemaphore = GammaCreateSemaphore();
		GammaCreateThread( &m_hThread, 1024, (THREADPROC)&CDbsThread::RunThread, this );
		while( !m_bCreated )
			GammaSleep( 1 );
	}

	CDbsThread::~CDbsThread(void)
	{
		m_bQuit = true;
		GammaPutSemaphore( m_hReadSemaphore );
		GammaJoinThread( m_hThread );

		m_pHandler->OnDisConnect( &m_vecDbsConnect[0], (uint8)m_vecDbsConnect.size() );
		CDynamicObject::DestroyInstance( m_pHandler );

		for( uint32 i = 0; i < m_vecDbsConnect.size(); i++ )
			SAFE_RELEASE( m_vecDbsConnect[i] );
		SAFE_RELEASE( m_pThreadLog );
		GammaDestroySemaphore( m_hReadSemaphore );
	}

	bool CDbsThread::ConnectDbs()
	{
		for( uint32 i = 0; i < m_vecDbsConnect.size(); i++ )
			SAFE_RELEASE( m_vecDbsConnect[i] );

		try
		{
			for( uint32 i = 0; i < m_vecDbsConnect.size(); i++ )
			{
				m_vecDbsConnect[i] = GetDatabase()->CreateConnection(
					m_listParam[i].m_szDbsHost.c_str(),
					m_listParam[i].m_nDbsPort,
					m_listParam[i].m_szUser.c_str(),
					m_listParam[i].m_szPassword.c_str(),
					m_listParam[i].m_szDataBase.c_str(),
					m_listParam[i].m_nDBId,
					m_listParam[i].m_bUpdateAffectRowAsFound,
					false, false );

			}
			m_pHandler->OnConnected( m_nThreadIndex, &m_vecDbsConnect[0], (uint8)m_vecDbsConnect.size(), m_bIsRecord );
		}
		catch( string szError )
		{
			for( uint32 i = 0; i < m_vecDbsConnect.size(); i++ )
				SAFE_RELEASE( m_vecDbsConnect[i] );
			GammaLog << "CDbsThread ConnectDbs " << szError.c_str() << endl;
			return false;
		}
		catch( const char* szError )
		{
			for( uint32 i = 0; i < m_vecDbsConnect.size(); i++ )
				SAFE_RELEASE( m_vecDbsConnect[i] );
			GammaLog << "CDbsThread ConnectDbs " << szError << endl;
			return false;
		}
		catch( ... )
		{
			for( uint32 i = 0; i < m_vecDbsConnect.size(); i++ )
				SAFE_RELEASE( m_vecDbsConnect[i] );
			GammaLog << "CDbsThread ConnectDbs " << "Unknow Error" << endl;
			return false;
		}

		return true;
	}

	uint32 CDbsThread::RunThread( void* pParam )
	{
		GammaSetThreadName( "CDbsThread" );
		return reinterpret_cast<CDbsThread*>( pParam )->Run();
	}

	uint32 CDbsThread::Run()
	{
		if( !ConnectDbs() )
			GammaThrow( "Can not connect dbs server!!!!" );
		m_bCreated = true;

		while( !m_bQuit )
		{			
			if( !m_CmdBuffer.CanPop() )
			{
				m_nHungryID++;
				GammaGetSemaphore( m_hReadSemaphore, 30000 );
			}
			Execute();
		}
		return 0;
	}

	void CDbsThread::Execute()
	{
		// 执行数据库查询之前确认数据库连接是否仍然存在
		while( true )
		{
			bool bAllValid = true;
			for( uint32 i = 0; i < m_vecDbsConnect.size() && bAllValid; i++ )
				if( !m_vecDbsConnect[i] || !m_vecDbsConnect[i]->Ping() )
					bAllValid = false;
			if( bAllValid )
				break;

			const string strLog = "ReConnect to DB";
			m_pThreadLog->Write(strLog.c_str(), strLog.size());

			if( m_vecDbsConnect[0] )
				m_pHandler->OnDisConnect( &m_vecDbsConnect[0], (uint8)m_vecDbsConnect.size() );
			for( uint32 i = 0; i < m_vecDbsConnect.size(); i++ )
				SAFE_RELEASE( m_vecDbsConnect[i] );


			// ConnectDbs有内存泄漏，
			// 数据库连接过于频繁会导致内存消耗非常大
			GammaSleep( 2000 );
			ConnectDbs();
		}

		SSendBuf arySendBuf[MAX_SEGMENT];

		// 执行数据库查询
		for( ;; )
		{
			uint32 nCount = ELEM_COUNT( arySendBuf );
			SQueryContext context;
			if (!m_CmdBuffer.PopBuffer(context, arySendBuf, nCount))
				return;
			GammaAst( nCount == 1 );
			context.nPopTime = GetProcessTime();
			try
			{
				if (context.bIsRecord)
				{
					// 使用DB_LOG指定的数据库 写业务日志
					m_pHandler->OnShellMsg(&m_vecDbsConnect[0],
						(uint8)m_vecDbsConnect.size(), nullptr,
						arySendBuf[0].first, arySendBuf[0].second);
				}
				else
				{
					// 使用DB指定的数据库 读写游戏数据
					m_nResultIndex = 0;
					m_aryResultBuffer[0].clear();
					m_pHandler->OnShellMsg(&m_vecDbsConnect[0],
						(uint8)m_vecDbsConnect.size(), this,
						arySendBuf[0].first, arySendBuf[0].second);
					if (!m_aryResultBuffer[m_nResultIndex].empty())
						m_nResultIndex++;
					for (uint32 i = 0; i < m_nResultIndex; i++)
					{
						arySendBuf[i].first = m_aryResultBuffer[i].c_str();
						arySendBuf[i].second = (uint32)m_aryResultBuffer[i].size();
					}
					context.nOutDBTime = GetProcessTime();
					m_ResultBuffer.PushBuffer(context, arySendBuf, m_nResultIndex, false);
				}
			}
			catch(...)
			{
				vector<char> vecBuf( arySendBuf[0].second*2, 0 );
				for( size_t i = 0; i < arySendBuf[0].second; i++ )
				{
					uint8 nValue = ((const uint8*)arySendBuf[0].first)[i];
					vecBuf[i*2] = (char)ValueToHexNumber( nValue >> 4 );
					vecBuf[i*2 + 1] = (char)ValueToHexNumber( nValue&0xf );
				}
				m_pThreadLog->Write( &vecBuf[0], vecBuf.size() );
			}
		}
	}

	void CDbsThread::Write( const void* pData, size_t nSize )
	{
		if( !nSize )
			return;
		if( !pData )
			m_aryResultBuffer[m_nResultIndex].append( nSize, 0 );
		else
			m_aryResultBuffer[m_nResultIndex].append( (const char*)pData, nSize );
	}

	void CDbsThread::Segment()
	{
		if( m_aryResultBuffer[m_nResultIndex].empty() )
			return;
		GammaAst( m_nResultIndex < INVALID_8BITID );
		if( ++m_nResultIndex >= m_aryResultBuffer.size() )
			m_aryResultBuffer.resize( m_nResultIndex + 1 );
		m_aryResultBuffer[m_nResultIndex].clear();
	}

	void CDbsThread::Querry( uint32 nQueryID, const SSendBuf aryBuffer[], uint32 nBufferCount )
	{
		SQueryContext context;
		context.bIsRecord = false;
		context.nQueryID = nQueryID;
		context.nThreadID = m_nThreadIndex;
		context.nPushTime = GetProcessTime();
		context.nPopTime = 0;

		m_CmdBuffer.PushBuffer(context, aryBuffer, nBufferCount, true );
		if( m_nHungryID == m_nFeedID )
			return;
		m_nFeedID = m_nHungryID;
		GammaPutSemaphore( m_hReadSemaphore );
	}


	void CDbsThread::Record(const SSendBuf aryBuffer[], uint32 nBufferCount)
	{
		SQueryContext context;
		context.bIsRecord = true;
		context.nThreadID = m_nThreadIndex;
		context.nPushTime = GetProcessTime();
		context.nPopTime = 0;

		m_CmdBuffer.PushBuffer(context, aryBuffer, nBufferCount, true);
		if (m_nHungryID == m_nFeedID)
			return;
		m_nFeedID = m_nHungryID;
		GammaPutSemaphore(m_hReadSemaphore);
	}

	bool CDbsThread::GetResult(SQueryContext& context, SSendBuf aryBuffer[], uint32& nBufferCount)
	{
		return m_ResultBuffer.PopBuffer(context, aryBuffer, nBufferCount );
	}

	uint32 CDbsThread::GetWaitingCmdCount() const
	{
		return m_CmdBuffer.GetWaitingBufferCount();
	}

	uint32 CDbsThread::GetWaitingResultCount() const
	{
		return m_ResultBuffer.GetWaitingBufferCount();
	}

	void CDbsThread::CheckHungry()
	{
		if( m_nHungryID == m_nFeedID )
			return;
		if( !m_CmdBuffer.CanPop() )
			return;
		m_nFeedID = m_nHungryID;
		GammaPutSemaphore( m_hReadSemaphore );
	}
}
