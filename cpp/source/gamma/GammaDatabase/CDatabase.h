#ifndef _CMYSQLDATABASE_H_
#define _CMYSQLDATABASE_H_

#include "GammaDatabase/IDatabase.h"

namespace Gamma
{
    class CDatabase
        :public IDatabase
    {
		EErrorCodes m_eErrorCode[eEC_ErrorCount];

    public:
		CDatabase();
		EErrorCodes FormatCode( uint32_t nErrorCode );

		IDbConnection* CreateConnection( const char* szHost, uint16_t uPort,
			const char* szUser, const char* szPassword, const char* szDatabase, uint32_t nDBId,
			bool bFoundAsUpdateRow, bool bMultiStatements, bool bPingReconnect );

		IDbConnection* CreateConnection( const char* szPipe,
			const char* szUser, const char* szPassword, const char* szDatabase, uint32_t nDBId,
			bool bFoundAsUpdateRow, bool bMultiStatements, bool bPingReconnect );
    };
}

#endif
