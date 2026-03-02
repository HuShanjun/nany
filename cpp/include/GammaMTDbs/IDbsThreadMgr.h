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
		int32		nDBId;		//数据库唯一表示
		const char* szDbsHost;
		uint16		nDbsPort;
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
		virtual uint32				GetChannelCmdWaitingCount( uint32 nChannel ) = 0;
		virtual uint32				GetAllCmdWaitingCount() = 0;
		virtual uint32				GetChannelResultWaitingCount( uint32 nChannel ) = 0;
		virtual uint32				GetAllResultWaitingCount() = 0;
	};

	GAMMA_MTDBS_API IDbsThreadMgr*	CreateDbsThreadMgr( uint32 nThreadCount, 
		uint32 nDbsConnClassID, const SDbsCreateParam* aryParam, uint8 nDbsCount,
		uint32 nRecordThreadCount, const Gamma::SDbsCreateParam* _aryRecordParam, uint8 _nRecordDbsCount);

	GAMMA_MTDBS_API IDbsThreadMgr*	CreateDbsThreadMgr( 
		uint32 nThreadCount, uint32 nDbsConnClassID, const char* szDbsHost, uint16 nDbsPort, 
		const char* szDatabase, const char* szUser, const char* szPassword, bool bFoundAsUpdateRow );
}
