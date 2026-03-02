
#include "CDatabase.h"
#include "CDbConnection.h"
#include <mysql/mysql.h>
#include <mysql/mysqld_error.h>

namespace Gamma
{
	CDatabase::CDatabase()
	{
		uint32 nCount = eEC_ErrorCount;
		while( nCount -- )
		{
			m_eErrorCode[ nCount ] = eEC_Unkown;
		}

		m_eErrorCode[ER_DUP_ENTRY]		= eEC_DupEntry;
		m_eErrorCode[ER_LOCK_DEADLOCK]	= eEC_DeadLock;
		m_eErrorCode[0]					= eEC_Succeed;
	}

	EErrorCodes CDatabase::FormatCode( uint32 nErrorCode )
	{
		return nErrorCode < (uint32)eEC_ErrorCount ? m_eErrorCode[nErrorCode] : eEC_Unkown;
	}

    IDatabase* GetDatabase()
    {
		static CDatabase s_Instance;
        return &s_Instance;
    }

    IDbConnection* CDatabase::CreateConnection( const char* szHost, uint16 uPort,
        const char* szUser, const char* szPassword, const char* szDatabase, uint32 nDBId,
        bool bFoundAsUpdateRow, bool bMultiStatements, bool bPingReconnect )
    {
        CDbConnection* pConnect = new CDbConnection(nDBId);
		std::string strError = pConnect->Connect( this, szHost, uPort, szUser, szPassword,
			szDatabase, bFoundAsUpdateRow, bMultiStatements, bPingReconnect );
		if( strError.empty() )
			return pConnect;
		delete pConnect;
		throw strError;
    }

    IDbConnection* CDatabase::CreateConnection( const char* szPipe,
        const char* szUser, const char* szPassword, const char* szDatabase, uint32 nDBId,
        bool bFoundAsUpdateRow, bool bMultiStatements, bool bPingReconnect )
    {
		CDbConnection* pConnect = new CDbConnection(nDBId);
		std::string strError = pConnect->Connect( this, szPipe, szUser, szPassword, 
			szDatabase, bFoundAsUpdateRow, bMultiStatements, bPingReconnect );
		if( strError.empty() )
			return pConnect;
		delete pConnect;
		throw strError;
    }
}
