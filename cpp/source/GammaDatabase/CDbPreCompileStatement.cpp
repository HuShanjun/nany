#include "GammaCommon/GammaHelp.h"
#include "CDbPreCompileStatement.h"
#include "CDbConnection.h"
#include <sstream>
#include <stdio.h>

using namespace std;

namespace Gamma
{
	CDbPreCompileStatement::CDbPreCompileStatement( 
		CDbConnection* pConn, const char* szSqlBuffer, uint32_t uSize )
		: CDbStatement( pConn, szSqlBuffer, uSize )
		, m_pStmt( nullptr )
		, m_aryParam( nullptr )
		, m_nParamNum( 0 )
		, m_bIsNull( true )
	{
		m_pStmt = mysql_stmt_init( m_pConn->m_pMySql );

		if( !m_pStmt )
		{
			char szErrBuff[65535];
			sprintf( szErrBuff, "mysql_stmt_init failed with error:%s:%s\n", mysql_stmt_error( m_pStmt ), szSqlBuffer );
			GammaThrow( szErrBuff );
		}

		if( mysql_stmt_prepare( m_pStmt, szSqlBuffer, uSize ) )
		{
			char szErrBuff[65535];
			sprintf( szErrBuff, "mysql_stmt_prepare failed with error:%s:%s\n", mysql_stmt_error( m_pStmt ), szSqlBuffer );
			GammaThrow( szErrBuff );
		}

		m_pConn->m_nTotalStatement++;
		m_pRes = mysql_stmt_result_metadata( m_pStmt );
		
		m_nParamNum = mysql_stmt_param_count( m_pStmt );
		m_nResultColNum = m_pRes ? mysql_num_fields( m_pRes ) : 0;

		if( GetParamNum() )
		{
			m_aryParam = new MYSQL_BIND[GetParamNum()];
			memset( m_aryParam, 0, sizeof( MYSQL_BIND ) * GetParamNum() );
		}

		if( GetResultColNum() )
		{
			m_aryResult = new MYSQL_BIND[GetResultColNum()];
			memset( m_aryResult, 0, sizeof( MYSQL_BIND ) * GetResultColNum() );
		}
	}

	CDbPreCompileStatement::~CDbPreCompileStatement( void )
	{
		delete[] m_aryParam;
		mysql_stmt_close( m_pStmt );
		m_pConn->m_nTotalStatement--;
	}

	uint32_t CDbPreCompileStatement::GetParamNum()const
	{
		return m_nParamNum;
	}

	void CDbPreCompileStatement::Execute()
	{
		if( GetParamNum() && m_bParamDirty && 
			mysql_stmt_bind_param( m_pStmt, m_aryParam ) )
		{
			ostringstream strm;
			strm << "mysql_stmt_bind_param failed with error:" << mysql_stmt_error( m_pStmt ) << ends;
			GammaThrow( strm.str() );
		}

		if( mysql_stmt_execute( m_pStmt ) )
		{
			char szErrBuff[65535];
			sprintf( szErrBuff, "mysql_stmt_execute failed with error:%s\n", mysql_stmt_error( m_pStmt ) );
			GammaThrow( szErrBuff );
		}

		if( mysql_stmt_store_result( m_pStmt ) )
		{
			char szErrBuff[65535];
			sprintf( szErrBuff, "mysql_stmt_store_result failed with error:%s\n", mysql_stmt_error( m_pStmt ) );
			GammaThrow( szErrBuff );
		}
	}

	uint32_t CDbPreCompileStatement::ExecuteWithoutException( void )
	{
		if( GetParamNum() && m_bParamDirty &&
			mysql_stmt_bind_param( m_pStmt, m_aryParam ) )
			return mysql_stmt_errno( m_pStmt );

		if( mysql_stmt_execute( m_pStmt ) )
			return mysql_stmt_errno( m_pStmt );

		if( mysql_stmt_store_result( m_pStmt ) )
			return mysql_stmt_errno( m_pStmt );

		return 0;
	}

	const char* CDbPreCompileStatement::GetExecuteError( void )
	{
		return mysql_stmt_error( m_pStmt );
	}

	uint64_t CDbPreCompileStatement::GetInsertID( void ) const
	{
		return (uint64_t)mysql_stmt_insert_id( m_pStmt );
	}

	uint32_t CDbPreCompileStatement::GetResultRowNum()const
	{
		return (uint32_t)mysql_stmt_num_rows( m_pStmt );
	}

	uint32_t CDbPreCompileStatement::GetAffectRowNum() const
	{
		return (uint32_t)mysql_stmt_affected_rows(m_pStmt);
	}

	void CDbPreCompileStatement::FetchResultRow( uint32_t nIndex )
	{
		if( nIndex >= GetResultRowNum() )
			GammaThrow( "uRowIndex >= GetResultRowNum()." );

		if( GetResultColNum() && m_bResultDirty &&
			mysql_stmt_bind_result( m_pStmt, m_aryResult ) )
		{
			ostringstream strm;
			strm << "mysql_stmt_bind_result failed with error:" << mysql_stmt_error( m_pStmt ) << ends;
			GammaThrow( strm.str() );
		}

		mysql_stmt_data_seek( m_pStmt, nIndex );

		int nReuslt = mysql_stmt_fetch( m_pStmt );
		if( nReuslt )
		{
			char szErrBuff[65535];
			if( nReuslt == 1 )
				sprintf( szErrBuff, "mysql_stmt_fetch failed with error(%d):%s\n",
					mysql_stmt_errno( m_pStmt ), mysql_stmt_error( m_pStmt ) );
			else if( nReuslt == MYSQL_NO_DATA )
				sprintf( szErrBuff, "mysql_stmt_fetch failed without data\n" );
			else if( nReuslt == MYSQL_DATA_TRUNCATED )
				sprintf( szErrBuff, "mysql_stmt_fetch failed with data truncate\n" );
			else
				sprintf( szErrBuff, "mysql_stmt_fetch failed with result(%d)\n", nReuslt );
			GammaThrow( szErrBuff );
		}
	}

	void CDbPreCompileStatement::SetParamNull( uint32_t nIndex )
	{
		BindNull( nIndex );
	}

	void CDbPreCompileStatement::SetParamInt8( const int8_t* pBuffer, uint32_t nIndex )
	{
		BindValue( pBuffer, sizeof( int8_t ), nIndex, MYSQL_TYPE_TINY, false );
	}

	void CDbPreCompileStatement::SetParamUint8( const uint8_t* pBuffer, uint32_t nIndex )
	{
		BindValue( pBuffer, sizeof( uint8_t ), nIndex, MYSQL_TYPE_TINY, true );
	}

	void CDbPreCompileStatement::SetParamInt16( const int16_t* pBuffer, uint32_t nIndex )
	{
		BindValue( pBuffer, sizeof( int16_t ), nIndex, MYSQL_TYPE_SHORT, false );
	}

	void CDbPreCompileStatement::SetParamUint16( const uint16_t* pBuffer, uint32_t nIndex )
	{
		BindValue( pBuffer, sizeof( uint16_t ), nIndex, MYSQL_TYPE_SHORT, true );
	}

	void CDbPreCompileStatement::SetParamInt32( const int32_t* pBuffer, uint32_t nIndex )
	{
		BindValue( pBuffer, sizeof( int32_t ), nIndex, MYSQL_TYPE_LONG, false );
	}

	void CDbPreCompileStatement::SetParamUint32( const uint32_t* pBuffer, uint32_t nIndex )
	{
		BindValue( pBuffer, sizeof( uint32_t ), nIndex, MYSQL_TYPE_LONG, true );
	}

	void CDbPreCompileStatement::SetParamInt64( const int64_t* pBuffer, uint32_t nIndex )
	{
		BindValue( pBuffer, sizeof( int64_t ), nIndex, MYSQL_TYPE_LONGLONG, false );
	}

	void CDbPreCompileStatement::SetParamUint64( const uint64_t* pBuffer, uint32_t nIndex )
	{
		BindValue( pBuffer, sizeof( uint64_t ), nIndex, MYSQL_TYPE_LONGLONG, true );
	}

	void CDbPreCompileStatement::SetParamFloat( const float* pBuffer, uint32_t nIndex )
	{
		BindValue( pBuffer, sizeof( float ), nIndex, MYSQL_TYPE_FLOAT, false );
	}

	void CDbPreCompileStatement::SetParamDouble( const double* pBuffer, uint32_t nIndex )
	{
		BindValue( pBuffer, sizeof( double ), nIndex, MYSQL_TYPE_DOUBLE, false );
	}

	void CDbPreCompileStatement::SetParamText( const void* pBuffer, uint32_t nMaxSize, ulong* pActualSize, uint32_t nIndex )
	{
		BindBuffer( pBuffer, nMaxSize, pActualSize, nIndex, MYSQL_TYPE_STRING );
	}

	void CDbPreCompileStatement::SetParamBinary( const void* pBuffer, uint32_t nMaxSize, ulong* pActualSize, uint32_t nIndex )
	{
		BindBuffer( pBuffer, nMaxSize, pActualSize, nIndex, MYSQL_TYPE_BLOB );
	}

	void CDbPreCompileStatement::SetParamDate( const void* pBuffer, uint32_t nIndex )
	{
		BindValue( pBuffer, sizeof( MYSQL_TIME ), nIndex, MYSQL_TYPE_DATE, false );
	}

	void CDbPreCompileStatement::SetParamDateTime( const void* pBuffer, uint32_t nIndex )
	{
		BindValue( pBuffer, sizeof( MYSQL_TIME ), nIndex, MYSQL_TYPE_DATETIME, false );
	}

	void CDbPreCompileStatement::SetParamTimeStamp( const void* pBuffer, uint32_t nIndex )
	{
		BindValue( pBuffer, sizeof( MYSQL_TIME ), nIndex, MYSQL_TYPE_TIMESTAMP, false );
	}

	void CDbPreCompileStatement::SetParamTime( const void* pBuffer, uint32_t nIndex )
	{
		BindValue( pBuffer, sizeof( MYSQL_TIME ), nIndex, MYSQL_TYPE_TIME, false );
	}

	void CDbPreCompileStatement::BindNull( uint32_t nIndex )
	{
		if( nIndex >= GetParamNum() )
			GammaThrow( "nIndex >= GetParamNum()" );

		MYSQL_BIND& ParamBind = m_aryParam[nIndex];
		ParamBind.is_null = (my_bool*)&m_bIsNull;
		ParamBind.buffer = NULL;
		ParamBind.buffer_type = MYSQL_TYPE_NULL;
		ParamBind.is_unsigned = false;
		m_bParamDirty = true;
	}

	void CDbPreCompileStatement::BindValue( const void* pBuffer, uint32_t nSize,
		uint32_t nIndex, enum_field_types eBufferType, bool bIsUnsigned )
	{
		if( nIndex >= GetParamNum() )
			GammaThrow( "nIndex >= GetParamNum()" );

		MYSQL_BIND& ParamBind = m_aryParam[nIndex];
		ParamBind.is_null = NULL;
		ParamBind.buffer = (void*)pBuffer;
		ParamBind.buffer_type = eBufferType;
		ParamBind.is_unsigned = bIsUnsigned;
		ParamBind.buffer_length = nSize;
		m_bParamDirty = true;
	}

	void CDbPreCompileStatement::BindBuffer( const void* pBuffer, uint32_t nMaxSize,
		ulong* pActualSize, uint32_t nIndex, enum_field_types eBufferType )
	{
		if( nIndex >= GetParamNum() )
			GammaThrow( "nIndex >= GetParamNum()" );
		if( !pActualSize )
			GammaThrow( "pActualSize can not be NULL!!" );

		MYSQL_BIND& ParamBind = m_aryParam[nIndex];
		ParamBind.is_null = NULL;
		ParamBind.buffer = (void*)pBuffer;
		ParamBind.buffer_type = eBufferType;
		ParamBind.buffer_length = nMaxSize;
		ParamBind.length = (ulong*)pActualSize;
		m_bParamDirty = true;
	}
}
