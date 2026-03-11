#pragma once
#include "GammaCommon/GammaCommonType.h"
#include "GammaCommon/IGammaUnknow.h"

#if ( defined( _WIN32 ) && defined( GAMMA_DLL ) )
#if defined( GAMMA_GRAPHIC_EXPORTS )
#define GAMMA_SHM_API __declspec(dllexport)
#else
#define GAMMA_SHM_API __declspec(dllimport)
#endif
#else
#define GAMMA_SHM_API 
#endif

namespace Gamma
{
	struct SShareCommonHead;
	struct SCommitBlocksInfo;
	struct SCommitFlag;

	class IShareMemoryHandler
	{
	public:
		virtual void				OnReboot() = 0;
		virtual void				OnCommitBlocksInfo( const SCommitBlocksInfo* pInfo, size_t nSize ) = 0;
		virtual void				OnCommitFlag( const SCommitFlag* pInfo, size_t nSize ) = 0;
		virtual void				OnCommitAllBlocksOK( const SCommitFlag* pInfo, size_t nSize ) = 0;
	};

	class IShareMemoryMgr : public IGammaUnknown
	{
	public:
		virtual SShareCommonHead*	GetShareMemory() = 0;
		virtual void				SetCheckInterval( uint32_t nInterval ) = 0;
		virtual void				Start() = 0;
		virtual bool				IsReady() = 0;
		virtual void				Check() = 0;
		virtual void				NotifyAppClose() = 0;

		virtual void				OnRebootFinish( bool bFailed ) = 0;
		virtual void				OnCommitAllBlocksOKFinish( const SCommitFlag* pInfo, bool bFailed ) = 0;
		virtual void				OnCommitBlocksFinish( const SCommitBlocksInfo* pInfo, bool bFailed ) = 0;
		virtual void				OnCommitFlagFinish( const SCommitFlag* pInfo, bool bFailed ) = 0;
	};

	GAMMA_SHM_API IShareMemoryMgr* CreateShareMemoryMgr( uint16_t nGasID,
		SShareCommonHead* pSrc, const char* szFileName, IShareMemoryHandler* pHandler,
		uint32_t nCommitInterval, bool bKeepShmFile, uint32_t nFlushInterval );
}
