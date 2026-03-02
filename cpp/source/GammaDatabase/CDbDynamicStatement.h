#pragma once
#include "CDbStatement.h"
#include <mysql/mysql.h>
#include <vector>
#include <string>

namespace Gamma
{
    class CDbConnection;
    typedef std::vector<std::string> CSQLCommand;

    class CDbDynamicStatement : public CDbStatement
    {
    public:
        CDbDynamicStatement(CDbConnection* pConn,const char* szSqlBuffer, uint32 uSize);
        ~CDbDynamicStatement(void);

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
        CSQLCommand     m_listCommand;
        CSQLCommand     m_listParams;
		std::string		m_strCommand;
		std::string		m_strError;
		uint32			m_nAffectRows;

		template<typename ValueType>
		void			SetParamValue( ValueType nValue, uint32 nIndex );
		void			BuildCommand();
    };
}
