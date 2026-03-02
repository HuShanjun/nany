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

        uint32_t           GetRowNum();
        uint32_t           GetColNum();

        void             Locate( uint32_t nIndex );
        const char*      GetData( uint32_t uColIndex );
        uint32_t           GetDataLength( uint32_t uColIndex );
        void             Release();

        void             SetTag( void* pTag );
        void*            GetTag()const;
    };
}

#endif
