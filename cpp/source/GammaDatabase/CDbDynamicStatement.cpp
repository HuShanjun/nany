#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/TGammaStrStream.h"
#include "CDbDynamicStatement.h"
#include "CDbConnection.h"
#include <sstream>
#include <stdio.h>

using namespace std;

namespace Gamma
{
	CDbDynamicStatement::CDbDynamicStatement( 
		CDbConnection* pConn, const char* szSqlBuffer, uint32 uSize )
		: CDbStatement( pConn, szSqlBuffer, uSize )
		, m_nAffectRows( 0 )
	{
		MYSQL_STMT* pStmt = mysql_stmt_init( m_pConn->m_pMySql );

		if( !pStmt )
		{
			char szErrBuff[65535];
			sprintf( szErrBuff, "mysql_stmt_init failed with error:%s:%s\n",
				mysql_stmt_error( pStmt ), szSqlBuffer );
			GammaThrow( szErrBuff );
		}

		if( mysql_stmt_prepare( pStmt, szSqlBuffer, uSize ) )
		{
			char szErrBuff[65535];
			sprintf( szErrBuff, "mysql_stmt_prepare failed with error:%s:%s\n",
				mysql_stmt_error( pStmt ), szSqlBuffer );
			GammaThrow( szErrBuff );
		}

		m_listCommand = SeparateString( szSqlBuffer, '?' );
		m_listParams.resize( m_listCommand.size() - 1 );
		if( mysql_stmt_param_count( pStmt ) != m_listParams.size() )
		{
			GammaThrow( "invalid statement parameter count" );
		}

		auto pRes = mysql_stmt_result_metadata( pStmt );
		m_nResultColNum = pRes ? mysql_num_fields( pRes ) : 0;
		if( pRes )
			mysql_free_result( pRes );
		mysql_stmt_close( pStmt );

		if( GetResultColNum() )
		{
			m_aryResult = new MYSQL_BIND[GetResultColNum()];
			memset( m_aryResult, 0, sizeof( MYSQL_BIND ) * GetResultColNum() );
		}
	}

	CDbDynamicStatement::~CDbDynamicStatement( void )
	{
	}

	uint32 CDbDynamicStatement::GetParamNum()const
	{
		return (uint32)m_listParams.size();
	}

	void CDbDynamicStatement::BuildCommand()
	{
		if( !m_bParamDirty )
			return;

		m_strCommand.clear();
		m_strCommand = m_listCommand[0];
		uint32 nIndex = 0;
		while( nIndex < m_listParams.size() )
		{
			m_strCommand.append( m_listParams[nIndex++] );
			m_strCommand.append( m_listCommand[nIndex] );
		}
	}

	void CDbDynamicStatement::Execute()
	{
		BuildCommand();
		int32 nError = mysql_real_query( m_pConn->m_pMySql,
			m_strCommand.c_str(), (ulong)m_strCommand.size() );

		if( m_pRes )
			mysql_free_result( m_pRes );
		m_pRes = nullptr;

		if( nError )
		{
			ostringstream strm;
			strm << "mysql_real_query failed with error:" 
				<< mysql_error( m_pConn->m_pMySql ) << ends;
			GammaThrow( strm.str() );
		}

		if( mysql_field_count( m_pConn->m_pMySql ) > 0 )
		{
			m_pRes = mysql_store_result( m_pConn->m_pMySql );

			if( !m_pRes )
			{
				ostringstream strm;
				strm << "mysql_store_result failed with error:" 
					<< mysql_error( m_pConn->m_pMySql ) << ends;
				GammaThrow( strm.str() );
			}
		}

		m_nAffectRows = (uint32)mysql_affected_rows( m_pConn->m_pMySql );
	}

	uint32 CDbDynamicStatement::ExecuteWithoutException( void )
	{
		BuildCommand();
		int32 nError = mysql_real_query( m_pConn->m_pMySql,
			m_strCommand.c_str(), (ulong)m_strCommand.size() );

		if( m_pRes )
			mysql_free_result( m_pRes );
		m_pRes = nullptr;

		if( nError )
		{
			m_strError = mysql_error( m_pConn->m_pMySql );
			return mysql_errno( m_pConn->m_pMySql );
		}

		m_nAffectRows = (uint32)mysql_affected_rows( m_pConn->m_pMySql );
		if( mysql_field_count( m_pConn->m_pMySql ) == 0 )
			return 0;
		m_pRes = mysql_store_result( m_pConn->m_pMySql );
		if( m_pRes )
			return 0;
		m_strError = mysql_error( m_pConn->m_pMySql );
		return mysql_errno( m_pConn->m_pMySql );
	}

	const char* CDbDynamicStatement::GetExecuteError( void )
	{
		return m_strError.c_str();
	}

	uint64 CDbDynamicStatement::GetInsertID( void ) const
	{
		return m_pConn->LastInsertId();
	}

	uint32 CDbDynamicStatement::GetResultRowNum()const
	{
		return m_pRes ? (uint32)mysql_num_rows(m_pRes) : 0;
	}

	uint32 CDbDynamicStatement::GetAffectRowNum() const
	{
		return m_nAffectRows;
	}

	void CDbDynamicStatement::FetchResultRow( uint32 nIndex )
	{
		GammaAst( nIndex < GetResultRowNum() );
		mysql_data_seek( m_pRes, nIndex );
		auto aryRow = mysql_fetch_row( m_pRes );
		auto aryLength = mysql_fetch_lengths( m_pRes );
		for( uint32 i = 0; i < GetResultColNum(); i++ )
		{
			if( aryRow[i] == nullptr )
			{
				if( m_aryResult[i].is_null )
					*m_aryResult[i].is_null = true;
				continue;
			}

			enum_field_types eType = m_aryResult[i].buffer_type;
			switch( eType )
			{
			case MYSQL_TYPE_TINY:
				if( m_aryResult[i].is_unsigned )
					*(uint8*)m_aryResult[i].buffer = (uint8)GammaA2I( aryRow[i] );
				else
					*(int8*)m_aryResult[i].buffer = (int8)GammaA2I( aryRow[i] );
				break;
			case MYSQL_TYPE_SHORT:
				if( m_aryResult[i].is_unsigned )
					*(uint16*)m_aryResult[i].buffer = (uint16)GammaA2I( aryRow[i] );
				else
					*(int16*)m_aryResult[i].buffer = (int16)GammaA2I( aryRow[i] );
				break;
			case MYSQL_TYPE_LONG:
				if( m_aryResult[i].is_unsigned )
					*(uint32*)m_aryResult[i].buffer = (uint32)GammaA2I64( aryRow[i] );
				else
					*(int32*)m_aryResult[i].buffer = (int32)GammaA2I( aryRow[i] );
				break;
			case MYSQL_TYPE_LONGLONG:
				if( m_aryResult[i].is_unsigned )
					*(uint64*)m_aryResult[i].buffer = (uint64)GammaA2I64( aryRow[i] );
				else
					*(int64*)m_aryResult[i].buffer = (int64)GammaA2I64( aryRow[i] );
				break;
			case MYSQL_TYPE_FLOAT:
				*(float*)m_aryResult[i].buffer = (float)GammaA2F( aryRow[i] );
				break;
			case MYSQL_TYPE_DOUBLE:
				*(double*)m_aryResult[i].buffer = GammaA2F( aryRow[i] );
				break;
			case MYSQL_TYPE_BLOB:
			case MYSQL_TYPE_STRING:
				*m_aryResult[i].length = m_aryResult[i].buffer_length;
				if( aryLength[i] < *m_aryResult[i].length )
					*m_aryResult[i].length = aryLength[i];
				memcpy( m_aryResult[i].buffer, aryRow[i], *m_aryResult[i].length );
				if( *m_aryResult[i].length < m_aryResult[i].buffer_length )
					memset( (tbyte*)( m_aryResult[i].buffer ) + *m_aryResult[i].length, 
						0, m_aryResult[i].buffer_length - *m_aryResult[i].length );
				break;
			default:
				break;
			}
		}
	}

	template<typename ValueType>
	void CDbDynamicStatement::SetParamValue( ValueType nValue, uint32 nIndex )
	{
		if( nIndex >= GetParamNum() )
			GammaThrow( "nIndex >= GetParamNum()" );
		m_listParams[nIndex].clear();
		gammasstream( m_listParams[nIndex] ) << nValue;
		m_bParamDirty = true;
	}

	void CDbDynamicStatement::SetParamNull( uint32 nIndex )
	{
		if( nIndex >= GetParamNum() )
			GammaThrow( "nIndex >= GetParamNum()" );
		m_listParams[nIndex] = "null";
		m_bParamDirty = true;
	}

	void CDbDynamicStatement::SetParamInt8( const int8* pBuffer, uint32 nIndex )
	{
		SetParamValue( (int32)*pBuffer, nIndex );
	}

	void CDbDynamicStatement::SetParamUint8( const uint8* pBuffer, uint32 nIndex )
	{
		SetParamValue( (int32)*pBuffer, nIndex );
	}

	void CDbDynamicStatement::SetParamInt16( const int16* pBuffer, uint32 nIndex )
	{
		SetParamValue( (int32)*pBuffer, nIndex );
	}

	void CDbDynamicStatement::SetParamUint16( const uint16* pBuffer, uint32 nIndex )
	{
		SetParamValue( (int32)*pBuffer, nIndex );
	}

	void CDbDynamicStatement::SetParamInt32( const int32* pBuffer, uint32 nIndex )
	{
		SetParamValue( *pBuffer, nIndex );
	}

	void CDbDynamicStatement::SetParamUint32( const uint32* pBuffer, uint32 nIndex )
	{
		SetParamValue( *pBuffer, nIndex );
	}

	void CDbDynamicStatement::SetParamInt64( const int64* pBuffer, uint32 nIndex )
	{
		SetParamValue( *pBuffer, nIndex );
	}

	void CDbDynamicStatement::SetParamUint64( const uint64* pBuffer, uint32 nIndex )
	{
		SetParamValue( *pBuffer, nIndex );
	}

	void CDbDynamicStatement::SetParamFloat( const float* pBuffer, uint32 nIndex )
	{
		SetParamValue( *pBuffer, nIndex );
	}

	void CDbDynamicStatement::SetParamDouble( const double* pBuffer, uint32 nIndex )
	{
		SetParamValue( *pBuffer, nIndex );
	}

	void CDbDynamicStatement::SetParamText( const void* pBuffer, uint32 nMaxSize, ulong* pActualSize, uint32 nIndex )
	{
		if( nIndex >= GetParamNum() )
			GammaThrow( "nIndex >= GetParamNum()" );

		m_bParamDirty = true;
		if( *pActualSize == 0 )
		{
			m_listParams[nIndex] = "\'\'";
			return;
		}

		auto& strBinary = m_listParams[nIndex];
		strBinary.resize( 2 + *pActualSize*2 );
		strBinary[0] = '0';
		strBinary[1] = 'x';
		const tbyte* aryData = (const tbyte*)pBuffer;
		for( size_t i = 0, n = 2; i < *pActualSize; i++ )
		{
			strBinary[n++] = ValueToHexNumber( aryData[i] / 16 );
			strBinary[n++] = ValueToHexNumber( aryData[i] % 16 );
		}
		m_bParamDirty = true;
	}

	void CDbDynamicStatement::SetParamBinary( const void* pBuffer, uint32 nMaxSize, ulong* pActualSize, uint32 nIndex )
	{
		SetParamText( pBuffer, nMaxSize, pActualSize, nIndex );
		m_bParamDirty = true;
	}

	void CDbDynamicStatement::SetParamDate( const void* pBuffer, uint32 nIndex )
	{
		if( nIndex >= GetParamNum() )
			GammaThrow( "nIndex >= GetParamNum()" );
		MYSQL_TIME Time = *(const MYSQL_TIME*)pBuffer;
		m_listParams[nIndex].clear();
		gammasstream( m_listParams[nIndex] ) << '\'' 
			<< Time.year << '-' << Time.month << '-' << Time.day << '\'';
		m_bParamDirty = true;
	}

	void CDbDynamicStatement::SetParamDateTime( const void* pBuffer, uint32 nIndex )
	{
		if( nIndex >= GetParamNum() )
			GammaThrow( "nIndex >= GetParamNum()" );
		MYSQL_TIME Time = *(const MYSQL_TIME*)pBuffer;
		m_listParams[nIndex].clear();
		gammasstream( m_listParams[nIndex] ) << '\''
			<< Time.year << '-' << Time.month << '-' << Time.day << ' '
			<< Time.hour << ':' << Time.minute << ':' << Time.second << '\'';
		m_bParamDirty = true;
	}

	void CDbDynamicStatement::SetParamTimeStamp( const void* pBuffer, uint32 nIndex )
	{
		if( nIndex >= GetParamNum() )
			GammaThrow( "nIndex >= GetParamNum()" );
		MYSQL_TIME Time = *(const MYSQL_TIME*)pBuffer;
		m_listParams[nIndex].clear();
		gammasstream( m_listParams[nIndex] ) << '\''
			<< Time.year << '-' << Time.month << '-' << Time.day << ' '
			<< Time.hour << ':' << Time.minute << ':' << Time.second << '\'';
		m_bParamDirty = true;
	}

	void CDbDynamicStatement::SetParamTime( const void* pBuffer, uint32 nIndex )
	{
		if( nIndex >= GetParamNum() )
			GammaThrow( "nIndex >= GetParamNum()" );
		MYSQL_TIME Time = *(const MYSQL_TIME*)pBuffer;
		m_listParams[nIndex].clear();
		gammasstream( m_listParams[nIndex] ) << '\''
			<< Time.year << '-' << Time.month << '-' << Time.day << ' '
			<< Time.hour << ':' << Time.minute << ':' << Time.second << '\'';
		m_bParamDirty = true;
	}
}
