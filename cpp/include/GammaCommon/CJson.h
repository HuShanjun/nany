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
		uint32				m_nContentLen;
		uint32				m_nLevel;
		uint32				m_nChildCount;
		bool				m_bForceString;

		friend class TGammaList<CJson>;

		bool NextNode( size_t& nCurPos, bool bWithName );
		char GetString( size_t& nCurPos, bool bBlankEnd, uint32* pLen );
		bool GetNumber( size_t& nCurPos );
		void OutContent( std::ostream& os ) const;
		void SetLevel( uint32 nLevel );

		bool ParseArray( CRefStringPtr& DomXmlBuffer, size_t& nCurPos );
		bool ParseObject( CRefStringPtr& DomXmlBuffer, size_t& nCurPos );
		bool Parse( CRefStringPtr& DomXmlBuffer, size_t& nCurPos, bool bWithName );
	public:
		CJson( const char* szName = NULL, const char* szContent = NULL );
		CJson( const CJson& rhs );
		~CJson();

		const CJson& operator= ( const CJson& rhs );

		bool Load( const void* szBuffer, uint32 nSize );
		bool Load( const wchar_t* szFileName );
		bool Load( const char* szFileName );
		bool Save( const wchar_t* szFileName ) const;
		bool Save( const char* szFileName ) const;
		bool Save( std::ostream& os, uint32 nStack = 0 ) const;
		void Clear();

		CJson* AddChild( CJson* pChild, CJson* pBefore = NULL );
		CJson* AddChild( const char* szName, CJson* pBefore = NULL );
		template <typename ValueType>
		CJson* AddChild( const char* szName, ValueType Value, CJson* pBefore = NULL );

		CJson* GetChild( uint32 nChildIndex );
		CJson* GetChild( const char* szChildName );
		CJson* operator[]( uint32 nChildIndex );
		CJson* operator[]( const char* szChildName );
		CJson* GetNext();
		CJson* GetPre();

		uint32 GetChildCount() const;
		const CJson* GetChild( uint32 nChildIndex ) const;
		const CJson* GetChild( const char* szChildName ) const;
		const CJson* operator[]( uint32 nChildIndex ) const;
		const CJson* operator[]( const char* szChildName ) const;
		const CJson* GetNext() const;
		const CJson* GetPre() const;

		const char* GetName() const;
		const char* GetValue() const;
		const char* GetContent() const;
		uint32 GetContentLen() const;
		bool IsForceString() const;
		void ForceString( bool bForce );

		template <typename ValueType>
		ValueType As(ValueType Value = ValueType()) const;

		template <typename ValueType>
		ValueType At( const char* szChildName, ValueType eDefaultValue = ValueType() ) const;

		template <typename ValueType>
		ValueType At( uint32 nChildIndex, ValueType eDefaultValue = ValueType() ) const;
	};

	template <typename ValueType>
	inline ValueType CJson::At( const char* szChildName, ValueType eDefaultValue ) const
	{
		const CJson* pChild = GetChild( szChildName );
		return pChild ? pChild->As<ValueType>() : eDefaultValue;
	}

	template <typename ValueType>
	inline ValueType CJson::At( uint32 nChildIndex, ValueType eDefaultValue ) const
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
	inline uint8 CJson::As<uint8>(uint8 Value) const
	{
		if (this == m_pInvalid)
			return Value;
		return (uint8)GammaA2I( m_szContent );
	}

	template<>
	inline int8 CJson::As<int8>(int8 Value) const
	{
		if (this == m_pInvalid)
			return Value;
		return (int8)GammaA2I( m_szContent );
	}

	template<>
	inline uint16 CJson::As<uint16>(uint16 Value) const
	{
		if (this == m_pInvalid)
			return Value;
		return (uint16)GammaA2I( m_szContent );
	}

	template<>
	inline int16 CJson::As<int16>(int16 Value) const
	{
		if (this == m_pInvalid)
			return Value;
		return (int16)GammaA2I( m_szContent );
	}

	template<>
	inline uint32 CJson::As<uint32>(uint32 Value) const
	{
		if (this == m_pInvalid)
			return Value;
		return (uint32)GammaA2I( m_szContent );
	}

	template<>
	inline int32 CJson::As<int32>(int32 Value) const
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
	inline uint64 CJson::As<uint64>(uint64 Value) const
	{
		if (this == m_pInvalid)
			return Value;
		return (uint64)GammaA2I64( m_szContent );
	}

	template<>
	inline int64 CJson::As<int64>(int64 Value) const
	{
		if (this == m_pInvalid)
			return Value;
		return (int64)GammaA2I64( m_szContent );
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
		return As<int32>() != 0;
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
