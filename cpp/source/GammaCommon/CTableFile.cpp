/*
*	Tab File Operation Class
*/


#include "GammaCommon/CPkgFile.h"
#include "GammaCommon/CTableFile.h"
#include "GammaCommon/GammaMath.h"
#include "GammaCommon/GammaCodeCvs.h"
#include "GammaCommon/BinaryFind.h"
#include "GammaCommon/CDictionary.h"
#include "GammaCommon/GammaTime.h"
#include <map>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

namespace Gamma
{
	struct SFieldInfo
	{
		enum { eNone, eString, eInteger, eDouble, eDateStamp, eDictString };
		uint32						m_nOffset		: 29;
		uint32						m_nValueType	: 3;
		union 
		{
			int64					m_nInteger;
			double					m_fDouble;
			const char*				m_szString;
		};
	};

	struct STableFile
	{
		string						m_strBuf;
		vector<uint32>				m_arySortColumn;
		vector<SFieldInfo>			m_aryFieldInfo;
		uint32						m_nRowCount;
	};

	struct STableFileCompare
	{
		inline bool operator() ( uint32 nColumnIndex, const char* szStr )	const 
		{ 
			uint32 nOffset = m_pFile->m_aryFieldInfo[nColumnIndex].m_nOffset;
			return strcmp( &m_pFile->m_strBuf[nOffset], szStr ) < 0; 
		}

		inline bool operator() ( const char* szStr, uint32 nColumnIndex ) const 
		{ 
			uint32 nOffset = m_pFile->m_aryFieldInfo[nColumnIndex].m_nOffset;
			return strcmp( szStr, &m_pFile->m_strBuf[nOffset] ) < 0; 
		}

		inline bool operator() ( uint32 nLeftColumn, uint32 nRightColumn )	const 
		{ 
			uint32 nLeftOffset = m_pFile->m_aryFieldInfo[nLeftColumn].m_nOffset;
			uint32 nRightOffset = m_pFile->m_aryFieldInfo[nRightColumn].m_nOffset;
			return strcmp( &m_pFile->m_strBuf[nLeftOffset], &m_pFile->m_strBuf[nRightOffset] ) < 0; 
		}

		STableFile* m_pFile;
	};

	inline void Covert2Type( STableFile& TableFile, SFieldInfo& Info, uint32 nType )
	{
		if( Info.m_nValueType == nType )
			return;

		if( nType == SFieldInfo::eDictString )
		{
			const char* szString = &TableFile.m_strBuf[Info.m_nOffset];
			if( ( (uint8)szString[0] ) == CDictionary::eUtf8HeadTag0 &&
				( (uint8)szString[1] ) == CDictionary::eUtf8HeadTag1 &&
				( (uint8)szString[2] ) == CDictionary::eUtf8HeadTag2  )
				szString = szString + 3;
			szString = CDictionary::Inst().GetValue( szString );
			Info.m_szString = szString && szString[0] ? szString : NULL;
			Info.m_nValueType = SFieldInfo::eDictString;
			return;
		}

		if( nType == SFieldInfo::eString )
		{
			const char* szString = &TableFile.m_strBuf[Info.m_nOffset];
			if( ( (uint8)szString[0] ) == CDictionary::eUtf8HeadTag0 &&
				( (uint8)szString[1] ) == CDictionary::eUtf8HeadTag1 &&
				( (uint8)szString[2] ) == CDictionary::eUtf8HeadTag2  )
				szString = CDictionary::Inst().GetValue( CDictionary::StrToKey( szString + 3 ) );
			Info.m_szString = szString && szString[0] ? szString : NULL;
			Info.m_nValueType = SFieldInfo::eString;
			return;
		}

		if( Info.m_nValueType != SFieldInfo::eString )
		{
			Info.m_szString = &TableFile.m_strBuf[Info.m_nOffset];
			if( ( (uint8)Info.m_szString[0] ) == CDictionary::eUtf8HeadTag0 &&
				( (uint8)Info.m_szString[1] ) == CDictionary::eUtf8HeadTag1 &&
				( (uint8)Info.m_szString[2] ) == CDictionary::eUtf8HeadTag2  )
				Info.m_szString = CDictionary::Inst().GetValue( CDictionary::StrToKey( Info.m_szString + 3 ) );
			Info.m_szString = Info.m_szString && Info.m_szString[0] ? Info.m_szString : NULL;
			Info.m_nValueType = SFieldInfo::eString;
		}
		
		if( Info.m_nValueType == SFieldInfo::eString && !Info.m_szString )
			return;

		const char* szString = Info.m_szString;
		if( nType == SFieldInfo::eInteger )
			Info.m_nInteger = GammaA2I64( szString );
		else if( nType == SFieldInfo::eDouble )
			Info.m_fDouble = strtod( szString, NULL );
		else if( nType == SFieldInfo::eDateStamp )
			Info.m_nInteger = Str2LocalTime( szString );
		Info.m_nValueType = nType;
	}


	CTabFile::CTabFile( void ) 
		: m_pFile( new STableFile )
	{
		m_pFile->m_nRowCount = 0;
	}

	CTabFile::~CTabFile( void )
	{
		Clear();
		SAFE_DELETE( m_pFile );
	}

	// 载入制表符分隔文件
	bool CTabFile::Load( const char* szFileName, char cSeperator )
	{
		CPkgFile TabFile;
		if ( !TabFile.Open( szFileName ) )
			return false;
		if ( TabFile.Size() == 0 )
			return false;
		return Init( TabFile.GetFileBuffer(), TabFile.Size(), cSeperator );
	}

	// 载入制表符分隔文件
	bool CTabFile::Load( const wchar_t* szFileName, char cSeperator )
	{
		CPkgFile TabFile;
		if ( !TabFile.Open( szFileName ) )
			return false;
		if ( TabFile.Size() == 0 )
			return false;
		return Init( TabFile.GetFileBuffer(), TabFile.Size(), cSeperator );
	}

	bool CTabFile::Init( const tbyte* pBuffer, uint32 nSize, char cSeperator )
	{
		Clear();

		if( !pBuffer || !nSize )
			return false;

		if( pBuffer[0] == 0xef && 
			pBuffer[1] == 0xbb && 
			pBuffer[2] == 0xbf )
		{
			m_pFile->m_strBuf.assign( (const char*)pBuffer + 3, nSize - 3 );
		}
		else if( 
			pBuffer[0] == 0xff && 
			pBuffer[1] == 0xfe )
		{
			const uint16* pUcs2 = (const uint16*)( pBuffer + 2 );
			uint32 nLen = Ucs2ToUtf8( NULL, 0, pUcs2, nSize/sizeof(uint16) - 1 );
			m_pFile->m_strBuf.resize( nLen + 1 );
			Ucs2ToUtf8( &m_pFile->m_strBuf[0], nLen + 1, pUcs2, nSize/sizeof(uint16) - 1 );
		}
		else
		{
			if( !IsUtf8( (const char*)pBuffer, nSize ) )
				GammaThrow( "can not use asc file here!!" );
			m_pFile->m_strBuf.assign( (const char*)pBuffer, nSize );
		}

		if( m_pFile->m_strBuf.empty() )
			return false;
		return MakeOffset( cSeperator );
	}

	// 清空
	void CTabFile::Clear()
	{
		m_pFile->m_strBuf.clear();
		m_pFile->m_arySortColumn.clear();
		m_pFile->m_aryFieldInfo.clear();
		m_pFile->m_nRowCount = 0;
	}	

	// 得到行数
	int32 CTabFile::GetHeight() const
	{
		return (int32)m_pFile->m_nRowCount;
	}

	// 得到列数
	int32 CTabFile::GetWidth() const
	{
		return (int32)m_pFile->m_arySortColumn.size();
	}

	// 得到列名
	int32 CTabFile::GetCloumn( const char* szColumnName ) const
	{
		STableFileCompare Compare = { m_pFile };
		int32 nIndex = Find( m_pFile->m_arySortColumn, m_pFile->m_arySortColumn.size(), szColumnName, Compare );
		return nIndex < 0 ? -1 : (int32)m_pFile->m_arySortColumn[nIndex];
	}

	// 根据列号得到某行某列
	const char* CTabFile::GetString( int32 nRow, int32 nColumn, const char* szDefault ) const
	{
		if( (uint32)nColumn >= m_pFile->m_arySortColumn.size() )
			return szDefault;

		int32 nIndex = nRow*(int32)m_pFile->m_arySortColumn.size() + nColumn;
		if( (uint32)nIndex >= m_pFile->m_aryFieldInfo.size() )
			return szDefault;

		SFieldInfo& Info = m_pFile->m_aryFieldInfo[nIndex];
		Covert2Type( *m_pFile, Info, SFieldInfo::eString );
		return Info.m_szString ? Info.m_szString : szDefault;
	}

	// 根据列名得到某行某列
	const char* CTabFile::GetString( int32 nRow, const char* szColumnName, const char* szDefault ) const
	{
		int32 nColume = GetCloumn( szColumnName );
		return nColume < 0 ? szDefault : GetString( nRow, nColume, szDefault );
	}

	// 根据列号得到某行某列
	const char* CTabFile::GetDicString( int32 nRow, int32 nColumn, const char* szDefault ) const
	{
		if( (uint32)nColumn >= m_pFile->m_arySortColumn.size() )
			return szDefault;

		int32 nIndex = nRow*(int32)m_pFile->m_arySortColumn.size() + nColumn;
		if( (uint32)nIndex >= m_pFile->m_aryFieldInfo.size() )
			return szDefault;

		SFieldInfo& Info = m_pFile->m_aryFieldInfo[nIndex];
		Covert2Type( *m_pFile, Info, SFieldInfo::eDictString );
		return Info.m_szString ? Info.m_szString : szDefault;
	}

	// 根据列名得到某行某列
	const char* CTabFile::GetDicString( int32 nRow, const char* szColumnName, const char* szDefault ) const
	{
		int32 nColume = GetCloumn( szColumnName );
		return nColume < 0 ? szDefault : GetDicString( nRow, nColume, szDefault );
	}

	// 根据列号得到某行某列
	int32 CTabFile::GetInteger( int32 nRow, int32 nColumn, int32 defaultvalue ) const
	{
		if( (uint32)nColumn >= m_pFile->m_arySortColumn.size() )
			return defaultvalue;

		int32 nIndex = nRow*(int32)m_pFile->m_arySortColumn.size() + nColumn;
		if( (uint32)nIndex >= m_pFile->m_aryFieldInfo.size() )
			return defaultvalue;

		SFieldInfo& Info = m_pFile->m_aryFieldInfo[nIndex];
		Covert2Type( *m_pFile, Info, SFieldInfo::eInteger );		
		return Info.m_nValueType == SFieldInfo::eInteger ? (int32)Info.m_nInteger : defaultvalue;
	}

	// 根据列名得到某行某列
	int32 CTabFile::GetInteger( int32 nRow, const char* szColumnName, int32 defaultvalue ) const
	{
		int32 nColume = GetCloumn( szColumnName );
		return nColume < 0 ? defaultvalue : GetInteger( nRow, nColume, defaultvalue );
	}

	// 根据列号得到某行某列
	float CTabFile::GetFloat( int32 nRow, int32 nColumn, float defaultvalue ) const
	{
		if( (uint32)nColumn >= m_pFile->m_arySortColumn.size() )
			return defaultvalue;

		int32 nIndex = nRow*(int32)m_pFile->m_arySortColumn.size() + nColumn;
		if( (uint32)nIndex >= m_pFile->m_aryFieldInfo.size() )
			return defaultvalue;

		SFieldInfo& Info = m_pFile->m_aryFieldInfo[nIndex];
		Covert2Type( *m_pFile, Info, SFieldInfo::eDouble );		
		return Info.m_nValueType == SFieldInfo::eDouble ? (float)Info.m_fDouble : defaultvalue;
	}

	// 根据列名得到某行某列
	float CTabFile::GetFloat( int32 nRow, const char* szColumnName, float defaultvalue ) const
	{
		int32 nColume = GetCloumn( szColumnName );
		return nColume < 0 ? defaultvalue : GetFloat( nRow, nColume, defaultvalue );
	}

	int64 CTabFile::GetInteger64( int32 nRow, int32 nColumn, int64 defaultvalue /*= 0 */ ) const
	{
		if( (uint32)nColumn >= m_pFile->m_arySortColumn.size() )
			return defaultvalue;

		int32 nIndex = nRow*(int32)m_pFile->m_arySortColumn.size() + nColumn;
		if( (uint32)nIndex >= m_pFile->m_aryFieldInfo.size() )
			return defaultvalue;

		SFieldInfo& Info = m_pFile->m_aryFieldInfo[nIndex];
		Covert2Type( *m_pFile, Info, SFieldInfo::eInteger );		
		return Info.m_nValueType == SFieldInfo::eInteger ? Info.m_nInteger : defaultvalue;
	}

	int64 CTabFile::GetInteger64( int32 nRow, const char* szColumnName, int64 defaultvalue ) const
	{
		int32 nColume = GetCloumn( szColumnName );
		return nColume < 0 ? defaultvalue : GetInteger64( nRow, nColume, defaultvalue );
	}	

	double CTabFile::GetDouble( int32 nRow, int32 nColumn, double defaultvalue /*= 0 */ ) const
	{
		if( (uint32)nColumn >= m_pFile->m_arySortColumn.size() )
			return defaultvalue;

		int32 nIndex = nRow*(int32)m_pFile->m_arySortColumn.size() + nColumn;
		if( (uint32)nIndex >= m_pFile->m_aryFieldInfo.size() )
			return defaultvalue;

		SFieldInfo& Info = m_pFile->m_aryFieldInfo[nIndex];
		Covert2Type( *m_pFile, Info, SFieldInfo::eDouble );		
		return Info.m_nValueType == SFieldInfo::eDouble ? Info.m_fDouble : defaultvalue;
	}

	double CTabFile::GetDouble( int32 nRow, const char* szColumnName, double defaultvalue ) const
	{
		int32 nColume = GetCloumn( szColumnName );
		return nColume < 0 ? defaultvalue : GetDouble( nRow, nColume, defaultvalue );
	}

	int64 CTabFile::GetDateSec( int32 nRow, int32 nColumn, int64 defaultvalue /*= 0 */ ) const
	{
		if( (uint32)nColumn >= m_pFile->m_arySortColumn.size() )
			return defaultvalue;

		int32 nIndex = nRow*(int32)m_pFile->m_arySortColumn.size() + nColumn;
		if( (uint32)nIndex >= m_pFile->m_aryFieldInfo.size() )
			return defaultvalue;

		SFieldInfo& Info = m_pFile->m_aryFieldInfo[nIndex];
		Covert2Type( *m_pFile, Info, SFieldInfo::eDateStamp );		
		return Info.m_nValueType == SFieldInfo::eDateStamp ? Info.m_nInteger : defaultvalue;
	}

	int64 CTabFile::GetDateSec( int32 nRow, const char* szColumnName, int64 defaultvalue /*= 0 */ ) const
	{
		int32 nColume = GetCloumn( szColumnName );
		return nColume < 0 ? defaultvalue : GetDateSec( nRow, nColume, defaultvalue );
	}

	bool CTabFile::MakeOffset( char cSeperator )
	{
		uint32 nIndex = 0;
		uint32 nColumnCount = 1;
		char* pBuffer = &m_pFile->m_strBuf[0];
		uint32 nBuffEnd = (uint32)m_pFile->m_strBuf.size() - 1;
		while( nBuffEnd && ( pBuffer[ nBuffEnd - 1 ] == '\r' || pBuffer[ nBuffEnd - 1 ] == '\n' ) )
			pBuffer[--nBuffEnd] = 0;

		cSeperator = cSeperator ? cSeperator : '\t';
		while( nIndex < nBuffEnd && pBuffer[nIndex] != '\n' )
			if( pBuffer[nIndex++] == cSeperator )
				nColumnCount++;

		m_pFile->m_nRowCount = 1;

		while( nIndex < nBuffEnd && pBuffer[nIndex] )
			if( pBuffer[nIndex++] == '\n' )
				m_pFile->m_nRowCount++;

		// 初始化为空串
		SFieldInfo Info = { nBuffEnd, SFieldInfo::eNone };
		m_pFile->m_aryFieldInfo.resize( nColumnCount*m_pFile->m_nRowCount, Info );

		for( uint32 i = 0, n = 0; i < m_pFile->m_nRowCount; i++ )
		{
			char c = 0;
			for( uint32 j = 0, m = i*nColumnCount; j < nColumnCount; j++, m++ )
			{
				m_pFile->m_aryFieldInfo[m].m_nOffset = n;
				while( pBuffer[n] && pBuffer[n] != cSeperator && pBuffer[n] != '\n' )
					n++;
				
				c = pBuffer[n];
				pBuffer[n++] = 0;
				if( c == cSeperator )
					continue;
				if( n >= 2 && pBuffer[ n - 2 ] == '\r' )
					pBuffer[ n - 2 ] = 0;
				break;
			}

			if( c == cSeperator )
			{
				while( pBuffer[n] && pBuffer[n] != '\n' )
					n++;
				n++;
			}
		}

		m_pFile->m_arySortColumn.resize( nColumnCount );
		for( uint32 i = 0; i < nColumnCount; i++ )
			m_pFile->m_arySortColumn[i] = i;

		STableFileCompare Compare = { m_pFile };
		std::sort( m_pFile->m_arySortColumn.begin(), m_pFile->m_arySortColumn.end(), Compare );
		return true;
	}
}
