
#include "GammaCommon/CDictionary.h"
#include "GammaCommon/TConstString.h"

namespace Gamma
{
	//==========================================
	// CDictionary::SLocalizeString
	//==========================================
	uint32 CDictionary::SLocalizeString::SetNewKey( uint32 nNewKey, const char* szNewValue )
	{
		if( eInvalidKey == nNewKey )
			return _InvalidKey;

		if( CDictionary::Inst().ExistKey( nNewKey ) )
			CDictionary::Inst().Erase( nNewKey );
		if( NULL == szNewValue || szNewValue[0] == 0 )
		{
			m_nKey = nNewKey;
			return _Ok;
		}
		m_nKey = CDictionary::Inst().AddValueWithOffset( szNewValue, nNewKey );
		if( m_nKey != nNewKey )
			return _Diff;
		m_szValue = CDictionary::Inst().GetValue( m_nKey );
		return _Ok;
	}

	void CDictionary::SLocalizeString::clear()
	{
		CDictionary::Inst().Erase( m_nKey );
		m_szValue = NULL; 
		m_nKey = m_bReset ? eInvalidKey : m_nKey;
	}

	const char* CDictionary::SLocalizeString::operator = ( const char* _Right )
	{
		clear();
		if( _Right != NULL && _Right[0] != 0 )
		{
			m_nKey = CDictionary::Inst().AddValueWithOffset( _Right, m_nKey );
			m_szValue = CDictionary::Inst().GetValue( m_nKey );
		}
		return m_szValue;
	}

	struct SDictionaryBuffer
	{
		SDictionaryBuffer(): m_nOffset( 1 )
		{ memset( m_bModify, 0, sizeof(m_bModify) ); }
		typedef std::map<CDictionary::KeyType, gammacstring> MapType;
		MapType		m_mapLocalString;
		bool		m_bModify[16];
		std::string	m_strDirectory;
		uint32		m_nOffset; 
	};

	//==========================================
	// CDictionary
	//==========================================
	CDictionary::CDictionary()
		: m_pDictionary( new SDictionaryBuffer )
	{
	}

	CDictionary::~CDictionary()
	{
		delete m_pDictionary;
	}

	bool CDictionary::LoadFromDir( const char* _szDir )
	{
		char szFileName[1024];
		uint32 nLen = 0;
		while( _szDir[nLen] )
		{
			szFileName[nLen] = _szDir[nLen];
			nLen++;
		}

		if( szFileName[ nLen - 1 ] != '\\' && szFileName[ nLen - 1 ] != '/' )
			szFileName[nLen++] = '/';
		uint32 nIndexPos = nLen;
		szFileName[nLen++] = '0';
		szFileName[nLen++] = '.';
		szFileName[nLen++] = 't';
		szFileName[nLen++] = 'x';
		szFileName[nLen++] = 't';
		szFileName[nLen++] = 0;

		for( uint8 i = 0; i <= 0xf; i++ )
		{
			szFileName[nIndexPos] = i <= 9 ? '0' + i : 'a' + i - 10;
			Load( szFileName );
		}

		return true;
	}

	bool CDictionary::LoadFromDir( const wchar_t* szDir )
	{
		return LoadFromDir( UcsToUtf8( szDir ).c_str() );
	}

	bool CDictionary::Load( const char* szFileName )
	{
		if( !szFileName )
			return false;

		CTabFile tabFile;
		if( !tabFile.Load( szFileName ) || tabFile.GetHeight() == 0 )
			return false;

		for( int32 nRow = 0; nRow < tabFile.GetHeight(); nRow++ )
		{
			const char* szKey = tabFile.GetString( nRow, 0 );
			const char* szValue = tabFile.GetString( nRow, 1 );
			KeyType nKey = (KeyType)StrToKey( szKey );
			if( nKey == eInvalidKey )
				continue;
			m_pDictionary->m_mapLocalString[nKey].assign( szValue, false );
		}

		m_pDictionary->m_strDirectory = GammaString( szFileName );
        std::string::size_type nPos = m_pDictionary->m_strDirectory.find_last_of( '/' );
		m_pDictionary->m_strDirectory.erase( nPos );
		return true;
	}

	bool CDictionary::Load( const wchar_t* szFileName )
	{
		return Load( UcsToUtf8( szFileName ).c_str() );
	}

	bool CDictionary::Save()
	{		
		std::map<KeyType, gammacstring>& mapLocalString = m_pDictionary->m_mapLocalString;
		for( uint32 i = 0; i < 16; i++ )
		{
			if( !m_pDictionary->m_bModify[i] )
				continue;

			uint32 nKeyBegin = i << 28;
			uint32 nKeyEnd = nKeyBegin|0x0fffffff;
			std::map<KeyType, gammacstring>::iterator itBegin = mapLocalString.lower_bound( nKeyBegin ); 
			std::map<KeyType, gammacstring>::iterator itEnd = mapLocalString.upper_bound( nKeyEnd ); 

			std::string strFileName = m_pDictionary->m_strDirectory + "/" ;
			strFileName.push_back( (char)( i <= 9 ? '0' + i : 'a' + i - 10 ) );
			strFileName += ".txt";

            std::ios_base::openmode nFlag = std::ios::out | std::ios::binary;
#ifndef _WIN32
			std::ofstream ofs( strFileName.c_str(), nFlag );
#else
			std::ofstream ofs( Utf8ToUcs( strFileName.c_str() ).c_str(), nFlag );
#endif
			if( !ofs )
				return false;

			const uint8 _tag[] = { 0xFF, 0xFE };
			ofs.write( (const char*)_tag, sizeof(_tag) );

			for( std::map<KeyType, gammacstring>::iterator j = itBegin; j != itEnd; j++ )
			{
				char szTemp[32];
				sprintf( szTemp, "%08x\t",j->first );
				std::wstring strCombine = Utf8ToUcs( szTemp ) + 
					Utf8ToUcs( j->second.c_str() ) + std::wstring( L"\r\n" );
				for( size_t i = 0; i < strCombine.size(); i++ )
					ofs.write( (const char*)&strCombine[i], sizeof(uint16) );
			}
			ofs.close();
		}

		memset( m_pDictionary->m_bModify, 0, sizeof(m_pDictionary->m_bModify) );
		return true;
	}

	bool CDictionary::ExistKey( CDictionary::KeyType nKey )
	{
		return m_pDictionary->m_mapLocalString.find( nKey ) != m_pDictionary->m_mapLocalString.end();
	}

	CDictionary::KeyType CDictionary::AddValueWithOffset( const wchar_t* szValue, uint32 nKey )
	{
		if( !szValue )
			return (KeyType)eInvalidKey;
		return AddValueWithOffset( UcsToUtf8( szValue ).c_str(), nKey );
	}

	CDictionary::KeyType CDictionary::AddValueWithOffset( const char* szValue, uint32 nKey )
	{
		if( !szValue || strlen( szValue ) == 0 )
			return (KeyType)eInvalidKey;

		KeyType nCurKey = (KeyType)eInvalidKey;
		if( nKey != eInvalidKey )
		{
			if( CDictionary::Inst().ExistKey( nKey ) )
				GammaLog << "!!! AddValueWithOffset error " << std::endl;
			nCurKey = nKey;
		}
		else
			nCurKey = GetKey();

		if( ExistKey(nCurKey ) )
			return (KeyType)eInvalidKey;

		if( nCurKey == (KeyType)eInvalidKey )
			return (KeyType)eInvalidKey;

		m_pDictionary->m_mapLocalString[nCurKey].assign( szValue );
		m_pDictionary->m_bModify[ nCurKey >> 28 ] = true;
		return nCurKey;
	}

	void CDictionary::SetOffset( uint32 nOffset )
	{
		m_pDictionary->m_nOffset = nOffset;
	}

	uint32 CDictionary::GetOffset() const
	{
		return m_pDictionary->m_nOffset;
	}

	bool CDictionary::SetValue( CDictionary::KeyType nKey, const wchar_t* szValue )
	{
		return SetValue( nKey, UcsToUtf8( szValue ).c_str() );
	}

	bool CDictionary::SetValue( CDictionary::KeyType nKey, const char* szValue )
	{
		if( !szValue || !szValue[0] )
			return false;

		std::map<KeyType, gammacstring>& mapLocalString = m_pDictionary->m_mapLocalString;
		std::map<KeyType, gammacstring>::iterator i = mapLocalString.find( nKey );
		if( i == mapLocalString.end() )
			return false;
		i->second.assign( szValue );
		m_pDictionary->m_bModify[ nKey >> 28 ] = true;
		return true;
	}

	const char* CDictionary::GetValue( CDictionary::KeyType nKey )
	{
		std::map<KeyType, gammacstring>& mapLocalString = m_pDictionary->m_mapLocalString;
		std::map<KeyType, gammacstring>::iterator i = mapLocalString.find( nKey );
		return i == mapLocalString.end() ? NULL : i->second.c_str();
	}

	const char* CDictionary::GetValue( const char* szKey )
	{ 
		CDictionary::KeyType nKey = StrToKey( szKey );
		return nKey == CDictionary::eInvalidKey ? NULL : GetValue( nKey );
	}

	void CDictionary::Clear()
	{
		m_pDictionary->m_mapLocalString.clear();
		memset( m_pDictionary->m_bModify, 0, sizeof(m_pDictionary->m_bModify) );
	}

	CDictionary& CDictionary::Inst()
	{
		static CDictionary _inst;
		return _inst;
	}

	void CDictionary::Erase( KeyType nKey )
	{
		std::map<KeyType, gammacstring>& mapLocalString = m_pDictionary->m_mapLocalString;
		std::map<KeyType, gammacstring>::iterator i = mapLocalString.find( nKey );
		if( i == mapLocalString.end() )
			return;
		m_pDictionary->m_mapLocalString.erase( i );
		m_pDictionary->m_bModify[ nKey >> 28 ] = true;
	}

	void CDictionary::Erase( CDictionary::KeyType nKeyBegin, CDictionary::KeyType nKeyEnd )
	{
		nKeyEnd = Max( nKeyEnd, nKeyBegin + 1 );
		std::map<KeyType, gammacstring>& mapLocalString = m_pDictionary->m_mapLocalString;
		std::map<KeyType, gammacstring>::iterator itBegin = mapLocalString.lower_bound( nKeyBegin );
		std::map<KeyType, gammacstring>::iterator itEnd = mapLocalString.lower_bound( nKeyEnd );
		mapLocalString.erase( itBegin, itEnd );

		nKeyBegin = nKeyBegin >> 28;
		nKeyEnd = ( nKeyEnd - 1 ) >> 28;

		for( uint32 i = nKeyBegin; i <= nKeyEnd; i++ )
			m_pDictionary->m_bModify[i] = true;
	}

	void CDictionary::Erase( const char* szKey )
	{
		Erase( StrToKey( szKey ) );
	}

	bool CDictionary::HasCH( const char* szValue )
	{
		if( !szValue )
			return false;
		for( size_t i = 0; szValue[i]; i++ )
			if( szValue[i] < 0 )
				return true;
		return false;
	}

	bool CDictionary::IsValidKey( const char* szKey )
	{
		if( !szKey )
			return false;

		if( szKey[0] == '0' && ( szKey[1] == 'x' || szKey[1] == 'X' ) )
			szKey += 2;

		for( size_t i = 0; i < 8; i++ )
			if( ValueFromHexNumber( szKey[i] ) < 0 )
				return false;
		return true;
	}

	template<typename CharType>
	CDictionary::KeyType _StrToKey( const CharType* szKey )
	{	
		if( !szKey )
			return (CDictionary::KeyType)CDictionary::eInvalidKey;

		if( szKey[0] == '0' && ( szKey[1] == 'x' || szKey[1] == 'X' ) )
			szKey += 2;

		CDictionary::KeyType nKey = 0;
		for( size_t i = 0; i < 8; i++ )
		{
			int32 n = ValueFromHexNumber( szKey[i] );
			if( n < 0 )
				return (CDictionary::KeyType)CDictionary::eInvalidKey;
			nKey = ( nKey << 4 ) + n;
		}

		return (CDictionary::KeyType)nKey;
	}

	CDictionary::KeyType CDictionary::StrToKey( const wchar_t* szKey )
	{
		return _StrToKey( HasTagHead( szKey ) ? szKey + 1 : szKey );
	}

	CDictionary::KeyType CDictionary::StrToKey( const char* szKey )
	{
		return _StrToKey( HasTagHead( szKey ) ? szKey + 3 : szKey );
	}

	uint8 CDictionary::MakeID( KeyType nValue )
	{
		return (uint8) ( uint32( ( nValue >> 28 ) & 0xf ) ) ;
	}

	CDictionary::KeyType CDictionary::GetKey() 
	{
		uint32 nOffset = GetOffset();

		std::map<KeyType, gammacstring>& mapLocalString = m_pDictionary->m_mapLocalString;
		if( mapLocalString.empty() )
			return (KeyType) nOffset;

		KeyType nCurKey = (KeyType)eInvalidKey;

		std::map<KeyType, gammacstring>::const_iterator i, j;
		std::map<KeyType, gammacstring>::const_iterator iOffset = mapLocalString.find( nOffset );
		if( iOffset != mapLocalString.end() )
		{
			j = iOffset;
			i = j++;
		}
		else
		{
			j = mapLocalString.begin();
			i = j++;

			//if( j->first > nOffset )
				return (KeyType) nOffset;
		}

		nCurKey = (KeyType)eInvalidKey;
		for( ; j != mapLocalString.end(); ++i, ++j )
		{
			if( (  ( i->first + 1 ) != j->first ) && ( ( i->first + 1 ) >= nOffset ) )
			{
				nCurKey = i->first + 1;
				break;
			}
		}

		if( j == mapLocalString.end() )
		{
			if( i->first + 1 >= nOffset )
				nCurKey = i->first + 1;
			else 
				nCurKey = nOffset;
		}

		return nCurKey;
	}
}
