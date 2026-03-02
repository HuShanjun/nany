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
        uint32          m_nDBId;
		ILog*			m_pLog;
        CDbResult*		GetResult( int32 nQueryReturnCode );

    public:
		MYSQL*			m_pMySql;
		uint32			m_nTotalStatement;

		CDbConnection(uint32 nDBId);
		virtual ~CDbConnection();

        uint32			GetDBId() { return m_nDBId; }

		std::string		Connect( CDatabase* pDatabase, const char* szHost, uint16 uPort,
							const char* szUser, const char* szPassword, const char* szDatabase, 
							bool bFoundAsUpdateRow, bool bMultiStatements, bool bPingReconnect );
		std::string		Connect( CDatabase* pDatabase, const char* szPipe, 
							const char* szUser, const char* szPassword, const char* szDatabase, 
							bool bFoundAsUpdateRow, bool bMultiStatements, bool bPingReconnect );

        uint64          LastInsertId() const;
        uint64          LastAffectedRowNum() const;
		uint32			CheckMultiExecute() const;
        uint32          EscapeString( char* szOut, const char* szIn, uint32 uInSize );
        bool            Ping();

        IDbTextResult*  Execute( const char* szSqlBuffer );
        IDbTextResult*  Execute( const char* szSqlBuffer, uint32 uSize );
        IDbStatement*   CreateStatement( const char* szSqlBuffer, uint32 uSize );
		IDbStatement*   CreateStatement( const char* szSqlBuffer );
		IDbStatement*	CreateDynamicStatement( const char* szSqlBuffer, uint32 uSqlLength );
		IDbStatement*	CreateDynamicStatement( const char* szSqlBuffer );
		uint32			GetTotalStatement() const;

		IDbTextResult*  ExecuteProcedure(const char* szSqlProcedureBuffer, uint32 uSize1,
                            const char* szSqlResultBuffer, uint32 uSize2);

        
		void			SetLog(ILog* pLog );
		ILog*			GetLog() const;
        void            SetTag( void* pTag );
        void*           GetTag()const;
		void            Release();

		void			StartTran();
		void			EndTran();
		void			CancelTran();

		int32			GetFoundRow();
    };
}
#endif
