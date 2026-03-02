
#include "CDbConnection.h"
#include "CDbResult.h"
#include "CDbStatement.h"
#include "CDbDynamicStatement.h"
#include "CDbPreCompileStatement.h"
#include "GammaCommon/GammaHelp.h"
#include <sstream>

using namespace std;

namespace Gamma
{
	CDbConnection::CDbConnection( uint32 nDBId )
		: m_pDatabase( NULL )
		, m_pTag( NULL )
		, m_nTotalStatement( 0 )
		, m_nDBId( nDBId )
	{
	}

	CDbConnection:: ~CDbConnection()
	{
		if( !m_pMySql )
			return;
		mysql_close( m_pMySql );
	}

	std::string CDbConnection::Connect(
		CDatabase* pDatabase,
		const char* szHost,
		uint16 uPort,
		const char* szUser,
		const char* szPassword,
		const char* szDatabase,
		bool bFoundAsUpdateRow,
		bool bMultiStatements,
		bool bPingReconnect )
	{
		m_pDatabase = pDatabase;
		uint32 nFlag = bFoundAsUpdateRow ? CLIENT_FOUND_ROWS : 0;
		nFlag = nFlag | (bMultiStatements ? CLIENT_MULTI_STATEMENTS : 0);
		my_bool op = true;

		if( (m_pMySql = mysql_init( NULL )) == NULL )
		{
			ostringstream strm;
			strm << "mysql_init failed with error:Can not create Mysql." << ends;
			return strm.str();
		}

		if( bPingReconnect && mysql_options( m_pMySql, MYSQL_OPT_RECONNECT, &op ) != 0 )
		{
			ostringstream strm;
			strm << "mysql_init failed with error:" << mysql_error( m_pMySql ) << ends;
			return strm.str();
		}

		//mysql 的文档指出，mysql_real_connect的返回值和他第一个参数相等时调用成功
		if( m_pMySql != mysql_real_connect( m_pMySql, szHost,
			szUser, szPassword, szDatabase, uPort, NULL, nFlag ) )
		{
			ostringstream strm;
			strm << "mysql_init failed with error:" << mysql_error( m_pMySql ) << ends;
			return strm.str();
		}

		return string();
	}

	std::string CDbConnection::Connect(
		CDatabase* pDatabase,
		const char* szPipe,
		const char* szUser,
		const char* szPassword,
		const char* szDatabase,
		bool bFoundAsUpdateRow,
		bool bMultiStatements,
		bool bPingReconnect )
	{
		m_pDatabase = pDatabase;
		uint32 nFlag = bFoundAsUpdateRow ? CLIENT_FOUND_ROWS : 0;
		nFlag = nFlag | (bMultiStatements ? CLIENT_MULTI_STATEMENTS : 0);
		my_bool op = true;

		if( (m_pMySql = mysql_init( NULL )) == NULL )
		{
			ostringstream strm;
			strm << "mysql_init failed with error:Can not create Mysql." << ends;
			return strm.str();
		}

		if( bPingReconnect && mysql_options( m_pMySql, MYSQL_OPT_RECONNECT, &op ) != 0 )
		{
			ostringstream strm;
			strm << "mysql_init failed with error:" << mysql_error( m_pMySql ) << ends;
			return strm.str();
		}

		//mysql 的文档指出，mysql_real_connect的返回值和他第一个参数相等时调用成功
		if( m_pMySql != mysql_real_connect( m_pMySql, NULL,
			szUser, szPassword, szDatabase, 0, szPipe, nFlag ) )
		{
			ostringstream strm;
			strm << "mysql_init failed with error:" << mysql_error( m_pMySql ) << ends;
			return strm.str();
		}

		return string();
	}

	uint64 CDbConnection::LastInsertId() const
	{
		return mysql_insert_id( m_pMySql );
	}

	uint64 CDbConnection::LastAffectedRowNum() const
	{
		return mysql_affected_rows( m_pMySql );
	}

	uint32 CDbConnection::CheckMultiExecute() const
	{
		do
		{
			MYSQL_RES* result = mysql_store_result( m_pMySql );
			mysql_free_result( result );
		} while( mysql_next_result( m_pMySql ) == 0 );

		if( mysql_errno( m_pMySql ) != 0 )
		{
			ostringstream strm;
			strm << "mysql_real_query(multi query) failed with error:" << mysql_error( m_pMySql ) << ends;
			GammaThrow( strm.str() );
		}
		return 0;
	}

	uint32 CDbConnection::EscapeString( char* szOut, const char* szIn, uint32 uInSize )
	{
		return mysql_real_escape_string( m_pMySql, szOut, szIn, uInSize );
	}

	bool CDbConnection::Ping()
	{
		return 0 == mysql_ping( m_pMySql );
	}

	CDbResult* CDbConnection::GetResult( int nQueryReturnCode )
	{
		if( nQueryReturnCode )
		{
			ostringstream strm;
			strm << "mysql_real_query failed with error:" << mysql_error( m_pMySql ) << ends;
			GammaThrow( strm.str() );
		}

		if( mysql_field_count( m_pMySql ) > 0 )
		{
			MYSQL_RES* pRes;

			pRes = mysql_store_result( m_pMySql );

			if( !pRes )
			{
				ostringstream strm;
				strm << "mysql_store_result failed with error:" << mysql_error( m_pMySql ) << ends;
				GammaThrow( strm.str() );
			}
			return new CDbResult( pRes );
		}
		return NULL;
	}

	IDbTextResult* CDbConnection::Execute( const char* szSqlBuffer )
	{
		return GetResult( mysql_query( m_pMySql, szSqlBuffer ) );
	}

	IDbTextResult* CDbConnection::Execute( const char* szSqlBuffer, uint32 uSize )
	{
		return GetResult( mysql_real_query( m_pMySql, szSqlBuffer, (unsigned long)uSize ) );
	}

	Gamma::IDbTextResult* CDbConnection::ExecuteProcedure( const char* szSqlProcedureBuffer,
		uint32 uSize1, const char* szSqlResultBuffer, uint32 uSize2 )
	{
		int nQueryReturnCode = mysql_real_query( m_pMySql, szSqlProcedureBuffer, uSize1 );
		if( 0 != nQueryReturnCode )
			return NULL;
		return Execute( szSqlResultBuffer, uSize2 );
	}

	void CDbConnection::SetLog(ILog* pLog)
	{
		m_pLog = pLog;
	}

	Gamma::ILog* CDbConnection::GetLog() const
	{
		return m_pLog;
	}

	IDbStatement* CDbConnection::CreateStatement(const char* szSqlBuffer, uint32 uSize)
	{
		return new CDbPreCompileStatement( this, szSqlBuffer, uSize );
	}

	IDbStatement* CDbConnection::CreateStatement( const char* szSqlBuffer )
	{
		return CreateStatement( szSqlBuffer, (uint32)strlen( szSqlBuffer ) );
	}

	IDbStatement* CDbConnection::CreateDynamicStatement( const char* szSqlBuffer, uint32 uSize )
	{
		return new CDbDynamicStatement( this, szSqlBuffer, uSize );
	}

	IDbStatement* CDbConnection::CreateDynamicStatement( const char* szSqlBuffer )
	{
		return CreateDynamicStatement( szSqlBuffer, (uint32)strlen( szSqlBuffer ) );
	}

	uint32 CDbConnection::GetTotalStatement() const
	{
		return m_nTotalStatement;
	}

	void CDbConnection::Release()
	{
		delete this;
	}

	void CDbConnection::SetTag( void* pTag )
	{
		m_pTag = pTag;
	}

	void* CDbConnection::GetTag()const
	{
		return m_pTag;
	}

	void CDbConnection::StartTran()
	{
		static const char* szSql = "start transaction";
		static size_t nStrSize = strlen( szSql );
		Execute( szSql, (uint32)nStrSize );
	}

	void CDbConnection::EndTran()
	{
		static const char* szSql = "commit";
		static size_t nStrSize = strlen( szSql );
		Execute( szSql, (uint32)nStrSize );
	}

	void CDbConnection::CancelTran()
	{
		static const char* szSql = "rollback";
		static size_t nStrSize = strlen( szSql );
		Execute( szSql, (uint32)nStrSize );
	}

	int32 CDbConnection::GetFoundRow()
	{
		static const char* szSql = "select FOUND_ROWS()";
		static size_t nStrSize = strlen(szSql);
		IDbTextResult* pResult = Execute(szSql, (uint32)nStrSize);
		if (!pResult)
			return 0;
		uint32 nRow = pResult->GetRowNum();
		if (nRow == 0)
			return 0;
		pResult->Locate(0);
		const char* ret = pResult->GetData(0);
		if (!ret)
			return 0;
		return GammaA2I(ret);
	}
}
