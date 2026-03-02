#pragma once
#include "GammaCommon/GammaCommonType.h"
#include "GammaCommon/IGammaUnknow.h"

#if ( defined( _WIN32 ) && defined( GAMMA_DLL ) )
	#if defined( GAMMA_MTDBS_EXPORTS )
		#define GAMMA_MTDBS_API __declspec(dllexport)
	#else
		#define GAMMA_MTDBS_API __declspec(dllimport)
	#endif
#else
#define GAMMA_MTDBS_API 
#endif

namespace Gamma
{
	class IDbsQuery;
	class IDbsQueryHandler;
	class IQuerryCmd;

	struct SDbsCreateParam
	{
		int32_t		nDBId;		//数据库唯一表示
		const char* szDbsHost;
		uint16_t		nDbsPort;
		const char*	szDataBase;
		const char* szUser;
		const char* szPassword;
		bool		bFoundAsUpdateRow;
	};

	class IDbsThreadMgr : public IGammaUnknown
	{
	public:
		virtual IDbsQuery*			CreateDbsQuery( IDbsQueryHandler* pHandler ) = 0;
		virtual void				Check() = 0;
		virtual uint32_t				GetChannelCmdWaitingCount( uint32_t nChannel ) = 0;
		virtual uint32_t				GetAllCmdWaitingCount() = 0;
		virtual uint32_t				GetChannelResultWaitingCount( uint32_t nChannel ) = 0;
		virtual uint32_t				GetAllResultWaitingCount() = 0;
	};

	GAMMA_MTDBS_API IDbsThreadMgr*	CreateDbsThreadMgr( uint32_t nThreadCount, 
		uint32_t nDbsConnClassID, const SDbsCreateParam* aryParam, uint8_t nDbsCount,
		uint32_t nRecordThreadCount, const Gamma::SDbsCreateParam* _aryRecordParam, uint8_t _nRecordDbsCount);

	GAMMA_MTDBS_API IDbsThreadMgr*	CreateDbsThreadMgr( 
		uint32_t nThreadCount, uint32_t nDbsConnClassID, const char* szDbsHost, uint16_t nDbsPort, 
		const char* szDatabase, const char* szUser, const char* szPassword, bool bFoundAsUpdateRow );
}
