/** 多语言版本字典管理
author:lye
*/
#pragma once
#include "GammaHelp.h"
#include "CTableFile.h"
#include "CPkgFile.h"
#include "GammaCodeCvs.h"
#include <sstream>
#include <string>
#include <map>
#include <set>

namespace Gamma
{
	class IFileWritable
	{
	public:
		virtual void Write( const void* pBuffer, size_t nSize ) = 0;
	};

	struct SDictionaryBuffer;

	class GAMMA_COMMON_API CDictionary
	{
	protected:
		CDictionary();
		~CDictionary();

	public:
		static CDictionary& Inst();

		typedef uint32_t KeyType;
		enum 
		{ 
			eInvalidKey     = 0xffffffff,
			eAutoMaxKey     = 0x80000000,
			eHeadTag        = 0x865f,
            eUtf8HeadTag0   = 0xe8,
            eUtf8HeadTag1   = 0x99,
            eUtf8HeadTag2   = 0x9f,
		};

		struct SLocalizeString
		{
			enum { _Ok, _InvalidKey, _ExistKey, _Diff, _Unknown };

			CDictionary::KeyType m_nKey;
			const char* m_szValue;
			bool m_bReset;

			friend class CDictionary;
		public:
			SLocalizeString( bool bReset = false )
				: m_nKey( (CDictionary::KeyType)eInvalidKey )
				, m_szValue(NULL)
				, m_bReset( bReset ) {}

			const char* c_str() const { return m_szValue; };
			const char* operator = ( const char* _Right );
			const char* operator = ( const std::string& _Right ){ return *this = _Right.c_str(); }
			bool operator == ( const std::string& _Right ) const{ return _Right == m_szValue; }

			uint32_t GetKey() { return m_nKey; }
			uint32_t SetNewKey( uint32_t nNewKey, const char* szNewValue );
			bool empty() { return !m_szValue; }
			void clear();			
		};

		static bool HasTagHead( const wchar_t* szStr )
		{ 
			return szStr && szStr[0] == eHeadTag;
		}

		static bool HasTagHead( const char* szStr )
		{ 
			return szStr &&
				( (uint8_t)szStr[0] ) == eUtf8HeadTag0 &&
				( (uint8_t)szStr[1] ) == eUtf8HeadTag1 &&
				( (uint8_t)szStr[2] ) == eUtf8HeadTag2;
		}

		bool LoadFromDir( const char* szDir );
		bool LoadFromDir( const wchar_t* szDir );

		/** 加载一个字典文件 */
		bool Load( const char* szFileName );
		bool Load( const wchar_t* szFileName );

		/** 保存字典 */
		bool Save();

		/** 字典中是否存在某个key	
		*/
		bool ExistKey( KeyType nKey );

		/** 向字典增加一个value
		@note 失败时返回eInvalidKey
		*/
		KeyType AddValueWithOffset( const wchar_t* szValue, uint32_t nKey = eInvalidKey );
		KeyType AddValueWithOffset( const char* szValue, uint32_t nKey = eInvalidKey );

		void SetOffset( uint32_t nOffset );
		uint32_t GetOffset() const;
		/**	修改某个key对应的value
		@remarks 返回true的前提是：key在字典中存在，value在字典中不存在。
		*/
		bool SetValue( CDictionary::KeyType nKey, const wchar_t* szValue );
		bool SetValue( CDictionary::KeyType nKey, const char* szValue );

		/** 返回某个key对应的value */
		void GetValue( CDictionary::KeyType nKey, std::string& strValue );

		/** 这两个字符串返回NULL标识没找到 */
		const char* GetValue( CDictionary::KeyType nKey );
		const char* GetValue( const char* szKey );

		/** 清空字典 */
		void Clear();
		
		/** 删除一个value */
		void Erase( CDictionary::KeyType nKey );
		void Erase( CDictionary::KeyType nKeyBegin, CDictionary::KeyType nKeyEnd );

		/** 删除一个value */
		void Erase( const char* szKey );

		/** szValue 是否存在非字母字符 */
		bool HasCH( const char* szValue );

		/** szValue 是否是一个合法的可以转换为 KeyType的字符串 */
		bool IsValidKey( const char* szKey );
		
		/** 字符串转换为 KeyType
		@note 失败时返回 eInvalidKey
		*/
		static CDictionary::KeyType StrToKey( const wchar_t* szKey );
		static CDictionary::KeyType StrToKey( const char* szKey );

	protected:
		SDictionaryBuffer* m_pDictionary;

	protected:
		KeyType		GetKey();
		uint8_t		MakeID( KeyType nValue );
	};

	inline void CDictionary::GetValue( CDictionary::KeyType nKey, std::string& strValue )
	{
		strValue = "";
		const char* szValue = GetValue( nKey );
		if( szValue == NULL )
			return;
		strValue = szValue;
	}

	template< class File, class Fun >
	inline void ReadString( File& fileRead, Fun funRead, CDictionary::SLocalizeString& sRead )
	{
		fileRead.Read( &sRead.m_nKey, sizeof( sRead.m_nKey ) );
		sRead.m_szValue = CDictionary::Inst().GetValue( sRead.m_nKey );
	}

	template< class File, class Fun >
	inline void WriteString( File& fileWrite, Fun funWrite, CDictionary::SLocalizeString& sWrite )
	{
		fileWrite.write( &sWrite.m_nKey, sizeof(sWrite.m_nKey) );
	}
}
