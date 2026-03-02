
#include "GammaCommon/GammaProfile.h"
#include "GammaCommon/CThread.h"
#include "GammaCommon/TConstString.h"
#include "GammaCommon/GammaTime.h"
#include <set>

#ifndef _WIN32
#include <sys/time.h>
#endif

#ifdef _ANDROID
#include <jni.h>
#endif

namespace Gamma
{
	struct SFrameProfile
	{
		CProfile*	m_pProfile;
		uint32_t		m_nCostTime;
		uint32_t		m_nHitCount;
	};

	struct SFrameInfo
	{
		SFrameInfo() : m_nFrameInfoCount(0), m_nFrameInfoMaxCount(0){}
		std::vector<SFrameProfile>		m_vecFrameInfo;
		uint32_t							m_nFrameInfoCount;
		uint32_t							m_nFrameInfoMaxCount;
		uint32_t							m_nMaxCostTime;
	};

	class CProfileMgr : public IProfileMgr
	{
	public:
		TGammaRBTree<CProfile>			m_mapProfile;
		CLock							m_Lock;
		uint32_t							m_nCurFrame;
		SFrameInfo						m_aryFrames[4096];

		CProfileMgr()
			: m_nCurFrame( 0 )
		{
		}

		virtual ~CProfileMgr()
		{
			while( m_mapProfile.GetFirst() )
				delete m_mapProfile.GetFirst();
		}

		static CProfileMgr& Instance()
		{
			static CProfileMgr s_instance;
			return s_instance;
		}

		CProfile* CreateProfile( const char* szFile, uint32_t nLine, const char* szFunction, const char* szLabel )
		{
			gammacstring strLabel( szLabel, true );
			if( m_mapProfile.find( strLabel ) != m_mapProfile.end() )
				GammaAbort( "CreateProfile with exist name!!" );
			m_Lock.Lock();
			CProfile* pProfile = new CProfile( szFile, nLine, szFunction, szLabel );
			m_mapProfile.Insert( *pProfile );
			m_Lock.Unlock();
			return pProfile;
		}

		CProfile* GetFirstProfile()
		{
			return m_mapProfile.GetFirst();
		}

		CProfile* GetNextProfile( CProfile* pProfile )
		{
			return pProfile ? pProfile->GetNext() : NULL;
		}

		CProfile* GetProfile( const char* szLabel )
		{
			return m_mapProfile.Find( gammacstring( szLabel, true ) );
		}
		
		void FrameMove()
		{			
			uint32_t nCurIndex = ( ++m_nCurFrame )&0xfff;
			SFrameInfo& FrameInfo = m_aryFrames[nCurIndex];
			FrameInfo.m_nFrameInfoCount = 0;
			FrameInfo.m_nMaxCostTime = 0;
		}
		
		uint32_t GetCurFrame()
		{
			return m_nCurFrame;
		}
		
		void GetFrameRange( uint32_t& nStart, uint32_t& nEnd )
		{
			nEnd = m_nCurFrame;
			nStart = m_nCurFrame >= 4096 ? m_nCurFrame - 4096 : 0;
		}
		
		uint32_t GetFrameProfileCount( uint32_t nFrame )
		{
			return (uint32_t)( m_aryFrames[nFrame&0xfff].m_nFrameInfoCount );
		}
		
		uint32_t GetFrameMaxCost( uint32_t nFrame )
		{
			return m_aryFrames[nFrame&0xfff].m_nMaxCostTime;
		}
		
		CProfile* GetFrameProfile( uint32_t nFrame, uint32_t nIndex, uint32_t& nCostTime, uint32_t& nHitCount )
		{
			std::vector<SFrameProfile>& vecFrameProfile = m_aryFrames[nFrame&0xfff].m_vecFrameInfo;
			if( nIndex >= vecFrameProfile.size() )
				return NULL;
			SFrameProfile FrameProfile = vecFrameProfile[nIndex];
			nCostTime = FrameProfile.m_nCostTime;
			nHitCount = FrameProfile.m_nHitCount;
			return FrameProfile.m_pProfile;
		}
	};

	IProfileMgr& GetProfileMgr(){ return CProfileMgr::Instance(); }

#ifdef _WIN32
	inline uint64_t GetCycleCount()
	{ 
		//__asm _emit 0x0F __asm _emit 0x31 
		static uint64_t gPerformanceFrequency = 0;		
		if (gPerformanceFrequency == 0)
			QueryPerformanceFrequency((LARGE_INTEGER*)&gPerformanceFrequency);

		LARGE_INTEGER value;
		QueryPerformanceCounter(&value);
		uint64_t nSecond = value.QuadPart/gPerformanceFrequency;
		uint64_t nTick = value.QuadPart%gPerformanceFrequency;
		return nSecond*1000000 + ( nTick*1000000 )/gPerformanceFrequency;
	} 
//#elif defined X86
//	inline uint64_t GetCycleCount() { int64_t n; __asm__ __volatile__ ( "rdtsc" : "=A" (n) ); return n;	}
#else
	inline uint64_t GetCycleCount() 
	{ 
		struct timeval tv;
		::gettimeofday(&tv, NULL);
		uint64_t seconds = (uint64_t)tv.tv_sec * 1000000;
		uint64_t microseconds = (uint64_t)tv.tv_usec;
		uint64_t result = seconds + microseconds;
		return result;
	}
#endif

	uint64_t GetProfileTickCount()
	{
		return GetCycleCount();
	}

	//===============================================
	// profile 实现  
	//===============================================
	CProfile::CProfile( const char* szFile, uint32_t nLine, 
		const char* szFunction, const char* szLabel )
		: m_szFile( szFile )
		, m_szFunction( szFunction )
		, m_nLine( nLine )
		, m_nPreCheckCpuTick( 0 )
		, m_nContext( 0 )
		, m_nCurFrame( INVALID_32BITID )
		, m_nCurFrameInfoIndex( INVALID_32BITID )
	{
		if( !szLabel || !szLabel[0] )
		{
			m_szLabel = "";
			return;
		}

		uint32_t nLen = (uint32_t)strlen( szLabel );
		char* szBuffer = new char[nLen + 1];
		memcpy( szBuffer, szLabel, nLen + 1 );
		m_szLabel = szBuffer;
	}

	CProfile::~CProfile()
	{
		if( !m_szLabel || !m_szLabel[0] )
			return;
		SAFE_DELETE( m_szLabel );
	}

	bool CProfile::operator < ( const gammacstring& strLabl ) const
	{
		return ( (gammacstring)*this ) < strLabl;
	}

	void CProfile::CheckBegin()
	{
		m_nPreCheckCpuTick = GetCycleCount();
	}

	void CProfile::CheckEnd()
	{
		CProfileMgr& Mgr = CProfileMgr::Instance();
		uint32_t nCurFrame = Mgr.m_nCurFrame;
		m_nCurDelta = GetCycleCount() - m_nPreCheckCpuTick;

		uint32_t nIndex = nCurFrame&0xfff;
		SFrameInfo& FrameInfo = Mgr.m_aryFrames[nIndex];
		std::vector<SFrameProfile>& vecFrameProfile = FrameInfo.m_vecFrameInfo;

		if( m_nCurFrame != nCurFrame )
		{
			m_nCurFrame = nCurFrame;
			m_nCurFrameInfoIndex = FrameInfo.m_nFrameInfoCount++;
			if( FrameInfo.m_nFrameInfoCount > FrameInfo.m_nFrameInfoMaxCount )
			{
				FrameInfo.m_nFrameInfoMaxCount = FrameInfo.m_nFrameInfoCount;
				vecFrameProfile.resize( FrameInfo.m_nFrameInfoMaxCount );
			}
			SFrameProfile& FrameProfile = vecFrameProfile[m_nCurFrameInfoIndex];
			FrameProfile.m_nCostTime = 0;
			FrameProfile.m_nHitCount = 0;
			FrameProfile.m_pProfile = this;
		}

		SFrameProfile& FrameProfile = vecFrameProfile[m_nCurFrameInfoIndex];
		FrameProfile.m_nCostTime += (uint32_t)m_nCurDelta;
		FrameProfile.m_nHitCount++;
		FrameInfo.m_nMaxCostTime = Max( FrameInfo.m_nMaxCostTime, FrameProfile.m_nCostTime );
	}
}
