#include "GammaCommon/GammaHelp.h"
#include "CDbStatement.h"
#include "CDbConnection.h"
#include <sstream>
#include <stdio.h>

using namespace std;

namespace Gamma
{
	CDbStatement::CDbStatement( CDbConnection* pConn, const char* szSqlBuffer, uint32 uSize )
		: m_pConn( pConn )
		, m_pTag( nullptr )
		, m_pRes( nullptr )
		, m_aryResult( nullptr )
		, m_bParamDirty( true )
		, m_bResultDirty( true )
		, m_nResultColNum( 0 )
	{
	}

	CDbStatement::~CDbStatement( void )
	{
		if( m_pRes )
			mysql_free_result( m_pRes );
		delete[] m_aryResult;
	}

	IDbConnection* CDbStatement::GetConnection()const
	{
		return m_pConn;
	}

	void CDbStatement::Release()
	{
		delete this;
	}

	void CDbStatement::SetTag( void* pTag )
	{
		m_pTag = pTag;
	}

	void* CDbStatement::GetTag()const
	{
		return m_pTag;
	}

	uint32 CDbStatement::GetResultColNum() const
	{
		return m_nResultColNum;
	}

	void CDbStatement::SetResultInt8( int8* pBuffer, bool* pIsNull, bool* pError, uint32 uColIndex )
	{
		SetResultIntegerSeries( static_cast<void*>(pBuffer), pIsNull, pError, uColIndex, MYSQL_TYPE_TINY, false );
	}

	void CDbStatement::SetResultUint8( uint8* pBuffer, bool* pIsNull, bool* pError, uint32 uColIndex )
	{
		SetResultIntegerSeries( static_cast<void*>(pBuffer), pIsNull, pError, uColIndex, MYSQL_TYPE_TINY, true );
	}

	void CDbStatement::SetResultInt16( int16* pBuffer, bool* pIsNull, bool* pError, uint32 uColIndex )
	{
		SetResultIntegerSeries( static_cast<void*>(pBuffer), pIsNull, pError, uColIndex, MYSQL_TYPE_SHORT, false );
	}

	void CDbStatement::SetResultUint16( uint16* pBuffer, bool* pIsNull, bool* pError, uint32 uColIndex )
	{
		SetResultIntegerSeries( static_cast<void*>(pBuffer), pIsNull, pError, uColIndex, MYSQL_TYPE_SHORT, true );
	}

	void CDbStatement::SetResultInt32( int32* pBuffer, bool* pIsNull, bool* pError, uint32 uColIndex )
	{
		SetResultIntegerSeries( static_cast<void*>(pBuffer), pIsNull, pError, uColIndex, MYSQL_TYPE_LONG, false );
	}

	void CDbStatement::SetResultUint32( uint32* pBuffer, bool* pIsNull, bool* pError, uint32 uColIndex )
	{
		SetResultIntegerSeries( static_cast<void*>(pBuffer), pIsNull, pError, uColIndex, MYSQL_TYPE_LONG, true );
	}

	void CDbStatement::SetResultInt64( int64* pBuffer, bool* pIsNull, bool* pError, uint32 uColIndex )
	{
		SetResultIntegerSeries( static_cast<void*>(pBuffer), pIsNull, pError, uColIndex, MYSQL_TYPE_LONGLONG, false );
	}

	void CDbStatement::SetResultUint64( uint64* pBuffer, bool* pIsNull, bool* pError, uint32 uColIndex )
	{
		SetResultIntegerSeries( static_cast<void*>(pBuffer), pIsNull, pError, uColIndex, MYSQL_TYPE_LONGLONG, true );
	}

	void CDbStatement::SetResultFloat( float* pBuffer, bool* pIsNull, bool* pError, uint32 uColIndex )
	{
		SetResultRealSeries( static_cast<void*>(pBuffer), pIsNull, pError, uColIndex, MYSQL_TYPE_FLOAT );
	}

	void CDbStatement::SetResultDouble( double* pBuffer, bool* pIsNull, bool* pError, uint32 uColIndex )
	{
		SetResultRealSeries( static_cast<void*>(pBuffer), pIsNull, pError, uColIndex, MYSQL_TYPE_DOUBLE );
	}

	void CDbStatement::SetResultText( void* pBuffer, uint32 uBufferSize,
		ulong* uDataSize, bool* pIsNull, bool* pError, uint32 uColIndex )
	{
		SetResultBufferSeries( pBuffer, uBufferSize, uDataSize, pIsNull, pError, uColIndex, MYSQL_TYPE_STRING );
	}

	void CDbStatement::SetResultBinary( void* pBuffer, uint32 uBufferSize,
		ulong* uDataSize, bool* pIsNull, bool* pError, uint32 uColIndex )
	{
		SetResultBufferSeries( pBuffer, uBufferSize, uDataSize, pIsNull, pError, uColIndex, MYSQL_TYPE_BLOB );
	}

	void CDbStatement::SetResultIntegerSeries( void* pBuffer, bool* pIsNull, bool* pError,
		uint32 uColIndex, enum_field_types eBufferType, bool bIsUnsigned )
	{
		GammaAst( uColIndex < GetResultColNum() );

		MYSQL_BIND* pbind = m_aryResult + uColIndex;
		pbind->buffer = pBuffer;
		pbind->buffer_type = eBufferType;
		pbind->is_unsigned = bIsUnsigned;
		//pbind->length=&pbind->buffer_length;
		pbind->length = NULL;
		pbind->error = reinterpret_cast<my_bool*>(pError);
		pbind->is_null = reinterpret_cast<my_bool*>(pIsNull);
		m_bResultDirty = true;
	}


	void CDbStatement::SetResultRealSeries( void* pBuffer, bool* pIsNull, bool* pError,
		uint32 uColIndex, enum_field_types eBufferType )
	{
		GammaAst( uColIndex < GetResultColNum() );

		MYSQL_BIND* pbind = m_aryResult + uColIndex;
		pbind->buffer = pBuffer;
		pbind->buffer_type = eBufferType;
		//pbind->length=&pbind->buffer_length;
		pbind->length = NULL;
		pbind->error = reinterpret_cast<my_bool*>(pError);
		pbind->is_null = reinterpret_cast<my_bool*>(pIsNull);
		m_bResultDirty = true;
	}


	void CDbStatement::SetResultBufferSeries( void* pBuffer, uint32 uBufferSize,
		ulong* uDataSize, bool* pIsNull, bool* pError, uint32 uColIndex, enum_field_types eBufferType )
	{
		GammaAst( uColIndex < GetResultColNum() );

		MYSQL_BIND* pbind = m_aryResult + uColIndex;
		pbind->buffer = pBuffer;
		pbind->buffer_type = eBufferType;
		pbind->buffer_length = uBufferSize;
		pbind->length = reinterpret_cast<ulong*>(uDataSize);
		pbind->error = reinterpret_cast<my_bool*>(pError);
		pbind->is_null = reinterpret_cast<my_bool*>(pIsNull);
		m_bResultDirty = true;
	}

}
