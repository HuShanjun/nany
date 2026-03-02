
#include "GammaCommon/CIniFile.h"
#include "GammaCommon/CPkgFile.h"
#include "GammaCommon/GammaMath.h"
#include "GammaCommon/GammaCodeCvs.h"
#include "GammaCommon/TGammaStrStream.h"
#include "GammaCommon/CPathMgr.h"

using namespace std;

namespace Gamma
{
	class SSection : public map<string, string> {};

	struct SIniFile
	{
		string					m_sFileName;
		ETextFileType			m_eType;
		map<string, SSection>	m_Struct;
	};

	void			NextLine( char*& lpBuffer );
	const char*		GetWord( char*& lpBuffer, char cEndChar );
	bool			BuildKey( char*& lpBuffer, map<string, string>& Section);


	CIniFile::CIniFile()
		: m_pFile( new SIniFile )
	{	
		m_pFile->m_eType = eTFT_Ucs2;
	}

	CIniFile::~CIniFile()
	{
		Close();
		delete m_pFile;
	}

	const char* CIniFile::GetFileName()
	{
		return m_pFile ? m_pFile->m_sFileName.c_str() : NULL;
	}

	// 打开文件，扫描一遍，为以后的读操作做好准备(Get...)
	bool CIniFile::Open( const char* szFileName )
	{
		CPkgFile	File;
		int32		nSize;

		if( !szFileName || !szFileName[0] )
			return false;

		// comp file name  strcmp
		if( m_pFile->m_sFileName == szFileName )
			return true;

		// 先释放
		Close();

		// save file name
		m_pFile->m_sFileName = szFileName;

		// open Ini file  , 在内部形成全路径
		if ( !File.Open( szFileName ) )
			return false;

		// get file size
		if ( ( nSize = File.Size() ) == 0 )
			return false;

		// read data from file
		return Init( File.GetFileBuffer(), nSize );
	}

	// 打开文件，扫描一遍，为以后的读操作做好准备(Get...)
	bool CIniFile::Open( const wchar_t* szFileName )
	{
		return Open( UcsToUtf8( szFileName ).c_str() );
	}

	bool CIniFile::Init( const tbyte* pBuffer, uint32 nSize )
	{
		Clear();

		if( !pBuffer || !nSize )
			return false;

		string strFileBuffer;

		if( pBuffer[0] == 0xef && 
			pBuffer[1] == 0xbb && 
			pBuffer[2] == 0xbf )
		{
			strFileBuffer.assign( (const char*)( pBuffer + 3 ), nSize - 3 );
			m_pFile->m_eType = eTFT_Utf8;
		}
		else if( 
			pBuffer[0] == 0xff && 
			pBuffer[1] == 0xfe )
		{
			const uint16* pUcs2 = (const uint16*)( pBuffer + 2 );
			uint32 nLen = Ucs2ToUtf8( NULL, 0, pUcs2, (uint32)( nSize/sizeof(uint16) ) - 1 );
			strFileBuffer.resize( nLen + 1 );
			Ucs2ToUtf8( &strFileBuffer[0], nLen + 1, pUcs2, nSize/sizeof(uint16) - 1 );
			m_pFile->m_eType = eTFT_Ucs2;
		}
		else
		{
			strFileBuffer.assign( (const char*)pBuffer, nSize ); 
			if( !IsUtf8( strFileBuffer.c_str() ) )
				GammaThrow( "can not use asc file here!!" );
			m_pFile->m_eType = eTFT_Utf8NoneFlag;
		}

		if( strFileBuffer.empty() )
			return false;

		// 建立内部链表
		return Prepare( &strFileBuffer[0] );
	}

	// 保存文件
	bool CIniFile::Save( const char* szFileName, ETextFileType eType )
	{
		if( !szFileName || !szFileName[0] )
			szFileName = m_pFile->m_sFileName.c_str();

		// comp file name  strcmp
		if( !szFileName || !szFileName[0] )
			return false;

		class _SSaveFile 
			: public IGammFileInterface
			, public opkgstream
		{
		public:
			_SSaveFile( const char* szName ) : opkgstream( szName ){}
			int32 Size()									{ return 0; }
			int32 Read( void* pBuf, uint32 nSize )			{ return 0; }
			int32 Write( const void* pBuf, uint32 nSize )	{ opkgstream::write( (const char*)pBuf, nSize ); return nSize; }
			void  Seek( uint32 nPos )						{ opkgstream::seekp( nPos ); }
		};

		char szBuffer[2048];
		CPathMgr::ToPhysicalPath( szFileName, szBuffer, ELEM_COUNT( szBuffer ) );
		_SSaveFile File( szBuffer );
		return WriteTo( &File, eType );
	}

	bool CIniFile::WriteTo( IGammFileInterface* pFile, ETextFileType eType )
	{
		uint8 cFlag1[3] = { 0xef, 0xbb, 0xbf };
		uint8 cFlag2[2] = { 0xff, 0xfe };

		eType = ( eType == eTFT_Default ) ? m_pFile->m_eType : eType;
		m_pFile->m_eType = eType;

		if( eType == eTFT_Ucs2 )
			pFile->Write( cFlag2, 2 );
		else if( eType == eTFT_Utf8 )
			pFile->Write( cFlag1, 3 );

		struct SWrite
		{
			void Write( IGammFileInterface* pFile, const string& szStr, ETextFileType eType )
			{
				if( eType == eTFT_Ucs2 )
				{
					wstring szTemp = Utf8ToUcs( szStr.c_str() );
					for( size_t i = 0; i < szTemp.size(); i++ )
						pFile->Write( szTemp.c_str() + i, sizeof(uint16) );
				}
				else
				{
					pFile->Write( szStr.c_str(), (uint32)szStr.size() );
				}
			}			
		};

		for( map< string, SSection >::iterator it = m_pFile->m_Struct.begin(); 
			it != m_pFile->m_Struct.end(); it++ )
		{
			SWrite().Write( pFile, "[" + it->first + "]\r\n", eType );
			for( map<string, string>::iterator itKey = it->second.begin();
				itKey != it->second.end(); itKey++ )
				SWrite().Write( pFile, itKey->first + " = " + itKey->second + "\r\n", eType );
		}

		return true;
	}

	/*
	*	建立内部链表
	*/
	bool CIniFile::Prepare( char* lpBuffer )
	{
		map<string, string>* pSection = NULL;
		while( 0 != *lpBuffer )
		{
			if( IsBlank( *lpBuffer ) )
			{
				lpBuffer++;
				continue;
			}

			if( '/' == *lpBuffer || ';' == *lpBuffer )
			{
				NextLine( lpBuffer );
				continue;
			}

			if( '[' == *lpBuffer )
			{
				lpBuffer++;
				const char* szSection = GetWord( lpBuffer, ']' );
				if( !szSection )
					return false;
				pSection = &m_pFile->m_Struct[szSection];
				continue;
			}

			if( pSection == NULL )
				return false;

			if( !BuildKey( lpBuffer, *pSection ) )
				return false;
		}

		return true;  
	}

	// 清除文件内容
	void CIniFile::Clear()
	{
		m_pFile->m_Struct.clear();
	}

	// 关闭文件,释放内存
	void CIniFile::Close()
	{
		m_pFile->m_sFileName.clear();
		m_pFile->m_Struct.clear();
	}	

	// 得到下一个Section下的名字，NULL获取第一个
	const char* CIniFile::GetNextSection( const char* lpSection )
	{
		if( !lpSection )
			return m_pFile->m_Struct.empty() ? NULL : m_pFile->m_Struct.begin()->first.c_str();

		map< string, SSection >::iterator itSection = m_pFile->m_Struct.find( lpSection );
		if( itSection == m_pFile->m_Struct.end() )
			return NULL;

		if( ++itSection == m_pFile->m_Struct.end() )
			return NULL;

		return itSection->first.c_str();
	}

	// 得到某个Section下的下一个Key下的名字，NULL获取第一个
	const char* CIniFile::GetNextKey( const char* lpSection, const char* lpKeyName )
	{
		if( !lpSection )
			return NULL;

		map< string, SSection >::iterator itSection = m_pFile->m_Struct.find( lpSection );
		if( itSection == m_pFile->m_Struct.end() )
			return NULL;

		if( !lpKeyName )
			return itSection->second.empty() ? NULL : itSection->second.begin()->first.c_str();

		map<string,string>::iterator itKey = itSection->second.find( lpKeyName );
		if( itKey == itSection->second.end() )
			return NULL;

		if( ++itKey == itSection->second.end() )
			return NULL;
		
		return itKey->first.c_str();
	}

	// 遍历链表，查找匹配的值, 注意结果Buf 不能大于Size
	const char* CIniFile::GetString( const char* lpSection, const char* lpKeyName, const char* szDefault )
	{
		// 能执行到这里，要求ini文件已成功的打开了, 链表也已成功的建立了.
		//GammaAst( NULL != m_lpSectionCur);
		if( !lpSection || !lpKeyName )
			return szDefault;

		map< string, SSection >::iterator itSection = m_pFile->m_Struct.find( lpSection );
		if( itSection == m_pFile->m_Struct.end() )
			return szDefault;

		map<string,string>::iterator itKey = itSection->second.find( lpKeyName );
		if( itKey == itSection->second.end() )
			return szDefault;

		return itKey->second.c_str();
	}

	// 判断某个Section是否存在或者Section下的Key的配置是否存在
	bool CIniFile::HasSetting(const char* lpSection, const char* lpKeyName)
	{
		if (!lpSection)
			return true;

		map< string, SSection >::iterator itSection = m_pFile->m_Struct.find(lpSection);
		if (itSection == m_pFile->m_Struct.end())
			return false;

		if (!lpKeyName)
			return true;

		map<string, string>::iterator itKey = itSection->second.find(lpKeyName);
		if (itKey == itSection->second.end())
			return false;

		return true;
	}

	//	读取一个整数
	int32 CIniFile::GetInteger( const char* lpSection, const char* lpKeyName, int32 nDefault )
	{
		const char* szValue = GetString( lpSection, lpKeyName );
		return szValue ? GammaA2I( szValue ) : nDefault;
	}

	//	读取一个整数
	int64 CIniFile::GetInteger64( const char* lpSection, const char* lpKeyName, int64 nDefault )
	{
		const char* szValue = GetString( lpSection, lpKeyName );
		return szValue ? GammaA2I64( szValue ) : nDefault;
	}

	//	读取一个整数
	char CIniFile::GetChar( const char* lpSection, const char* lpKeyName, char chDefault )
	{
		const char* szValue = GetString( lpSection, lpKeyName );
		return szValue ? szValue[0] : chDefault;
	};

	//	读取一个浮点数
	float CIniFile::GetFloat( const char* lpSection, const char* lpKeyName, float fDefault )
	{
		const char* szValue = GetString( lpSection, lpKeyName );
		return szValue ? (float)GammaA2F( szValue ) : fDefault;	
	}	

	// 写入某个Section下的Key的值，字符串
	void CIniFile::WriteString( const char* lpSection, const char* lpKeyName, const char* lpString )
	{
		m_pFile->m_Struct[lpSection][lpKeyName] = lpString;
	}

	// 写入某个Section下的Key的值，字符串
	void CIniFile::WriteInteger( const char* lpSection, const char* lpKeyName, int nValue )
	{
		char szResult[32];
		gammasstream( szResult, ELEM_COUNT( szResult ) ) << nValue;
		WriteString( lpSection, lpKeyName, szResult );
	}

	// 写入某个Section下的Key的值，字符串
	void CIniFile::WriteInteger64( const char* lpSection, const char* lpKeyName, int64 nValue )
	{
		char szResult[64];
		gammasstream( szResult, ELEM_COUNT( szResult ) ) << nValue;
		WriteString( lpSection, lpKeyName, szResult );
	}

	// 写入某个Section下的Key的值，字符串
	void CIniFile::WriteFloat( const char* lpSection, const char* lpKeyName, float fValue )
	{
		char szResult[32];
		gammasstream( szResult, ELEM_COUNT( szResult ) ) << fValue;
		WriteString( lpSection, lpKeyName, szResult );
	}

	//---------------------------------------------------------------------------
	// Function     : 把Buffer指针移到下一行, 比如处理注释时需要.
	// Comment      : 传入的Buffer指针是一个引用，内部修改后可影响外部.
	//              : 当碰到Buffer结尾时（0），返回的lpBuffer 指向 0
	//              : 当碰到行结束时
	//---------------------------------------------------------------------------
	void NextLine( char*& lpBuffer )
	{
		GammaAst(NULL != lpBuffer);

		char cChar = *lpBuffer;

		while( 0 != cChar )
		{
			if( 0x0d == cChar )
			{
				GammaAst(lpBuffer[1] == 0x0a);
				lpBuffer += 2;
				return;
			}

			if (0x0a == cChar)
			{
				lpBuffer++;
				return;
			}

			cChar = *++lpBuffer;
		}
	}

	//---------------------------------------------------------------------------
	// Function     :  读取Section的名字，传给AddtoSectionList
	//---------------------------------------------------------------------------
	const char* GetWord( char*& lpBuffer, char cEndChar )
	{
		GammaAst( NULL != lpBuffer );

		while( *lpBuffer == ' ' || *lpBuffer == '\t' )
			lpBuffer++;

		const char* szWordStart = lpBuffer;
		while( *lpBuffer && *lpBuffer != '\r' && *lpBuffer != '\n' && *lpBuffer != cEndChar  )
			lpBuffer++;

		if( *lpBuffer != cEndChar )
			return NULL;

		char* szWordEnd = lpBuffer - 1;
		*lpBuffer++ = 0;

		while( ( *szWordEnd == ' ' || *szWordEnd == '\t' ) && szWordEnd >= szWordStart )
			*szWordEnd-- = 0;
		return szWordStart;
	}

	// 形成KeyName, 及KeyValue
	bool BuildKey( char*& lpBuffer, map<string, string>& Section )
	{
		const char* szKey = GetWord( lpBuffer, '=' );
		if( !szKey )
			return false;

		while( *lpBuffer == ' ' || *lpBuffer == '\t' )
			lpBuffer++;

		const char* szWord = lpBuffer;
		while( *lpBuffer && *lpBuffer != '\r' && *lpBuffer != '\n' )
			lpBuffer++;

		for( char* pEnd = lpBuffer - 1; IsBlank( *pEnd ) && pEnd >= szWord; pEnd-- )
			*pEnd = 0;

		if( *lpBuffer )
			*lpBuffer++ = 0;

		Section[szKey] = szWord;
		return true;
	}

}
