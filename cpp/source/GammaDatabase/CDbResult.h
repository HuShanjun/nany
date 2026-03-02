#ifndef _CMYSQLDBRESULT_H_
#define _CMYSQLDBRESULT_H_

#include "GammaDatabase/IDatabase.h"
#include <mysql/mysql.h>

namespace Gamma
{
    class CDbResult : public IDbTextResult
    {
    private:
        MYSQL_RES*       m_pRes;
        MYSQL_ROW        m_Row;
        ulong*           m_Lengths;
        void*            m_pTag;

    public:
        CDbResult( MYSQL_RES* pRes );
        virtual ~CDbResult();

        uint32           GetRowNum();
        uint32           GetColNum();

        void             Locate( uint32 nIndex );
        const char*      GetData( uint32 uColIndex );
        uint32           GetDataLength( uint32 uColIndex );
        void             Release();

        void             SetTag( void* pTag );
        void*            GetTag()const;
    };
}

#endif
