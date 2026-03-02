#pragma once
#include "GammaDatabase/IDatabase.h"
#include <mysql/mysql.h>

namespace Gamma
{
    class CDbConnection;

    class CDbStatement : public IDbStatement
    {
    public:
        CDbStatement(CDbConnection* pConn,const char* szSqlBuffer, uint32 uSize);
        ~CDbStatement(void);

        IDbConnection*	GetConnection() const;
        uint32			GetResultColNum() const;
        void			Release();

        void			SetTag( void* pTag );
        void*			GetTag() const;

		void			SetResultInt8( int8* pBuffer, bool* pIsNull, bool* pError, uint32 uColIndex );
		void			SetResultUint8( uint8* pBuffer, bool* pIsNull, bool* pError, uint32 uColIndex );
		void			SetResultInt16( int16* pBuffer, bool* pIsNull, bool* pError, uint32 uColIndex );
		void			SetResultUint16( uint16* pBuffer, bool* pIsNull, bool* pError, uint32 uColIndex );
		void			SetResultInt32( int32* pBuffer, bool* pIsNull, bool* pError, uint32 uColIndex );
		void			SetResultUint32( uint32* pBuffer, bool* pIsNull, bool* pError, uint32 uColIndex );
		void			SetResultInt64( int64* pBuffer, bool* pIsNull, bool* pError, uint32 uColIndex );
		void			SetResultUint64( uint64* pBuffer, bool* pIsNull, bool* pError, uint32 uColIndex );
		void			SetResultFloat( float* pBuffer, bool* pIsNull, bool* pError, uint32 uColIndex );
		void			SetResultDouble( double* pBuffer, bool* pIsNull, bool* pError, uint32 uColIndex );
		void			SetResultText( void* pBuffer, uint32 uBufferSize, ulong* uDataSize, bool* pIsNull, bool* pError, uint32 uColIndex );
		void			SetResultBinary( void* pBuffer, uint32 uBufferSize, ulong* uDataSize, bool* pIsNull, bool* pError, uint32 uColIndex );

	protected:
        CDbConnection*  m_pConn;
        void*           m_pTag;
		MYSQL_RES*      m_pRes;
        MYSQL_BIND*     m_aryResult;
		bool			m_bParamDirty;
		bool			m_bResultDirty;
		uint32          m_nResultColNum;

        virtual void	SetResultIntegerSeries( void* pBuffer, bool* pIsNull, bool* pError, uint32 uColIndex,
							enum_field_types eBufferType, bool bIsUnsigned );
        virtual void	SetResultRealSeries( void* pBuffer, bool* pIsNull, bool* pError, uint32 uColIndex,
							enum_field_types eBufferType );
        virtual void	SetResultBufferSeries( void* pBuffer, uint32 uBufferSize, ulong* uDataSize, bool* pIsNull, bool* pError, uint32 uColIndex,
							enum_field_types eBufferType );
    };
}
