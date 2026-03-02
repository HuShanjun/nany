#pragma once

#include "GammaCommon/GammaCommonType.h"

#if ( defined( _WIN32 ) && defined( GAMMA_DLL ) )
	#if defined( GAMMA_DATABASE_EXPORTS )
		#define GAMMA_DATABASE_API __declspec(dllexport)
	#else
		#define GAMMA_DATABASE_API __declspec(dllimport)
	#endif
#else
	#define GAMMA_DATABASE_API 
#endif

namespace Gamma
{
	enum EErrorCodes
	{
		eEC_Succeed		= 0, /// 成功
		eEC_Unkown		= -1,
		eEC_DupEntry	= -2,	 /// Duplicate entry '%s' for key %d
		eEC_DeadLock	= -3,
		eEC_ErrorCount	= 2000
	};

	class IDbTextResult
	{
	public:
		virtual uint32_t				GetRowNum() = 0;
		virtual uint32_t				GetColNum() = 0;

		virtual void				Locate( uint32_t uRowIndex ) = 0;
		//获取当前行某列的数据，数据以字符串的形式返回
		virtual const char*			GetData( uint32_t uColIndex ) = 0;
		virtual unsigned			GetDataLength( uint32_t uColIndex ) = 0;
		virtual void 				Release() = 0;
		virtual void 				SetTag( void* pTag ) = 0;
		virtual void*				GetTag() const = 0;
	};

	class IDbConnection;
	class IDbStatement;
	class ILog;

	class IDbStatement
	{
	public:				
		virtual void				FetchResultRow( uint32_t nIndex ) = 0;

		virtual uint32_t				GetParamNum() const = 0;
		virtual uint32_t				GetResultColNum() const = 0;
		virtual uint32_t				GetResultRowNum() const = 0;
		virtual	uint64_t				GetInsertID( void ) const = 0;
		virtual uint32_t				GetAffectRowNum() const = 0;		// 注意，在执行update时，如果设置的值跟原先的值一样，则返回的是0，除非创建连接的时候指定bFoundAsUpdateRow为true

		virtual IDbConnection*		GetConnection() const = 0;

		virtual void				Execute() = 0;
		virtual uint32_t				ExecuteWithoutException( void ) = 0;
		virtual const char*			GetExecuteError( void ) = 0;
		virtual void				Release() = 0;

		virtual void				SetTag( void* pTag ) = 0;
		virtual void*				GetTag()const = 0;

		// Bind parameters
		virtual void 				SetParamNull( uint32_t nIndex ) = 0;

		virtual void 				SetParamInt8( const int8_t* pBuffer, uint32_t nIndex ) = 0;
		virtual void 				SetParamUint8( const uint8_t* pBuffer, uint32_t nIndex ) = 0;

		virtual void 				SetParamInt16( const int16_t* pBuffer, uint32_t nIndex ) = 0;
		virtual void 				SetParamUint16( const uint16_t* pBuffer, uint32_t nIndex ) = 0;

		virtual void 				SetParamInt32( const int32_t* pBuffer, uint32_t nIndex ) = 0;
		virtual void				SetParamUint32( const uint32_t* pBuffer, uint32_t nIndex ) = 0;

		virtual void 				SetParamInt64( const int64_t* pBuffer, uint32_t nIndex ) = 0;
		virtual void 				SetParamUint64( const uint64_t* pBuffer, uint32_t nIndex ) = 0;

		virtual void 				SetParamFloat( const float* pBuffer, uint32_t nIndex ) = 0;
		virtual void				SetParamDouble( const double* pBuffer, uint32_t nIndex ) = 0;

		virtual void				SetParamText( const void* pBuffer, uint32_t nMaxSize, ulong* pActualSize, uint32_t nIndex ) = 0;
		virtual void				SetParamBinary( const void* pBuffer, uint32_t nMaxSize, ulong* pActualSize, uint32_t nIndex ) = 0;

		virtual void				SetParamDate( const void* pBuffer, uint32_t nIndex ) = 0;
		virtual void				SetParamDateTime( const void* pBuffer, uint32_t nIndex ) = 0;
		virtual void				SetParamTimeStamp( const void* pBuffer, uint32_t nIndex ) = 0;
		virtual void				SetParamTime( const void* pBuffer, uint32_t nIndex ) = 0;

		// Bind results
		virtual void				SetResultInt8( int8_t* pBuffer, bool* pIsNull, bool* pError, uint32_t uColIndex ) = 0;
		virtual void				SetResultUint8( uint8_t* pBuffer, bool* pIsNull, bool* pError, uint32_t uColIndex ) = 0;

		virtual void				SetResultInt16( int16_t* pBuffer, bool* pIsNull, bool* pError, uint32_t uColIndex ) = 0;
		virtual void				SetResultUint16( uint16_t* pBuffer, bool* pIsNull, bool* pError, uint32_t uColIndex ) = 0;

		virtual void				SetResultInt32( int32_t* pBuffer, bool* pIsNull, bool* pError, uint32_t uColIndex ) = 0;
		virtual void				SetResultUint32( uint32_t* pBuffer, bool* pIsNull, bool* pError, uint32_t uColIndex ) = 0;

		virtual void				SetResultInt64( int64_t* pBuffer, bool* pIsNull, bool* pError, uint32_t uColIndex ) = 0;
		virtual void				SetResultUint64( uint64_t* pBuffer, bool* pIsNull, bool* pError, uint32_t uColIndex ) = 0;

		virtual void				SetResultFloat( float* pBuffer, bool* pIsNull, bool* pError, uint32_t uColIndex ) = 0;
		virtual void				SetResultDouble( double* pBuffer, bool* pIsNull, bool* pError, uint32_t uColIndex ) = 0;

		virtual void				SetResultText( void* pBuffer, uint32_t uBufferSize, ulong* uDataSize, bool* pIsNull, bool* pError, uint32_t uColIndex ) = 0;
		virtual void				SetResultBinary( void* pBuffer, uint32_t uBufferSize, ulong* uDataSize, bool* pIsNull, bool* pError, uint32_t uColIndex ) = 0;
	};

	class IDbConnection
	{
	public:	
		virtual	uint32_t				GetDBId() = 0;
		virtual IDbTextResult*		Execute( const char* szSqlBuffer, uint32_t uSize ) = 0;
		virtual IDbTextResult*		Execute( const char* szSqlBuffer ) = 0;
		virtual IDbTextResult*		ExecuteProcedure(const char* szSqlProcedureBuffer, uint32_t uSize1,
										const char* szSqlResultBuffer, uint32_t uSize2) = 0;

		virtual IDbStatement*		CreateStatement( const char* szSqlBuffer, uint32_t uSqlLength ) = 0;
		virtual IDbStatement*		CreateStatement( const char* szSqlBuffer ) = 0;
		virtual IDbStatement*		CreateDynamicStatement( const char* szSqlBuffer, uint32_t uSqlLength ) = 0;
		virtual IDbStatement*		CreateDynamicStatement( const char* szSqlBuffer ) = 0;
		virtual uint32_t				GetTotalStatement() const = 0;

		virtual uint32_t				EscapeString( char* szOut,const char* szIn, uint32_t uInSize ) = 0;
		virtual uint64_t				LastInsertId() const = 0;
		virtual uint64_t				LastAffectedRowNum() const = 0;
		virtual	uint32_t				CheckMultiExecute() const = 0;
		virtual int32_t				GetFoundRow() = 0;

		virtual bool				Ping() = 0;

		virtual void				SetTag(void* pTag ) = 0;
		virtual void*				GetTag() const = 0;
		virtual void				SetLog(ILog* pLog ) = 0;
		virtual ILog*				GetLog() const = 0;

		virtual void				StartTran() = 0;
		virtual void				EndTran() = 0;
		virtual void				CancelTran() = 0;

		virtual void				Release() = 0;
	};

	class IDatabase
	{
	public:
		virtual IDbConnection* CreateConnection( const char* szHost,uint16_t uPort,
			const char* szUser,const char* szPassword, const char* szDatabase, uint32_t nDBId,
			bool bFoundAsUpdateRow, bool bMultiStatements, bool bPingReconnect ) = 0;

		virtual IDbConnection* CreateConnection( const char* szPipe,//under unix is this unixsocket
			const char* szUser,const char* szPassword, const char* szDatabase, uint32_t nDBId,
			bool bFoundAsUpdateRow, bool bMultiStatements, bool bPingReconnect ) = 0;

		virtual	EErrorCodes FormatCode( uint32_t nErrorCode ) = 0;
	};
	
	GAMMA_DATABASE_API IDatabase* GetDatabase();
}
 
