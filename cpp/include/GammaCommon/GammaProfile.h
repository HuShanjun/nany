//===============================================
// GammaProfile.h 
// 定义性能分析类
// 柯达昭
// 2009-12-16
//===============================================
#pragma once

#include <vector>
#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/TGammaRBTree.h"
#include "GammaCommon/TConstString.h"

namespace Gamma
{	
#ifdef _WIN32
	class CProfile;
	template class GAMMA_COMMON_API TGammaRBTree<CProfile>::CGammaRBTreeNode;
	template class GAMMA_COMMON_API TGammaRBTree<CProfile>;
#endif

	//===================================
	// CProfile 性能分析数据收集器   
	//===================================
	class GAMMA_COMMON_API CProfile 
		: public TGammaRBTree<CProfile>::CGammaRBTreeNode
	{
		const char*		m_szFile;
		const char*		m_szFunction;
		const char*		m_szLabel;
		uint32_t			m_nLine;
		uint64_t			m_nPreCheckCpuTick;
		int64_t			m_nCurDelta;
		uint32_t			m_nContext;

		uint32_t			m_nLogCount;
		uint32_t			m_nCurFrame;
		uint32_t			m_nCurFrameInfoIndex;

		CProfile( const char* szFile, uint32_t nLine, 
			const char* szFunction, const char* szLabel );
		~CProfile();
		friend class CProfileMgr;
		typedef gammacstring str;
	public:
		const char*		GetFileName()				{ return m_szFile;					}
		const char*		GetFunctionName()			{ return m_szFunction;				}
		uint32_t			GetLine()					{ return m_nLine;					}
		const char*		GetLabel()					{ return m_szLabel;					}
		int64_t			GetCurDelta()				{ return m_nCurDelta;				}
		uint32_t			GetContext()				{ return m_nContext;				}
		uint32_t			GetLogCount()				{ return m_nLogCount;				}
		void			SetContext( uint32_t n )		{ m_nContext = n;					}
		operator		gammacstring() const		{ return str( m_szLabel, true );	}	 

		bool			operator < ( const gammacstring& strLabl ) const;
		void			CheckBegin();
		void			CheckEnd();
	};

	//===================================
	// CBlockProfile 分析数据块内收集器
	//===================================
	class CBlockProfile
	{
		CProfile* m_pPropFile;
	public:
		CBlockProfile( CProfile* pPropFile )
			: m_pPropFile( pPropFile )
		{
			m_pPropFile->CheckBegin();
		}

		~CBlockProfile()
		{
			m_pPropFile->CheckEnd();
		}
	};

	//===================================
	// IProfileMgr 性能分析数据管理器
	//===================================
	class IProfileMgr
	{
	public:
		virtual CProfile* CreateProfile( const char* szFile, uint32_t nLine, const char* szFunction, const char* szLabel ) = 0;
		virtual CProfile* GetFirstProfile() = 0;
		virtual CProfile* GetNextProfile( CProfile* pProfile ) = 0;
		virtual CProfile* GetProfile( const char* szLabel ) = 0;
		virtual void	  FrameMove() = 0;
		virtual uint32_t	  GetCurFrame() = 0;
		virtual void	  GetFrameRange( uint32_t& nStart, uint32_t& nEnd ) = 0;
		virtual uint32_t	  GetFrameProfileCount( uint32_t nFrame ) = 0;
		virtual uint32_t	  GetFrameMaxCost( uint32_t nFrame ) = 0;
		virtual CProfile* GetFrameProfile( uint32_t nFrame, uint32_t nIndex, uint32_t& nCostTime, uint32_t& nHitCount ) = 0;
	};

	GAMMA_COMMON_API IProfileMgr& GetProfileMgr();
	GAMMA_COMMON_API uint64_t GetProfileTickCount();
};

//#define GAMMA_CHECK_PROFILE

#ifdef GAMMA_CHECK_PROFILE

	#define PROFILE_START( Label ) \
		static CProfile* pProfile_##Label = GetProfileMgr().CreateProfile( __FILE__, __LINE__, __FUNCTION__, #Label );\
		pProfile_##Label->CheckBegin();

	#define PROFILE_END( Label ) \
		pProfile_##Label->CheckEnd();	

	#define PROFILE_FUNCTION( Label ) \
		static CProfile* pProfile_##Label = GetProfileMgr().CreateProfile( __FILE__, __LINE__, __FUNCTION__, #Label );\
		CBlockProfile Profile_##Label( pProfile_##Label );

#else

	#define PROFILE_START( Label )
	#define PROFILE_END( Label )
	#define PROFILE_FUNCTION( Label ) 

#endif

