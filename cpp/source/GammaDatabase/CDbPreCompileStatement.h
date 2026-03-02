#pragma once
#include "CDbStatement.h"
#include <mysql/mysql.h>

namespace Gamma
{
    class CDbConnection;

    class CDbPreCompileStatement : public CDbStatement
    {
    public:
        CDbPreCompileStatement(CDbConnection* pConn,const char* szSqlBuffer, uint32 uSize);
        ~CDbPreCompileStatement(void);

        void			FetchResultRow( uint32 nIndex );

        uint32			GetParamNum() const;
        uint32			GetResultRowNum() const;
		uint64			GetInsertID( void ) const;
		uint32			GetAffectRowNum() const;

        void			Execute();
		uint32			ExecuteWithoutException( void );
		const char*		GetExecuteError( void );

		void			SetParamNull( uint32 nIndex );
		void			SetParamInt8( const int8* pBuffer, uint32 nIndex );
		void			SetParamUint8( const uint8* pBuffer, uint32 nIndex );
		void			SetParamInt16( const int16* pBuffer, uint32 nIndex );
		void			SetParamUint16( const uint16* pBuffer, uint32 nIndex );
		void			SetParamInt32( const int32* pBuffer, uint32 nIndex );
		void			SetParamUint32( const uint32* pBuffer, uint32 nIndex );
		void			SetParamInt64( const int64* pBuffer, uint32 nIndex );
		void			SetParamUint64( const uint64* pBuffer, uint32 nIndex );
		void			SetParamFloat( const float* pBuffer, uint32 nIndex );
		void			SetParamDouble( const double* pBuffer, uint32 nIndex );
		void			SetParamText( const void* pBuffer, uint32 nMaxSize, ulong* pActualSize, uint32 nIndex );
		void			SetParamBinary( const void* pBuffer, uint32 nMaxSize, ulong* pActualSize, uint32 nIndex );
		void			SetParamDate( const void* pBuffer, uint32 nIndex );
		void			SetParamDateTime( const void* pBuffer, uint32 nIndex );
		void			SetParamTimeStamp( const void* pBuffer, uint32 nIndex );
		void			SetParamTime( const void* pBuffer, uint32 nIndex );

    private:
        MYSQL_STMT*     m_pStmt;
		MYSQL_BIND*		m_aryParam;
        uint32          m_nParamNum;
        my_bool         m_bIsNull;

		void	        BindNull( uint32 nIndex );
        void			BindValue( const void* pBuffer, uint32 nSize, 
							uint32 nIndex, enum_field_types eBufferType, bool bIsUnsigned );
        void			BindBuffer( const void* pBuffer, uint32 nMaxSize, ulong* pActualSize,
							uint32 nIndex, enum_field_types eBufferType );
    };
}
