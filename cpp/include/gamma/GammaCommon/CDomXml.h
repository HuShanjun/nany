/*---------------------------------------------------------------------------
2005-12-16 柯达昭
完成制表符分割文件的读取，制表文件是一个特殊格式的文本文件，将Excel编辑的制表
保存为文本文件即可，制表符文件每行由回车换行符分隔(0x0d 0x0a)，每个单元之间由
制表符（TAB）（0x09）分隔.
//---------------------------------------------------------------------------*/
#pragma once
#include "GammaCommonType.h"
#include "GammaCommon/GammaMath.h"
#include "GammaCommon/CPkgFile.h"
#include "GammaCommon/TGammaList.h"
#include "GammaCommon/TTinyList.h"
#include "GammaCommon/TRefString.h"
#include "GammaCommon/TSmartPtr.h"

#include <sstream>
#include <string>
#include <list>

namespace Gamma
{	
	class CDomXmlDocument;
	class CDomXmlAttribute;

#ifdef _WIN32
	template class GAMMA_COMMON_API TGammaList<CDomXmlDocument>::CGammaListNode;
	template class GAMMA_COMMON_API TGammaList<CDomXmlDocument>;
	template class GAMMA_COMMON_API TGammaList<CDomXmlAttribute>::CGammaListNode;
	template class GAMMA_COMMON_API TGammaList<CDomXmlAttribute>;
	template class GAMMA_COMMON_API TSmartPtr< TRefString<char> >;
#endif

	class GAMMA_COMMON_API CDomXmlAttribute
		: private TGammaList<CDomXmlAttribute>::CGammaListNode
	{
		CRefStringPtr				m_ptrBuffer;
		const char*					m_szName;
		const char*					m_szContent;

		friend class CDomXmlDocument;
		friend class TGammaList<CDomXmlAttribute>;

		bool Parse( CRefStringPtr& DomXmlBuffer, size_t& nCurPos );
	public:
		CDomXmlAttribute();
		~CDomXmlAttribute();

		const char* GetName() const;
		const char* GetValue() const;
		void SetValue( const char* szValue );
		CDomXmlAttribute* GetNextSibling();
		const CDomXmlAttribute* GetNextSibling() const;

		template <typename ValueType>
		ValueType As() const;
	};

	template<>
	inline char CDomXmlAttribute::As<char>() const
	{
		return m_szContent[0];
	}

	template<>
	inline uint8_t CDomXmlAttribute::As<uint8_t>() const
	{
		return (uint8_t)GammaA2I( m_szContent );
	}

	template<>
	inline int8_t CDomXmlAttribute::As<int8_t>() const
	{
		return (int8_t)GammaA2I( m_szContent );
	}

	template<>
	inline uint16_t CDomXmlAttribute::As<uint16_t>() const
	{
		return (uint16_t)GammaA2I( m_szContent );
	}

	template<>
	inline int16_t CDomXmlAttribute::As<int16_t>() const
	{
		return (int16_t)GammaA2I( m_szContent );
	}

	template<>
	inline uint32_t CDomXmlAttribute::As<uint32_t>() const
	{
		return (uint32_t)GammaA2I( m_szContent );
	}

	template<>
	inline int32_t CDomXmlAttribute::As<int32_t>() const
	{
		return GammaA2I( m_szContent );
	}

	template<>
	inline float CDomXmlAttribute::As<float>() const
	{
		return (float)GammaA2F( m_szContent );
	}

	template<>
	inline uint64_t CDomXmlAttribute::As<uint64_t>() const
	{
		return (uint64_t)GammaA2I64( m_szContent );
	}

	template<>
	inline int64_t CDomXmlAttribute::As<int64_t>() const
	{
		return (int64_t)GammaA2I64( m_szContent );
	}

	template<>
	inline bool CDomXmlAttribute::As<bool>() const
	{
		if(
			(m_szContent[0]=='t' || m_szContent[0]=='T')  &&
			(m_szContent[1]=='r' || m_szContent[1]=='R')  &&
			(m_szContent[2]=='u' || m_szContent[2]=='U')  &&
			(m_szContent[3]=='e' || m_szContent[3]=='E') )
			return true;
		return As<int32_t>() != 0;
	}

	template<>
	inline std::string CDomXmlAttribute::As<std::string>() const
	{
		return GetValue();
	}

	template<>
	inline const char* CDomXmlAttribute::As<const char*>() const
	{
		return GetValue();
	}

	class GAMMA_COMMON_API CDomXmlDocument 
		: private TGammaList<CDomXmlDocument>::CGammaListNode
		, private TGammaList<CDomXmlDocument>
		, private TGammaList<CDomXmlAttribute>
	{
		CDomXmlDocument*			m_pParent;
		CRefStringPtr				m_ptrBuffer;
		const char*					m_szName;
		const char*					m_szContent;
		uint32_t						m_nLevel;
		uint32_t						m_nChildCount;

		friend class CDomXmlAttribute;
		friend class TGammaList<CDomXmlDocument>;

		bool FindNextNode( CRefStringPtr& DomXmlBuffer, size_t& nCurPos );
		bool FindNextAttribute( CRefStringPtr& DomXmlBuffer, size_t& nCurPos );
		bool Parse( CRefStringPtr& DomXmlBuffer, size_t& nCurPos );
		bool AddAttribute( CRefStringPtr& DomXmlBuffer, size_t& nCurPos );

	public:
		CDomXmlDocument( const char* szName = NULL );
		CDomXmlDocument( const CDomXmlDocument& rhs );
		~CDomXmlDocument();

		const CDomXmlDocument& operator= ( const CDomXmlDocument& rhs );

		bool Load( const wchar_t* szFileName );
		bool Load( const char* szFileName );
		bool LoadFromBuffer( const char* pBuffer, size_t nCount = -1 );
		bool Save( const wchar_t* szFileName ) const;
		bool Save( std::ostream& os, uint32_t nStack = 0 ) const;
		void clear();

		CDomXmlDocument* InsertNodeFirst( const char* szName );
		CDomXmlDocument* InsertNodeLast( const char* szName );
		void RemoveNode( const char* szName );

		CDomXmlAttribute* InsertAttributeFirst( const char* szName, const char* szValue );
		CDomXmlAttribute* InsertAttributeLast( const char* szName, const char* szValue );
		CDomXmlAttribute* InsertAttributeFirst( const char* szName, bool bValue );
		CDomXmlAttribute* InsertAttributeLast( const char* szName, bool bValue );
		void RemoveAttribute( const char* szName );

		CDomXmlAttribute* GetFirstAttribute();
		CDomXmlAttribute* GetAttribute( const char* szName );
		CDomXmlAttribute* GetAttribute( size_t nAttributeIndex );
		CDomXmlDocument* GetChild( const char* szName );
		CDomXmlDocument* GetFirstChild();
		CDomXmlDocument* GetNextSibling();

		const CDomXmlAttribute* GetFirstAttribute() const;
		const CDomXmlAttribute* GetAttribute( const char* szName ) const;
		const CDomXmlAttribute* GetAttribute( size_t nAttributeIndex ) const;
		const CDomXmlDocument* GetChild( const char* szName ) const;
		const CDomXmlDocument* GetFirstChild() const;
		const CDomXmlDocument* GetNextSibling() const;

		const char* GetName() const;
		const char*	GetText() const;
		uint32_t Count() const;

		CDomXmlDocument& operator()( size_t nChildIndex );
		CDomXmlDocument& operator()( const char* szChildName );
		CDomXmlAttribute& operator[]( size_t nAttributeIndex );
		CDomXmlAttribute& operator[]( const char* szAttributeName );

		const CDomXmlDocument& operator()( size_t nChildIndex ) const;
		const CDomXmlDocument& operator()( const char* szChildName ) const;
		const CDomXmlAttribute& operator[]( size_t nAttributeIndex ) const;
		const CDomXmlAttribute& operator[]( const char* szAttributeName ) const;

		template <typename ValueType>
		ValueType At( size_t nAttributeIndex, ValueType eDefaultValue = ValueType() ) const;
		template <typename ValueType>
		ValueType At( const char* szAttributeName, ValueType eDefaultValue = ValueType() ) const;

		template<typename type>
		CDomXmlAttribute* InsertAttributeFirst( const char* szName, type nValue );
		template<typename type>
		CDomXmlAttribute* InsertAttributeLast( const char* szName, type nValue );
	};

	template <typename ValueType>
	inline ValueType CDomXmlDocument::At( size_t nAttributeIndex, ValueType eDefaultValue ) const
	{
		const CDomXmlAttribute* pAttribute = GetAttribute( nAttributeIndex );
		return pAttribute ? pAttribute->As<ValueType>() : eDefaultValue;
	}

	template <typename ValueType>
	inline ValueType CDomXmlDocument::At( const char* szChildName, ValueType eDefaultValue ) const
	{
		const CDomXmlAttribute* pAttribute = GetAttribute( szChildName );
		return pAttribute ? pAttribute->As<ValueType>() : eDefaultValue;
	}

	template<typename type>
	inline CDomXmlAttribute* CDomXmlDocument::InsertAttributeFirst( const char* szName, type value )
	{
		char szBuff[256] = { 0 };
		gammasstream logstr( szBuff, 256 );
		logstr << value;
		return InsertAttributeFirst( szName, (const char*)szBuff );
	}

	template<typename type>
	inline CDomXmlAttribute* CDomXmlDocument::InsertAttributeLast( const char* szName, type value )
	{
		char szBuff[256] = { 0 };
		gammasstream logstr( szBuff, 256 );
		logstr << value;
		return InsertAttributeLast( szName, (const char*)szBuff );
	}
}
