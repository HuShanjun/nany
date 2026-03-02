//=====================================================================
// GammaTime.h 
// 定义时间相关的接口
// 柯达昭
// 2007-09-15
//=======================================================================

#ifndef __GAMMA_TIME_H__
#define __GAMMA_TIME_H__

#include "GammaCommon/GammaCommonType.h"
#include "GammaCommon/CGammaObject.h"
#include "GammaCommon/TConstString.h"
#include "GammaCommon/TGammaList.h"
#include "GammaCommon/GammaProfile.h"
#include <list>
#include <map>

#include "time.h"
namespace Gamma
{
	//========================================================================
	// struct STime 
	//========================================================================
	struct GAMMA_COMMON_API STime 
	{
		int32 m_nSecond;	/* seconds after the minute - [0,59] */
		int32 m_nMinutes;	/* minutes after the hour - [0,59] */
		int32 m_nHour;		/* hours since midnight - [0,23] */
		int32 m_nDayOfMonth;/* day of the month - [1,31] */
		int32 m_nMonth;		/* months since January - [1,12] */
		int32 m_nYear;		/* years since 0000, >= 1900 */
		int32 m_nDayofWeek;	/* days since Sunday - [0,6] */
		int32 m_nDayOfYear;	/* days since January 1 - [0,365] */
		int32 m_nIsdst;		/* daylight savings time flag */

		void Format2Str( char szOut[], uint32 nSize );
		bool IsSameDay( const STime& otherTime ) const;

		operator std::string()
		{
			char szOut[256];
			Format2Str( szOut, ELEM_COUNT(szOut) );
			return szOut;
		}

		inline bool operator < ( const STime& t2 ) const
		{   
			if ( m_nYear == t2.m_nYear )
			{            
				if ( m_nDayOfYear == t2.m_nDayOfYear )
				{
					if ( m_nHour == t2.m_nHour )
					{
						if ( m_nMinutes == t2.m_nMinutes )
							return m_nSecond < t2.m_nSecond;
						else
							return m_nMinutes < t2.m_nMinutes;
					}
					else
						return m_nHour < t2.m_nHour;
				}
				else
					return m_nDayOfYear < t2.m_nDayOfYear;
			}
			else
				return m_nYear < t2.m_nYear;
		}

		inline bool operator == ( const STime& t2 ) const
		{
			return !memcmp( this, &t2, sizeof(STime) );
		}

		inline bool operator > ( const STime& t2 ) const
		{
			return t2 < *this;
		}
	};

    //========================================================================
    // 引擎提供的统一时间函数
    // GetProcessTime()						得到进程启动到当前时刻的毫秒数
    // GetGammaTime()						得到逻辑时间
    // PauseLogicTime( bool bPause )		暂停逻辑时间
	// SetLogicTime( int64 nLogicTime )		设置逻辑时间（可前进，也可后退）
	// int64 GetNatureTime();				设置当前格林威治时间（毫秒，从1970年1月1日0时开始计时）
	// int64 GetNatureTime();				得到自然时间,即格林威治时间（毫秒，从1970年1月1日0时开始计时）
	// int64 NatureTime2LocalTime();		格林威治时间 -> 当前本地时间
    // STime GetFormatTimeSTMFromMillisecond( nMillisecond )  根据指定相对于1970年1月1日0时的毫秒转成日期结构(xophiix)
	// uint32 Str2Time						将20100831000000（或者2010-08-31 19:55:00）格式的字符串转为格林威治时间（秒）
    //========================================================================

	// GetProcessTime() 得到进程启动到当前时刻的毫秒数
	GAMMA_COMMON_API int64	GetProcessTime();
	// AddProcessTime() 将系统时间往前或者往后调整一段时间，会影响GammaTime、LocalTime和NatureTime，通常用于时间校正
	GAMMA_COMMON_API void	AddProcessTime( int64 nDeltaTime );

	// GetGammaTime() 得到逻辑时间
	GAMMA_COMMON_API int64	GetGammaTime();
	
	GAMMA_COMMON_API void	PauseGammaTime( bool bPause );
	GAMMA_COMMON_API bool	IsPauseGammaTime();
	GAMMA_COMMON_API void	SetGammaTime( int64 nLogicTime );
	GAMMA_COMMON_API void	SetGammaTimeScale( float fScale );
	GAMMA_COMMON_API float	GetGammaTimeScale();
	// int64 GetNatureTime(); 设置当前格林威治时间（毫秒，从1970年1月1日0时开始计时）
	GAMMA_COMMON_API void	SetNatureTime( int64 nNatureTime, int64 nZoneTime = 0 );

	//--逻辑上的机器启动时间戳+机器已运行时间
	GAMMA_COMMON_API int64	GetNatureTime();
	GAMMA_COMMON_API int64	GetLocalTime();
	GAMMA_COMMON_API int64	GetZoneTime();
	GAMMA_COMMON_API int64	GetProcessStartNatureTime();
	// int64 NatureTime2LocalTime();		格林威治时间 -> 当前本地时间
	//--nNatureTime应为格林威治时间
	GAMMA_COMMON_API int64	NatureTime2LocalTime( int64 nNatureTime );
	GAMMA_COMMON_API int64	LocalTime2NatureTime( int64 nLocalTime );
	GAMMA_COMMON_API STime	GetFormatTimeSTM();
	GAMMA_COMMON_API void	GetFormatTimeSTM( STime& stm );

	// 根据指定相对于1970年1月1日0时的毫秒转成日期结构(xophiix)
    GAMMA_COMMON_API STime	GetFormatTimeSTMFromMillisecond( int64 nMillisecond );

	GAMMA_COMMON_API tm		GetFormatTimeTM();
	/// 获取指定时刻的格林威治时间(s)
	GAMMA_COMMON_API int64	Format2NatureTime( uint32 nYear, uint32 nMon, uint32 nDay, 
								uint32 nHour = 0, uint32 nMin = 0, uint32 nSec = 0, uint32 nMSec = 0 );
	/// 获取指定时刻的当地时间(s)
	GAMMA_COMMON_API int64	Format2LocalTime( uint32 nYear, uint32 nMon, uint32 nDay, 
								uint32 nHour = 0, uint32 nMin = 0, uint32 nSec = 0 );

	// uint32 Str2Time 将20100831000000（或者2010-08-31 19:55:00）格式的本地时间字符串转为格林威治时间（秒）
	GAMMA_COMMON_API int64 Str2Time( const char* szTime );
	// uint32 Str2Time 将20100831000000（或者2010-08-31 19:55:00）格式的本地时间字符串转为本地时间（秒）即 格林威治时间+本地时差
	GAMMA_COMMON_API int64 Str2LocalTime( const char* szTime );

	//* nNatureTime 需要转换的自然时间，毫秒
	//* 返回值为偏离周一0点的秒数
	GAMMA_COMMON_API uint32 GetOffSetTime( uint64 nNatureTime );

	// 本地时间转换为本地时间显示（秒2010-08-31 19:55:00）( 或者格林威治时间转换为格林威治时间显示 ）  
	inline std::string Time2Str( int64 nSeconds ){ return (std::string)GetFormatTimeSTMFromMillisecond( nSeconds*1000 ); }

	// 格林威治时间转换为本地时间显示（秒2010-08-31 19:55:00）  
	inline std::string NatrueTime2Str( int64 nSeconds ) { return (std::string)GetFormatTimeSTMFromMillisecond( NatureTime2LocalTime( nSeconds*1000 ) ); }

    //========================================================================
    // 计时器
    // 用于记录时间流逝
    // void        Restart()        计时开始
    // uint64    GetStartTime()    计时开始时间
    // uint64    GetElapse()        计时到当前时刻经过的时间
    //========================================================================
    class GAMMA_COMMON_API CTimer
    {
        uint64    m_nBaseTime;
    public:
        CTimer(void)                      { Restart(); };
        void      Restart()               { m_nBaseTime = GetGammaTime(); };
        uint64    GetStartTime()    const { return m_nBaseTime; };
        uint64    GetElapse()       const { return GetGammaTime() - m_nBaseTime; }
    };


    //========================================================================
    // 帧率统计
    // 用于计算当前每秒的帧数
    // float GetFPS()            每帧调用一次，计算得到当前的帧率
    //========================================================================
    class GAMMA_COMMON_API CFPS
    {
        float     m_fFPS;
        uint64    m_nPreTime;
        uint32    m_nFrameCount;

    public:
        CFPS();
        float GetFPS();
    };


    //========================================================================
    // 定时器
    // 用于定时触发事件
    // virtual void OnTick() = 0;
    //========================================================================
    class CTick;
	class CTickMgr;
    
#ifdef _WIN32
	template class GAMMA_COMMON_API TGammaList<CTick>::CGammaListNode;
	template class GAMMA_COMMON_API TGammaList<CTick>;
#endif

    typedef TGammaList<CTick> TickList;
	
	class CProfile;

	class GAMMA_COMMON_API CTick 
		: public CGammaObject 
		, private TGammaList<CTick>::CGammaListNode
    {
        CTickMgr*		m_pMgr;
		int64			m_nNextTickTime;
        uint32			m_nTickInterval;
		uint16			m_nTickID;	//CTickMgr::AddTick中设置
		bool			m_bCppTickOnly;

        friend class CTickMgr;
		friend class TGammaList<CTick>;
		friend class TGammaList<CTick>::CGammaListNode;

    public:
        CTick( bool bCppTickOnly = false );
		virtual ~CTick();
		virtual void	OnCppTick(){};
		virtual void	OnTick(){};
		void			Stop();

		int64			GetPassTime()		const;
		uint16			GetTickID()			const { return m_nTickID; }
		uint32			GetInteval()		const { return m_nTickInterval; }
		int64			GetNextTickTime()	const { return m_nNextTickTime; }
		CTickMgr*		GetTickMgr()		const { return m_pMgr; }
		bool			IsRegisted()		const { return m_pMgr != NULL; }
		int64			GetStartTime()		const { return GetNextTickTime() - GetInteval(); }
    };


    //========================================================================
    // 用于管理定时器
    // 时间只可以前进，不可后退
    // void Update( uint32 nDeltaTime )
    //========================================================================
    class GAMMA_COMMON_API CTickMgr
    {
	protected:
		gammacstring	m_strName;
        int64			m_nPreUpdateTime;		// 上次更新时时间刻度更新到的位置
        CTick*			m_pCurUpdateTick;		// 用于遍历m_TickTimeArray时间刻度数组

		uint8*			m_aryTickEnable;		// 当TickID启用时用于开启关闭某些tick
		TickList*		m_aryTickTime;			// 时间刻度数组
		uint16			m_nTickArraySize;		// 对其到2的n次方，便于运算
		uint16			m_nTickMask;			// m_nTickArraySize - 1

#ifdef GAMMA_CHECK_PROFILE
		CProfile*		m_pTickProfile[65536];	 
#endif
		//--以tick的下次tick时间为&0xfff 作为id插入到即将相应的数组中
		void			InsertTick( CTick* pTick, int64 nTickTime );

    public:
		CTickMgr( 
			gammacstring strName = "", 
			uint16 nTickArraySize = 4096,
			bool bEnableTickID = true );
        ~CTickMgr();

		void			Reset();
		void			ClearAllTicks();
		void			AddTick( CTick* pTick, uint32 nInterval, uint16 nTickID );
        void			AddTick( CTick* pTick, uint32 nStart, uint32 nInterval, uint16 nTickID );
        void			DelTick( CTick* pTick );
        void			Update( uint32 nDeltaTime );
		bool			EnableTick( uint16 nTickID, bool bEnable );
		bool			IsTickEnable( uint16 nTickID );

        /// 当前更新时时间刻度更新到的位置(ms)
        int64			GetCurUpdateTime() const { return m_nPreUpdateTime; }
		void			SetCurUpdateTime( int64 nValue );
    };

	template<typename Class, bool bCppTick = false>
	class TTickFun
		: public CTick
	{
	public:
		typedef void (Class::*TickFun)();

	private:
		Class*		m_pClass;
		TickFun		m_funTick;

	public:
		TTickFun( Class* pClass, TickFun funTick )
			: m_pClass( pClass ), m_funTick( funTick )
			, CTick( bCppTick )
		{
		}

		void OnTick()
		{
			( m_pClass->*m_funTick )();
		}

		/** 如果使用了默认构造，那么在启动OnTick之前必须SetContext，并且外部保证 pClass和funTick的合法性。*/
		TTickFun()
			: m_pClass( NULL )
			, m_funTick( NULL )
		{}

		void SetContext( Class* pClass, TickFun funTick )
		{
			m_pClass = pClass;
			m_funTick = funTick;
		}

		TickFun	GetFun(void)
		{
			return m_funTick;
		}

		void OnCppTick() 
		{ 
			if ( !m_pClass )
			{
				GammaErr << "OnCppTick no owner," << GetTickID() << std::endl;
				return;
			}
			( m_pClass->*m_funTick )();
		}
	};


	/************************************************************************/
	/* 定义一些时间常用的常量                                               */
	/************************************************************************/
	enum	ETimeConst
	{        
		eTimeConst_DaySeconds	= 86400,		// 一天的秒数
		eTimeConst_DayMiSeconds = 86400000,		// 一天的毫秒数
        eTimeConst_HourSeconds = 3600,
        eTimeConst_HourMiliseconds = 3600000,
        eTimeConst_MinuteSeconds = 60,
        eTimeConst_MinuteMiliseconds = 60000,
	};
}

#endif
