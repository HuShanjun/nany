//=====================================================================
// CJson.h 
// 解释Json格式
// 柯达昭
// 2007-09-15
//=======================================================================
#ifndef _JSON_H_
#define _JSON_H_

#include "GammaCommonType.h"
#include "GammaCommon/CGammaObject.h"
#include "GammaCommon/GammaMath.h"
#include "GammaCommon/CPkgFile.h"
#include "GammaCommon/TSmartPtr.h"
#include "GammaCommon/TGammaList.h"
#include "GammaCommon/TTinyList.h"
#include "GammaCommon/TRefString.h"

namespace Gamma
{
	class CJson;

#ifdef _WIN32
	template class GAMMA_COMMON_API TGammaList<CJson>::CGammaListNode;
	template class GAMMA_COMMON_API TGammaList<CJson>;
	template class GAMMA_COMMON_API TSmartPtr< TRefString<char> >;
#endif

	class GAMMA_COMMON_API CJson
		: public CGammaObject
		, private TGammaList<CJson>
		, private TGammaList<CJson>::CGammaListNode
	{
		friend class TGammaList<CJson>;
		friend class TGammaList<CJson>::CGammaListNode;
		static volatile CJson*		m_pInvalid;

		CJson*				m_pParent;
		CRefStringPtr		m_ptrBuffer;
		const char*			m_szName;
		const char*			m_szContent;
		uint32_t				m_nContentLen;
		uint32_t				m_nLevel;
		uint32_t				m_nChildCount;
		bool				m_bForceString;

		friend class TGammaList<CJson>;

		bool NextNode( size_t& nCurPos, bool bWithName );
		char GetString( size_t& nCurPos, bool bBlankEnd, uint32_t* pLen );
		bool GetNumber( size_t& nCurPos );
		void OutContent( std::ostream& os ) const;
		void SetLevel( uint32_t nLevel );

		bool ParseArray( CRefStringPtr& DomXmlBuffer, size_t& nCurPos );
		bool ParseObject( CRefStringPtr& DomXmlBuffer, size_t& nCurPos );
		bool Parse( CRefStringPtr& DomXmlBuffer, size_t& nCurPos, bool bWithName );
	public:
		CJson( const char* szName = NULL, const char* szContent = NULL );
		CJson( const CJson& rhs );
		~CJson();

		const CJson& operator= ( const CJson& rhs );

		bool Load( const void* szBuffer, uint32_t nSize );
		bool Load( const wchar_t* szFileName );
		bool Load( const char* szFileName );
		bool Save( const wchar_t* szFileName ) const;
		bool Save( const char* szFileName ) const;
		bool Save( std::ostream& os, uint32_t nStack = 0 ) const;
		void Clear();

		CJson* AddChild( CJson* pChild, CJson* pBefore = NULL );
		CJson* AddChild( const char* szName, CJson* pBefore = NULL );
		template <typename ValueType>
		CJson* AddChild( const char* szName, ValueType Value, CJson* pBefore = NULL );

		CJson* GetChild( uint32_t nChildIndex );
		CJson* GetChild( const char* szChildName );
		CJson* operator[]( uint32_t nChildIndex );
		CJson* operator[]( const char* szChildName );
		CJson* GetNext();
		CJson* GetPre();

		uint32_t GetChildCount() const;
		const CJson* GetChild( uint32_t nChildIndex ) const;
		const CJson* GetChild( const char* szChildName ) const;
		const CJson* operator[]( uint32_t nChildIndex ) const;
		const CJson* operator[]( const char* szChildName ) const;
		const CJson* GetNext() const;
		const CJson* GetPre() const;

		const char* GetName() const;
		const char* GetValue() const;
		const char* GetContent() const;
		uint32_t GetContentLen() const;
		bool IsForceString() const;
		void ForceString( bool bForce );

		template <typename ValueType>
		ValueType As(ValueType Value = ValueType()) const;

		template <typename ValueType>
		ValueType At( const char* szChildName, ValueType eDefaultValue = ValueType() ) const;

		template <typename ValueType>
		ValueType At( uint32_t nChildIndex, ValueType eDefaultValue = ValueType() ) const;
	};

	template <typename ValueType>
	inline ValueType CJson::At( const char* szChildName, ValueType eDefaultValue ) const
	{
		const CJson* pChild = GetChild( szChildName );
		return pChild ? pChild->As<ValueType>() : eDefaultValue;
	}

	template <typename ValueType>
	inline ValueType CJson::At( uint32_t nChildIndex, ValueType eDefaultValue ) const
	{
		const CJson* pChild = GetChild( nChildIndex );
		return pChild ? pChild->As<ValueType>() : eDefaultValue;
	}

	template<>
	inline char CJson::As<char>(char Value) const
	{
		if (this == m_pInvalid)
			return Value;
		return m_szContent[0];
	}

	template<>
	inline uint8_t CJson::As<uint8_t>(uint8_t Value) const
	{
		if (this == m_pInvalid)
			return Value;
		return (uint8_t)GammaA2I( m_szContent );
	}

	template<>
	inline int8_t CJson::As<int8_t>(int8_t Value) const
	{
		if (this == m_pInvalid)
			return Value;
		return (int8_t)GammaA2I( m_szContent );
	}

	template<>
	inline uint16_t CJson::As<uint16_t>(uint16_t Value) const
	{
		if (this == m_pInvalid)
			return Value;
		return (uint16_t)GammaA2I( m_szContent );
	}

	template<>
	inline int16_t CJson::As<int16_t>(int16_t Value) const
	{
		if (this == m_pInvalid)
			return Value;
		return (int16_t)GammaA2I( m_szContent );
	}

	template<>
	inline uint32_t CJson::As<uint32_t>(uint32_t Value) const
	{
		if (this == m_pInvalid)
			return Value;
		return (uint32_t)GammaA2I( m_szContent );
	}

	template<>
	inline int32_t CJson::As<int32_t>(int32_t Value) const
	{
		if (this == m_pInvalid)
			return Value;
		return GammaA2I( m_szContent );
	}

	template<>
	inline float CJson::As<float>(float Value) const
	{
		if (this == m_pInvalid)
			return Value;
		return (float)GammaA2F( m_szContent );
	}

	template<>
	inline uint64_t CJson::As<uint64_t>(uint64_t Value) const
	{
		if (this == m_pInvalid)
			return Value;
		return (uint64_t)GammaA2I64( m_szContent );
	}

	template<>
	inline int64_t CJson::As<int64_t>(int64_t Value) const
	{
		if (this == m_pInvalid)
			return Value;
		return (int64_t)GammaA2I64( m_szContent );
	}

	template<>
	inline bool CJson::As<bool>(bool Value) const
	{
		if (this == m_pInvalid)
			return Value;
		if(
			(m_szContent[0]=='t' || m_szContent[0]=='T')  &&
			(m_szContent[1]=='r' || m_szContent[1]=='R')  &&
			(m_szContent[2]=='u' || m_szContent[2]=='U')  &&
			(m_szContent[3]=='e' || m_szContent[3]=='E') )
			return true;
		return As<int32_t>() != 0;
	}

	template<>
	inline std::string CJson::As<std::string>(std::string Value) const
	{
		if (this == m_pInvalid)
			return Value;
		return GetValue();
	}

	template<>
	inline const char* CJson::As<const char*>(const char* Value) const
	{
		if (this == m_pInvalid)
			return Value;
		return GetValue();
	}

	template<typename ValueType>
	inline CJson* CJson::AddChild( const char* szName, ValueType Value, CJson* pBefore /*= NULL */ )
	{
		char szValue[1024];
		return AddChild( szName, ( gammasstream( szValue ) << Value ).GetBuff(), pBefore );
	}

	template<>
	inline CJson*  CJson::AddChild( const char* szName, const char* szValue, CJson* pBefore )
	{
		return AddChild( new CJson( szName, szValue ), pBefore );
	}

	template<>
	inline CJson*  CJson::AddChild( const char* szName, const std::string& strValue, CJson* pBefore )
	{
		return AddChild( szName, strValue.c_str(), pBefore );
	}

	template<>
	inline CJson*  CJson::AddChild( const char* szName, bool bValue, CJson* pBefore )
	{
		return AddChild( szName, bValue ? "true" : "false", pBefore );
	}
}


#endif // _JSON_H_
