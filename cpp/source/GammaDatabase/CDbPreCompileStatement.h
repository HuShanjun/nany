#pragma once
#include "CDbStatement.h"
#include <mysql/mysql.h>

namespace Gamma
{
    class CDbConnection;

    class CDbPreCompileStatement : public CDbStatement
    {
    public:
        CDbPreCompileStatement(CDbConnection* pConn,const char* szSqlBuffer, uint32_t uSize);
        ~CDbPreCompileStatement(void);

        void			FetchResultRow( uint32_t nIndex );

        uint32_t			GetParamNum() const;
        uint32_t			GetResultRowNum() const;
		uint64_t			GetInsertID( void ) const;
		uint32_t			GetAffectRowNum() const;

        void			Execute();
		uint32_t			ExecuteWithoutException( void );
		const char*		GetExecuteError( void );

		void			SetParamNull( uint32_t nIndex );
		void			SetParamInt8( const int8_t* pBuffer, uint32_t nIndex );
		void			SetParamUint8( const uint8_t* pBuffer, uint32_t nIndex );
		void			SetParamInt16( const int16_t* pBuffer, uint32_t nIndex );
		void			SetParamUint16( const uint16_t* pBuffer, uint32_t nIndex );
		void			SetParamInt32( const int32_t* pBuffer, uint32_t nIndex );
		void			SetParamUint32( const uint32_t* pBuffer, uint32_t nIndex );
		void			SetParamInt64( const int64_t* pBuffer, uint32_t nIndex );
		void			SetParamUint64( const uint64_t* pBuffer, uint32_t nIndex );
		void			SetParamFloat( const float* pBuffer, uint32_t nIndex );
		void			SetParamDouble( const double* pBuffer, uint32_t nIndex );
		void			SetParamText( const void* pBuffer, uint32_t nMaxSize, ulong* pActualSize, uint32_t nIndex );
		void			SetParamBinary( const void* pBuffer, uint32_t nMaxSize, ulong* pActualSize, uint32_t nIndex );
		void			SetParamDate( const void* pBuffer, uint32_t nIndex );
		void			SetParamDateTime( const void* pBuffer, uint32_t nIndex );
		void			SetParamTimeStamp( const void* pBuffer, uint32_t nIndex );
		void			SetParamTime( const void* pBuffer, uint32_t nIndex );

    private:
        MYSQL_STMT*     m_pStmt;
		MYSQL_BIND*		m_aryParam;
        uint32_t          m_nParamNum;
        my_bool         m_bIsNull;

		void	        BindNull( uint32_t nIndex );
        void			BindValue( const void* pBuffer, uint32_t nSize, 
							uint32_t nIndex, enum_field_types eBufferType, bool bIsUnsigned );
        void			BindBuffer( const void* pBuffer, uint32_t nMaxSize, ulong* pActualSize,
							uint32_t nIndex, enum_field_types eBufferType );
    };
}
