#pragma once
#include "GammaDatabase/IDatabase.h"
#include <mysql/mysql.h>

namespace Gamma
{
    class CDbConnection;

    class CDbStatement : public IDbStatement
    {
    public:
        CDbStatement(CDbConnection* pConn,const char* szSqlBuffer, uint32_t uSize);
        ~CDbStatement(void);

        IDbConnection*	GetConnection() const;
        uint32_t			GetResultColNum() const;
        void			Release();

        void			SetTag( void* pTag );
        void*			GetTag() const;

		void			SetResultInt8( int8_t* pBuffer, bool* pIsNull, bool* pError, uint32_t uColIndex );
		void			SetResultUint8( uint8_t* pBuffer, bool* pIsNull, bool* pError, uint32_t uColIndex );
		void			SetResultInt16( int16_t* pBuffer, bool* pIsNull, bool* pError, uint32_t uColIndex );
		void			SetResultUint16( uint16_t* pBuffer, bool* pIsNull, bool* pError, uint32_t uColIndex );
		void			SetResultInt32( int32_t* pBuffer, bool* pIsNull, bool* pError, uint32_t uColIndex );
		void			SetResultUint32( uint32_t* pBuffer, bool* pIsNull, bool* pError, uint32_t uColIndex );
		void			SetResultInt64( int64_t* pBuffer, bool* pIsNull, bool* pError, uint32_t uColIndex );
		void			SetResultUint64( uint64_t* pBuffer, bool* pIsNull, bool* pError, uint32_t uColIndex );
		void			SetResultFloat( float* pBuffer, bool* pIsNull, bool* pError, uint32_t uColIndex );
		void			SetResultDouble( double* pBuffer, bool* pIsNull, bool* pError, uint32_t uColIndex );
		void			SetResultText( void* pBuffer, uint32_t uBufferSize, ulong* uDataSize, bool* pIsNull, bool* pError, uint32_t uColIndex );
		void			SetResultBinary( void* pBuffer, uint32_t uBufferSize, ulong* uDataSize, bool* pIsNull, bool* pError, uint32_t uColIndex );

	protected:
        CDbConnection*  m_pConn;
        void*           m_pTag;
		MYSQL_RES*      m_pRes;
        MYSQL_BIND*     m_aryResult;
		bool			m_bParamDirty;
		bool			m_bResultDirty;
		uint32_t          m_nResultColNum;

        virtual void	SetResultIntegerSeries( void* pBuffer, bool* pIsNull, bool* pError, uint32_t uColIndex,
							enum_field_types eBufferType, bool bIsUnsigned );
        virtual void	SetResultRealSeries( void* pBuffer, bool* pIsNull, bool* pError, uint32_t uColIndex,
							enum_field_types eBufferType );
        virtual void	SetResultBufferSeries( void* pBuffer, uint32_t uBufferSize, ulong* uDataSize, bool* pIsNull, bool* pError, uint32_t uColIndex,
							enum_field_types eBufferType );
    };
}
