#ifndef _CMYSQLDBCONNECTION_H_
#define _CMYSQLDBCONNECTION_H_

#include <string>
#include "GammaDatabase/IDatabase.h"
#include <mysql/mysql.h>

namespace Gamma
{
    class CDatabase;
    class CDbResult;

    class CDbConnection : public IDbConnection
    {
        CDatabase*		m_pDatabase;
        void*			m_pTag;
        uint32_t          m_nDBId;
		ILog*			m_pLog;
        CDbResult*		GetResult( int32_t nQueryReturnCode );

    public:
		MYSQL*			m_pMySql;
		uint32_t			m_nTotalStatement;

		CDbConnection(uint32_t nDBId);
		virtual ~CDbConnection();

        uint32_t			GetDBId() { return m_nDBId; }

		std::string		Connect( CDatabase* pDatabase, const char* szHost, uint16_t uPort,
							const char* szUser, const char* szPassword, const char* szDatabase, 
							bool bFoundAsUpdateRow, bool bMultiStatements, bool bPingReconnect );
		std::string		Connect( CDatabase* pDatabase, const char* szPipe, 
							const char* szUser, const char* szPassword, const char* szDatabase, 
							bool bFoundAsUpdateRow, bool bMultiStatements, bool bPingReconnect );

        uint64_t          LastInsertId() const;
        uint64_t          LastAffectedRowNum() const;
		uint32_t			CheckMultiExecute() const;
        uint32_t          EscapeString( char* szOut, const char* szIn, uint32_t uInSize );
        bool            Ping();

        IDbTextResult*  Execute( const char* szSqlBuffer );
        IDbTextResult*  Execute( const char* szSqlBuffer, uint32_t uSize );
        IDbStatement*   CreateStatement( const char* szSqlBuffer, uint32_t uSize );
		IDbStatement*   CreateStatement( const char* szSqlBuffer );
		IDbStatement*	CreateDynamicStatement( const char* szSqlBuffer, uint32_t uSqlLength );
		IDbStatement*	CreateDynamicStatement( const char* szSqlBuffer );
		uint32_t			GetTotalStatement() const;

		IDbTextResult*  ExecuteProcedure(const char* szSqlProcedureBuffer, uint32_t uSize1,
                            const char* szSqlResultBuffer, uint32_t uSize2);

        
		void			SetLog(ILog* pLog );
		ILog*			GetLog() const;
        void            SetTag( void* pTag );
        void*           GetTag()const;
		void            Release();

		void			StartTran();
		void			EndTran();
		void			CancelTran();

		int32_t			GetFoundRow();
    };
}
#endif
