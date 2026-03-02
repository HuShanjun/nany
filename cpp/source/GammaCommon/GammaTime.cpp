

#ifdef _WIN32
#pragma warning(disable: 4201)
#include <windows.h>
#include <mmsystem.h>
#pragma comment( linker, "/defaultlib:winmm.lib" )
#elif ( defined _ANDROID )
#include <stdlib.h>
#elif ( defined _IOS || defined _MAC )
#include <mach/mach_time.h>
#endif

#include "GammaCommon/GammaTime.h"
#include "GammaCommon/CThread.h"
#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/CVirtualFun.h"
#include "GammaCommon/TGammaStrStream.h"
#include "GammaCommon/GammaProfile.h"
#include <stdio.h>
#include <stdlib.h>

namespace Gamma
{
	//========================================================================
	// struct STime 
	//========================================================================
	void STime::Format2Str( char* szOut, uint32 nSize )
	{
		char szBuffer[32];
		gammasstream( szBuffer ) 
			<< m_nYear 
			<< "-"
			<< std::setw(2) << std::setfill('0') << m_nMonth 
			<< "-"
			<< std::setw(2) << std::setfill('0') << m_nDayOfMonth
			<< " "
			<< std::setw(2) << std::setfill('0') << m_nHour 
			<< ":"
			<< std::setw(2) << std::setfill('0') << m_nMinutes 
			<< ":"
			<< std::setw(2) << std::setfill('0') << m_nSecond;
		strcpy_safe( szOut, szBuffer, nSize, INVALID_32BITID );
	}

	bool STime::IsSameDay( const STime& otherTime ) const
	{
		return m_nYear == otherTime.m_nYear && m_nDayOfYear == otherTime.m_nDayOfYear;
	}

	//========================================================================
	// Time
	//========================================================================

    int64 GetTimeFromMechineStart()
    {
#ifdef _WIN32
		//struct CFreq
		//{
		//	LARGE_INTEGER m_Freq;
		//	CFreq()    
		//	{ 
		//		QueryPerformanceFrequency( &m_Freq );
		//	}
		//};

		//static CFreq Freq;
		//LARGE_INTEGER uCounter;
		//QueryPerformanceCounter( &uCounter );

		//// return uCounter.QuadPart*1000/Freq.m_Freq.QuadPart;
		//// 下面的代码功能上等于被注释掉的代码
		//// 不使用上面的代码的原因是因为uCounter.QuadPart*1000可能会超出int64范围
		//
		//int64 nSecond = uCounter.QuadPart/Freq.m_Freq.QuadPart;
		//int64 nMSecnd = uCounter.QuadPart%Freq.m_Freq.QuadPart;
		//return nSecond*1000 + ( nMSecnd*1000 )/Freq.m_Freq.QuadPart;

		//// QueryPerformanceCounter，QueryPerformanceFrequency函数有平台相关性，
		//// 暂不使用，用timeGetTime代替
		// return timeGetTime();

		// 如果支持的话，使用GetSystemTimePreciseAsFileTime获得更高精度
		typedef decltype(GetSystemTimePreciseAsFileTime)* FunGetTime;
		struct SGetTimeFunction
		{
			FunGetTime m_funGetTime;
			SGetTimeFunction()
			{
				HMODULE hDll = ::LoadLibraryA( "Kernel32.dll" );
				const char* szFunName = "GetSystemTimePreciseAsFileTime";
				m_funGetTime = (FunGetTime)GetProcAddress( hDll, szFunName );
			}
		};
		static SGetTimeFunction s_GetTimeFun;
		if( !s_GetTimeFun.m_funGetTime )
			return timeGetTime();

		FILETIME CurTime;
		s_GetTimeFun.m_funGetTime( &CurTime );
		uint64 nCurTime = CurTime.dwLowDateTime;
		nCurTime |= ((uint64)CurTime.dwHighDateTime) << 32;
		return nCurTime / 10000;
		
#elif ( defined _IOS || defined _MAC )
        static struct mach_timebase_info timebase = { 0, 0 };
        if( timebase.numer == 0 )
            mach_timebase_info( &timebase );
        return (int64)( ( mach_absolute_time()*timebase.numer/timebase.denom )/NSEC_PER_MSEC );
#else
        timespec ts;
        if( -1 == clock_gettime( CLOCK_MONOTONIC, &ts ) )
            GammaThrow( "Call clock_gettime failed!!");
        return ((uint64)ts.tv_sec)*1000 + ts.tv_nsec/1000000;
#endif
    }
    
    int64 InitZoneTime()
    {
        time_t t1 = 100000;
        time( &t1 );
#ifdef _WIN32
        int64 t2 =(int64) mktime( gmtime( &t1 ) );
#else
		tm time;
		int64 t2 =(int64) mktime( gmtime_r( &t1, &time ) );
#endif
        return ( t1 - t2 ) * 1000;
	}

	int64	g_nProcessStart = GetTimeFromMechineStart();
	int64	g_nNatureTime = ( (int64)time( NULL ) )*1000 - GetProcessTime();
	int64	g_nZoneTime = InitZoneTime();

	CLock	g_TimeLock;
	int64	g_nLogicTime = 0;
	int64	g_nDeltaFromProcessTime = 0;
	bool	g_bPause = false;
	double	g_fTimeScale = 1.0f;

	int64 GetProcessTime()
	{
		return GetTimeFromMechineStart() - g_nProcessStart;
	}
	
	void AddProcessTime( int64 nDeltaTime )
	{
		g_nProcessStart -= nDeltaTime;
	}

	int64 GetZoneTime()
	{
		return g_nZoneTime;
	}

	int64 CalculateGammaTime()
	{
		if( g_fTimeScale == 1.0 )
			return GetProcessTime();
		return (int64)( GetProcessTime()*g_fTimeScale );
	}

    int64 GetGammaTime()
    {
        if( g_bPause )
            return g_nLogicTime;
        return CalculateGammaTime() - g_nDeltaFromProcessTime;
    }

    void PauseGammaTime( bool bPause )
	{
		if( bPause == g_bPause )
			return;

        g_TimeLock.Lock();
        if( g_bPause )
            g_nDeltaFromProcessTime = CalculateGammaTime() - g_nLogicTime;
        else
            g_nLogicTime = CalculateGammaTime() - g_nDeltaFromProcessTime;
        g_bPause = bPause;
        g_TimeLock.Unlock();
	}
	
	bool IsPauseGammaTime()
	{
		return g_bPause;
	}

    void SetGammaTime( int64 nLogicTime )
    {
        g_TimeLock.Lock();
        if( g_bPause )
            g_nLogicTime = nLogicTime;
        else
            g_nDeltaFromProcessTime = CalculateGammaTime() - nLogicTime;
        g_TimeLock.Unlock();
	}
	
	void SetGammaTimeScale( float fScale )
	{
		int64 nCurTime = GetGammaTime();
		int64 nNatureTime = GetNatureTime();
		g_fTimeScale = fScale;
		SetNatureTime( nNatureTime, g_nZoneTime );
		SetGammaTime( nCurTime );
	}

	float GetGammaTimeScale()
	{
		return (float)g_fTimeScale;
	}

	void SetNatureTime( int64 nNatureTime, int64 nZoneTime)
	{
		g_TimeLock.Lock();
		g_nNatureTime = nNatureTime - CalculateGammaTime();
		g_nZoneTime = nZoneTime;
		g_TimeLock.Unlock();
	}

	int64 GetProcessStartNatureTime()
	{
		return g_nNatureTime;
	}

	int64 NatureTime2LocalTime(int64 nNatureTime)
	{
		return g_nZoneTime + nNatureTime;
	}

	int64 LocalTime2NatureTime( int64 nLocalTime )
	{
		return nLocalTime - g_nZoneTime;
	}

	int64 GetLocalTime()
	{
		return g_nZoneTime + GetNatureTime();
	}

	int64 GetNatureTime()
	{
		return g_nNatureTime + CalculateGammaTime();
	}

	STime GetFormatTimeSTM()
	{
		time_t nTime = (time_t)( NatureTime2LocalTime( GetNatureTime() )/1000 );
#ifdef _WIN32
		tm gmtm = *gmtime( &nTime );
#else
		tm gmtm;
		gmtime_r( &nTime, &gmtm );
#endif
		STime stm;
		stm.m_nSecond = gmtm.tm_sec;
		stm.m_nMinutes = gmtm.tm_min;
		stm.m_nHour = gmtm.tm_hour;
		stm.m_nDayOfMonth = gmtm.tm_mday;
		stm.m_nMonth = gmtm.tm_mon + 1;
		stm.m_nYear = gmtm.tm_year + 1900;
		stm.m_nDayofWeek = gmtm.tm_wday;    // 返回0
		stm.m_nDayOfYear =gmtm.tm_yday + 1;
		stm.m_nIsdst = gmtm.tm_isdst;
		return stm;
	}

	void GetFormatTimeSTM( STime& stm )
	{
		time_t nTime = (time_t)( NatureTime2LocalTime( GetNatureTime() )/1000 );
#ifdef _WIN32
		tm gmtm = *gmtime( &nTime );
#else
		tm gmtm;
		gmtime_r( &nTime, &gmtm );
#endif
		stm.m_nSecond = gmtm.tm_sec;
		stm.m_nMinutes = gmtm.tm_min;
		stm.m_nHour = gmtm.tm_hour;
		stm.m_nDayOfMonth = gmtm.tm_mday;
		stm.m_nMonth = gmtm.tm_mon + 1;
		stm.m_nYear = gmtm.tm_year + 1900;
		stm.m_nDayofWeek = gmtm.tm_wday;    // 返回0
		stm.m_nDayOfYear =gmtm.tm_yday + 1;
		stm.m_nIsdst = gmtm.tm_isdst;
	}

	tm GetFormatTimeTM()
	{
		time_t nTime = (time_t)( NatureTime2LocalTime( GetNatureTime() )/1000 );
#ifdef _WIN32
		return *gmtime( &nTime );
#else
		tm time;
		return *gmtime_r( &nTime, &time );
#endif
	}

    STime GetFormatTimeSTMFromMillisecond( int64 nMillisecond )
    {
		time_t nTime = (time_t)( nMillisecond / 1000 );
#ifdef _WIN32
		tm gmtm = *gmtime( &nTime );
#else
		tm gmtm;
		gmtime_r( &nTime, &gmtm );
#endif
        STime stm;
        stm.m_nSecond = gmtm.tm_sec;
        stm.m_nMinutes = gmtm.tm_min;
        stm.m_nHour = gmtm.tm_hour;
        stm.m_nDayOfMonth = gmtm.tm_mday;
        stm.m_nMonth = gmtm.tm_mon + 1;
        stm.m_nYear = gmtm.tm_year + 1900;
        stm.m_nDayofWeek = gmtm.tm_wday;    // 返回0
        stm.m_nDayOfYear =gmtm.tm_yday + 1;
        stm.m_nIsdst = gmtm.tm_isdst;
        return stm;
    }

	uint32 GetOffSetTime( uint64 nNatureTime )
	{
		STime ST = GetFormatTimeSTMFromMillisecond( nNatureTime );
		int32 nDay = ST.m_nDayofWeek - 1;

		if ( nDay < 0 )
			nDay = 6;

		uint32 nScecond = ( nDay * 24 + ST.m_nHour ) * 3600 + ST.m_nMinutes * 60 + ST.m_nSecond;
		return nScecond; 
	}

	int64 Format2NatureTime( uint32 nYear, uint32 nMon, uint32 nDay, uint32 nHour, uint32 nMin, uint32 nSec, uint32 nMSec )
	{
		time_t rawtime;
		time ( &rawtime );
#ifdef _WIN32
		tm Time = *::localtime( &rawtime );
#else
		tm Time;
		::localtime_r( &rawtime, &Time );
#endif

		Time.tm_year	= nYear - 1900;
		Time.tm_mon		= nMon - 1;
		Time.tm_mday	= nDay;
		Time.tm_hour	= nHour;
		Time.tm_min		= nMin;
		Time.tm_sec		= nSec;
		int64 nTime = mktime( &Time );
		return nTime >= 0 ? nTime*1000 + nMSec : -1;
	}

	/// 获取指定时刻的当地时间(s)
	int64 Format2LocalTime( uint32 nYear, uint32 nMon, uint32 nDay, uint32 nHour, uint32 nMin, uint32 nSec )
	{
		time_t rawtime;
		time ( &rawtime );
#ifdef _WIN32
		tm Time = *::localtime( &rawtime );
#else
		tm Time;
		::localtime_r( &rawtime, &Time );
#endif

		Time.tm_year	= nYear - 1900;
		Time.tm_mon		= nMon - 1;
		Time.tm_mday	= nDay;
		Time.tm_hour	= nHour;
		Time.tm_min		= nMin;
		Time.tm_sec		= nSec;
		int64 nTime		= mktime( &Time );
		return nTime >= 0 ? nTime + g_nZoneTime / 1000 : -1;
	}

	int64 Str2Time( const char* szTime )
	{
		char szBuf[32];
		uint32 nLen = 0, i = 0;
		while( nLen < 14 )
		{
			char c = szTime[i++];
			if( !IsNumber( c ) )
			{
				// 必须大于1970年
				if( nLen < 4 )
					return 0;

				if( nLen&1 )
				{
					szBuf[nLen] = szBuf[ nLen - 1 ];
					szBuf[ nLen - 1 ] = '0';
					nLen++;
				}

				if( c )
					continue;
				break;
			}
			szBuf[nLen++] = c;
		}

		while( nLen < 14 )
			szBuf[nLen++] = '0';

		szBuf[nLen] = 0;
		int32 nSec	= atoi( szBuf + 12 ); szBuf[12] = 0;
		int32 nMin	= atoi( szBuf + 10 ); szBuf[10] = 0;
		int32 nHour = atoi( szBuf + 8  ); szBuf[8 ] = 0;
		int32 nDay	= atoi( szBuf + 6  ); szBuf[6 ] = 0;
		int32 nMon	= atoi( szBuf + 4  ); szBuf[4 ] = 0;
		int32 nYear = atoi( szBuf + 0  ); szBuf[0 ] = 0;
		return Format2NatureTime( nYear, nMon, nDay, nHour, nMin, nSec )/1000;
	}

	int64 Str2LocalTime( const char* szTime )
	{
		char szBuf[32];
		uint32 nLen = 0, i = 0;
		while( nLen < 14 )
		{
			char c = szTime[i++];
			if( !IsNumber( c ) )
			{
				// 必须大于1970年
				if( nLen < 4 )
					return 0;

				if( nLen&1 )
				{
					szBuf[nLen] = szBuf[ nLen - 1 ];
					szBuf[ nLen - 1 ] = '0';
					nLen++;
				}

				if( c )
					continue;
				break;
			}
			szBuf[nLen++] = c;
		}

		while( nLen < 14 )
			szBuf[nLen++] = '0';

		szBuf[nLen] = 0;
		int32 nSec	= atoi( szBuf + 12 ); szBuf[12] = 0;
		int32 nMin	= atoi( szBuf + 10 ); szBuf[10] = 0;
		int32 nHour = atoi( szBuf + 8  ); szBuf[8 ] = 0;
		int32 nDay	= atoi( szBuf + 6  ); szBuf[6 ] = 0;
		int32 nMon	= atoi( szBuf + 4  ); szBuf[4 ] = 0;
		int32 nYear = atoi( szBuf + 0  ); szBuf[0 ] = 0;
		return Format2LocalTime( nYear, nMon, nDay, nHour, nMin, nSec );
	}

    //==========================================================
    // 帧率统计
    //==========================================================
    CFPS::CFPS() 
        : m_fFPS(0)
        , m_nFrameCount(0)
        , m_nPreTime(0)
    {
    }

    float CFPS::GetFPS()
    {
        m_nFrameCount++;
        int64 nCurTime = GetTimeFromMechineStart();
        if( nCurTime - m_nPreTime >= 1000 )
        {
            m_fFPS = ( m_nFrameCount*1000.0f )/( nCurTime - m_nPreTime );
            m_nPreTime = nCurTime;
            m_nFrameCount = 0;
        }
        return m_fFPS;
    }


    //========================================================================
    // 定时器
    // 用于定时触发事件
    // virtual void OnTick() = 0;
    //========================================================================
    CTick::CTick( bool bCppTickOnly )
        : m_pMgr( NULL )
        , m_nTickInterval( 0 )
		, m_nNextTickTime( 0 )
		, m_nTickID( INVALID_16BITID )
		, m_bCppTickOnly( bCppTickOnly )
    {
    }

    CTick::~CTick()
    {
       Stop();
    }

	int64 CTick::GetPassTime() const
	{
		int64 nPassTime = m_pMgr ? m_pMgr->GetCurUpdateTime() - GetStartTime() : 0;
		GammaAst( nPassTime >= 0 );
		return nPassTime;
	}

	void CTick::Stop()
	{
		if( !m_pMgr )
			return;
		m_pMgr->DelTick( this );
	}

    //========================================================================
    // 用于管理定时器
    // 时间只可以前进，不可后退
    // void Update( uint32 nDeltaTime )
    //========================================================================
	#define MAX_TICK_ARRAY_SIZE 4096

    CTickMgr::CTickMgr( gammacstring strName,
		uint16 nTickArraySize, bool bEnableTickID ) 
        : m_strName( strName )
		, m_nPreUpdateTime(0)
		, m_pCurUpdateTick(NULL)
		, m_aryTickEnable(NULL)
    {
#ifdef GAMMA_CHECK_PROFILE
		memset( m_pTickProfile, 0, sizeof(m_pTickProfile) );
#endif

		if( bEnableTickID )
		{
			m_aryTickEnable = new uint8[( INVALID_16BITID + 1 )/8 ];
			memset( m_aryTickEnable, 0xff, ( INVALID_16BITID + 1 )/8 );
		}

		if( nTickArraySize > MAX_TICK_ARRAY_SIZE )
			nTickArraySize = MAX_TICK_ARRAY_SIZE;
		m_nTickArraySize = (uint16)AligenUpTo2Power( nTickArraySize );
		m_nTickMask = m_nTickArraySize - 1;
		m_aryTickTime = new TickList[m_nTickArraySize];
    }

    CTickMgr::~CTickMgr()
	{
		SAFE_DEL_GROUP( m_aryTickEnable );
		ClearAllTicks();
		SAFE_DEL_GROUP( m_aryTickTime );
    }

	void CTickMgr::Reset()
	{
		GammaAst( m_pCurUpdateTick == NULL );
		m_nPreUpdateTime = 0;
	}
	
	void CTickMgr::ClearAllTicks()
	{
		for( uint32 i = 0; i < m_nTickArraySize; ++i )
		{
			TickList& List = m_aryTickTime[i];
			while( List.GetFirst() )
				DelTick( List.GetFirst() );
		}
	}

	void CTickMgr::AddTick( CTick* pTick, uint32 nInterval, uint16 nTickID )
	{
		AddTick( pTick, nInterval, nInterval, nTickID );
	}

    void CTickMgr::AddTick( CTick* pTick, uint32 nStart, uint32 nInterval, uint16 nTickID )
    {
		if( pTick->m_pMgr )
			DelTick( pTick );

		if( nInterval == 0 )
		{
			GammaErr << "cannot set 0," << nTickID << std::endl;
			return;
		}

        pTick->m_pMgr = this;
        pTick->m_nTickInterval = nInterval;
		pTick->m_nTickID = nTickID;
		
        // 将Tick注册到相应的时间刻度处
		InsertTick( pTick, m_nPreUpdateTime + nStart );
    }

    void CTickMgr::DelTick( CTick* pTick )
    {
		if( !pTick )
			return;

        // 被删除的Tick必须是本管理器所管理的Tick
        GammaAst( !pTick->m_pMgr || pTick->m_pMgr == this );//--? 如果是ptick被其他的tickMgr拥有，则未删除 还将保留在其他的tickMgr
        
		pTick->Remove();//只是从链表中脱离  delete需要自己调用
        pTick->m_pMgr			= NULL;
        pTick->m_nNextTickTime	= 0;
        pTick->m_nTickInterval	= 0;
		pTick->m_nTickID		= INVALID_16BITID;

		if( m_pCurUpdateTick == pTick )
			m_pCurUpdateTick	= NULL;
    }

	void CTickMgr::InsertTick( CTick* pTick, int64 nTickTime )
	{
		GammaAst( !pTick->IsInList() );
		GammaAst( pTick->m_pMgr == this );
		GammaAst( pTick->m_nTickInterval );
		pTick->m_nNextTickTime = nTickTime;
		m_aryTickTime[pTick->m_nNextTickTime&m_nTickMask].PushBack( *pTick );//某时间刻 的tick
	}

    void CTickMgr::Update( uint32 nDeltaTime )
	{
		#ifdef GAMMA_CHECK_PROFILE
		PROFILE_FUNCTION( Tick_Update );
		#endif
		int64 nEndTime = m_nPreUpdateTime + nDeltaTime; 
		//遍历tick List 数组中  对数组中属于调用时间内的所有list进行处理
        for( ; m_nPreUpdateTime < nEndTime; m_nPreUpdateTime++ )
		{
			TickList IgnoreTickList;

           const uint32 nPos = (uint32)( m_nPreUpdateTime&m_nTickMask );
		   //遍历list
            while( m_aryTickTime[nPos].GetFirst() )
            {
                // 取出第一个Tick，并且对已经到达时间刻度的Tick调用OnTick()
				m_pCurUpdateTick = m_aryTickTime[nPos].GetFirst();
				m_pCurUpdateTick->Remove();
				GammaAst( m_pCurUpdateTick != m_aryTickTime[nPos].GetFirst() );

                // 如果Tick的Update时间不等于当前时间，则说明此Tick的间隔大于等于nTickTimeArraySize，
				// 将此Tick放入忽略链表
                if( m_pCurUpdateTick->m_nNextTickTime != m_nPreUpdateTime )
				{
					IgnoreTickList.PushBack( *m_pCurUpdateTick );
					GammaAst( IgnoreTickList.GetLast() == m_pCurUpdateTick );
					continue;
				}

#ifdef GAMMA_CHECK_PROFILE
				if( m_pTickProfile[m_pCurUpdateTick->m_nTickID] == NULL )
				{
					char szLabel[64];
					sprintf( szLabel, "%s_%d", m_strName.c_str(), m_pCurUpdateTick->m_nTickID );
					m_pTickProfile[m_pCurUpdateTick->m_nTickID] = GetProfileMgr().CreateProfile( __FILE__, __LINE__, __FUNCTION__, szLabel );
				}

				CProfile* pProfile = m_pTickProfile[m_pCurUpdateTick->m_nTickID];
				pProfile->CheckBegin();
#endif
				uint16 nTickID = m_pCurUpdateTick->m_nTickID;
				uint32 nTickInterval = m_pCurUpdateTick->GetInteval();
				try
				{
					// Tick没有关闭
					if( !m_aryTickEnable || ( m_aryTickEnable[nTickID/8] & ( 1 << ( nTickID%8 ) ) ) )
					{
						if( m_pCurUpdateTick->m_bCppTickOnly )
							m_pCurUpdateTick->OnCppTick();
						else
							m_pCurUpdateTick->OnTick();
					}
				}
				catch( ... )
				{
					GammaErr << "CTickMgr update tick error tickid = " 
						<< nTickID << ", tick interval = " << nTickInterval << std::endl; 
				}

#ifdef GAMMA_CHECK_PROFILE
				pProfile->CheckEnd();
#endif
                // 如果在调用OnTick()过程中m_pCurUpdateTick没有被删除，则推动Tick继续前进
                if( m_pCurUpdateTick )
                {
					GammaAst( m_pCurUpdateTick->m_nNextTickTime >= m_nPreUpdateTime );
					InsertTick( m_pCurUpdateTick, m_nPreUpdateTime + m_pCurUpdateTick->m_nTickInterval );
                }
            }

			while( IgnoreTickList.GetFirst() )
			{
				CTick* pCurUpdateTick = IgnoreTickList.GetFirst();
				pCurUpdateTick->Remove();
				InsertTick( pCurUpdateTick, pCurUpdateTick->m_nNextTickTime );
			}

			m_pCurUpdateTick = NULL;
        }
    }

	bool CTickMgr::EnableTick( uint16 nTickID, bool bEnable )
	{
		if( !m_aryTickEnable )
			return false;
		if( bEnable )
			m_aryTickEnable[nTickID/8] |= 1 << ( nTickID%8 );
		else
			m_aryTickEnable[nTickID/8] &= ~( 1 << ( nTickID%8 ) );
		return true;
	}

	bool CTickMgr::IsTickEnable( uint16 nTickID )
	{
		if( !m_aryTickEnable )
			return true;
		return ( m_aryTickEnable[nTickID/8] & ( 1 << ( nTickID%8 ) ) ) != 0;
	}

	void CTickMgr::SetCurUpdateTime( int64 nValue )
	{
		int64 nDlt = nValue - m_nPreUpdateTime;
		GammaLog << "SetCurUpdateTime," << nValue << "," << m_nPreUpdateTime  << std::endl;
		m_nPreUpdateTime = nValue;

		TickList listTick[MAX_TICK_ARRAY_SIZE];
		for( uint32 i = 0; i < m_nTickArraySize; ++i )
		{
			TickList& List = m_aryTickTime[i];

			while( List.GetFirst() )
			{
				CTick* pNode = List.GetFirst();
				pNode->m_nNextTickTime += nDlt; 
				pNode->Remove();
				uint32 nIndex = pNode->m_nNextTickTime&m_nTickMask;
				listTick[nIndex].PushBack( *pNode );
			}
		}

		if( m_pCurUpdateTick && !m_pCurUpdateTick->IsInList() )
			m_pCurUpdateTick->m_nNextTickTime += nDlt; 

		for( uint32 i = 0; i < m_nTickArraySize; ++i )
		{
			TickList& List = listTick[i];

			while( List.GetFirst() )
			{
				CTick* pNode = List.GetFirst();
				pNode->Remove();
				uint32 nIndex = pNode->m_nNextTickTime&m_nTickMask;
				m_aryTickTime[nIndex].PushBack( *pNode );
			}
		}
	}

}
