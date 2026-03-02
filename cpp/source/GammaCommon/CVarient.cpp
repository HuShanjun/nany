
#include "GammaCommon/CVarient.h"
#include "GammaCommon/TGammaStrStream.h"
#include "GammaCommon/GammaMath.h"
#include "GammaCommon/GammaCodeCvs.h"

namespace Gamma
{
	CVarient::CVarient()
		: m_nCurSize( 0 )
		, m_nMaxSize( nBufSize )
		, m_pMin( NULL )
		, m_pMax( NULL )
    {
        *this = int32();
    }

    CVarient::~CVarient()
    {
		if( m_nMaxSize > nBufSize )
			delete [] m_Data.m_pBuf;
	}

	CVarient::CVarient( const CVarient& a )
		: m_nCurSize( 0 )
		, m_nMaxSize( nBufSize )
		, m_pMin( a.m_pMin ? new CVarient( *a.m_pMin ) : NULL )
		, m_pMax( a.m_pMax ? new CVarient( *a.m_pMax ) : NULL )
	{
		*this = a;
	}

    void CVarient::_Assign( const void* pBuf, size_t size )
    {
		if( size > m_nMaxSize )
		{
			if( m_nMaxSize > nBufSize )
				delete [] m_Data.m_pBuf;
			m_Data.m_pBuf = new tbyte[size];
			m_nMaxSize = size;
		}

		m_nCurSize = size;
		memcpy( _GetBuff(), pBuf, size );
    }

    const CVarient& CVarient::operator= ( SFileData FileData )
    {
        m_eType = eVT_FileName;
        _Assign( FileData.m_szFileName, ( wcslen( FileData.m_szFileName ) + 1 )*sizeof( wchar_t ) );
        return *this;
    }

    const CVarient& CVarient::operator= ( const char* szStr )
    {
        m_eType = eVT_String;
		std::wstring strStr = Utf8ToUcs( szStr );
        _Assign( strStr.c_str(), ( strStr.size() + 1 )*sizeof( wchar_t ) );
        return *this;
	}

	const CVarient& CVarient::operator= ( const wchar_t* szStr )
	{
		m_eType = eVT_String;
		_Assign( szStr, ( wcslen( szStr ) + 1 )*sizeof( wchar_t ) );
		return *this;
	}

    const CVarient& CVarient::operator= ( int32 nValue )
    {
        m_eType = eVT_Integer;
		if( m_pMin )
			nValue = Max( nValue, m_pMin->Int() );
		if( m_pMax )
			nValue = Min( nValue, m_pMax->Int() );
        _Assign( &nValue, sizeof( nValue ) );
        return *this;
    }

    const CVarient& CVarient::operator= ( float fValue )
    {
		m_eType = eVT_Float;
		if( m_pMin )
			fValue = Max( fValue, m_pMin->Float() );
		if( m_pMax )
			fValue = Min( fValue, m_pMax->Float() );
        _Assign( &fValue, sizeof( fValue ) );
        return *this;
	}

	const CVarient& CVarient::operator= ( CVector2f vecValue )
	{
		m_eType = eVT_Vector2;

		if( m_pMin )
		{
			vecValue.x = Max( vecValue.x, m_pMin->Vec2().x );
			vecValue.y = Max( vecValue.y, m_pMin->Vec2().y );
		}

		if( m_pMax )
		{
			vecValue.x = Min( vecValue.x, m_pMax->Vec2().x );
			vecValue.y = Min( vecValue.y, m_pMax->Vec2().y );
		}

		_Assign( &vecValue, sizeof( vecValue ) );
		return *this;
	}

    const CVarient& CVarient::operator= ( CVector3f vecValue )
    {
		m_eType = eVT_Vector3;

		if( m_pMin )
		{
			vecValue.x = Max( vecValue.x, m_pMin->Vec3().x );
			vecValue.y = Max( vecValue.y, m_pMin->Vec3().y );
			vecValue.z = Max( vecValue.z, m_pMin->Vec3().z );
		}

		if( m_pMax )
		{
			vecValue.x = Min( vecValue.x, m_pMax->Vec3().x );
			vecValue.y = Min( vecValue.y, m_pMax->Vec3().y );
			vecValue.z = Min( vecValue.z, m_pMax->Vec3().z );
		}

        _Assign( &vecValue, sizeof( vecValue ) );
        return *this;
	}

	const CVarient& CVarient::operator= ( CVector4f vecValue )
	{
		m_eType = eVT_Vector4;

		if( m_pMin )
		{
			vecValue.x = Max( vecValue.x, m_pMin->Vec4().x );
			vecValue.y = Max( vecValue.y, m_pMin->Vec4().y );
			vecValue.z = Max( vecValue.z, m_pMin->Vec4().z );
			vecValue.w = Max( vecValue.w, m_pMin->Vec4().w );
		}

		if( m_pMax )
		{
			vecValue.x = Min( vecValue.x, m_pMax->Vec4().x );
			vecValue.y = Min( vecValue.y, m_pMax->Vec4().y );
			vecValue.z = Min( vecValue.z, m_pMax->Vec4().z );
			vecValue.w = Min( vecValue.w, m_pMax->Vec4().w );
		}

		_Assign( &vecValue, sizeof( vecValue ) );
		return *this;
	}

    const CVarient& CVarient::operator= ( SComboBoxData comValue )
    {
        m_eType = eVT_ComboBox;
        _Assign( ( (char*)comValue.m_szComboBoxItems ) - sizeof(uint16), 
			( wcslen( comValue.m_szComboBoxItems ) + 1 )*sizeof( wchar_t ) + sizeof(uint16) );

		if( m_pMin )
			comValue.m_nComboBoxIndex = Max( comValue.m_nComboBoxIndex, m_pMin->ComIndex() );
		if( m_pMax )
			comValue.m_nComboBoxIndex = Min( comValue.m_nComboBoxIndex, m_pMax->ComIndex() );

        *(uint16*)( _GetBuff() ) = comValue.m_nComboBoxIndex;
        return *this;
	}

	const CVarient& CVarient::operator= ( SCheckComboBoxData comValue )
	{
		m_eType = eVT_CheckComboBox;
		uint32 nItemCount = 0;
		uint32 nIndex = 0;
		while( comValue.m_szComboBoxItems[nIndex] )
			nItemCount += comValue.m_szComboBoxItems[nIndex++] == ',' ? 1 : 0;
		if( comValue.m_szComboBoxItems[0] )
			nItemCount++;

		std::vector<char> vecBuf;
		vecBuf.resize( ( wcslen( comValue.m_szComboBoxItems ) + 1 )*sizeof( wchar_t ) + nItemCount + sizeof(uint32) );
		char* buf = &vecBuf[0];
		*(uint32*)( buf ) = nItemCount;
		memcpy( buf + sizeof(uint32), comValue.m_aryComboBoxMask, nItemCount );
		memcpy( buf + ( nItemCount + sizeof(uint32) ), comValue.m_szComboBoxItems, ( wcslen( comValue.m_szComboBoxItems ) + 1 )*sizeof( wchar_t ) );
		_Assign( buf, ( wcslen( comValue.m_szComboBoxItems ) + 1 )*sizeof( wchar_t ) + nItemCount + sizeof(uint32) );
		return *this;
	}

    const CVarient& CVarient::operator= ( uint32 nValue )
    {
		m_eType = eVT_UnsignedInteger;
		if( m_pMin )
			nValue = Max( nValue, m_pMin->Uint() );
		if( m_pMax )
			nValue = Min( nValue, m_pMax->Uint() );
        _Assign( &nValue, sizeof( nValue ) );
        return *this;
    }

    const CVarient&  CVarient::operator= ( CColor nValue )
    {
		m_eType = eVT_Color;

		if( m_pMin )
		{
			nValue.r() = Max( nValue.r(), m_pMin->Color().r() );
			nValue.g() = Max( nValue.g(), m_pMin->Color().g() );
			nValue.b() = Max( nValue.b(), m_pMin->Color().b() );
			nValue.a() = Max( nValue.a(), m_pMin->Color().a() );
		}

		if( m_pMax )
		{
			nValue.r() = Min( nValue.r(), m_pMax->Color().r() );
			nValue.g() = Min( nValue.g(), m_pMax->Color().g() );
			nValue.b() = Min( nValue.b(), m_pMax->Color().b() );
			nValue.a() = Min( nValue.a(), m_pMax->Color().a() );
		}

        _Assign( &nValue, sizeof( nValue ) );
        return *this;
    }

	const CVarient& CVarient::operator=  ( SMemory nValue )
	{
		m_eType = eVT_Memory;
		_Assign( nValue.m_pBuf, nValue.m_nSize );
		return *this;
	}

	const CVarient&  CVarient::operator= ( SScaleRange nValue )
	{
		m_eType = eVT_ScaleRange;
		_Assign( &nValue, sizeof( nValue ) );
		return *this;
	}

	const CVarient& CVarient::operator=( bool bValue )
	{
		m_eType = eVT_Boolean;
		_Assign( &bValue, sizeof( bValue ) );
		return *this;
	}

    const CVarient& CVarient::operator= ( const CVarient& a )
    {
        m_eType = a.m_eType;
        _Assign( a._GetBuff(), a.MemSize() );
		SAFE_DELETE( m_pMin );
		SAFE_DELETE( m_pMax );
		if( a.m_pMin )
			m_pMin = new CVarient( *a.m_pMin );
		if( a.m_pMax )
			m_pMax = new CVarient( *a.m_pMax );
        return *this;
    }

    bool CVarient::operator== ( const CVarient& a ) const
    {
		return m_eType == a.m_eType && m_nCurSize == a.m_nCurSize && !memcmp( _GetBuff(), a._GetBuff(), m_nCurSize );
	}

	bool CVarient::operator!= ( const CVarient& a ) const
	{
		return !( (*this) == a );
	}

	wgammasstream& operator << ( wgammasstream& ss, const CVector3f& _Val )
	{
		ss << L"(" << _Val.x << L"," << _Val.y << L"," << _Val.z << L")";
		return ss;
	}

	template<class T>
	const wchar_t* Gamma::CVarient::Convert2String( const T& v ) const
	{
		wchar_t szBuffer[256];
		wgammasstream ss( szBuffer, ELEM_COUNT(szBuffer) );
		ss << v;
		uint32 nLength = (uint32)ss.length();

		if( m_nCurSize + ( nLength + 1 )*sizeof(wchar_t) > m_nMaxSize )
		{
			tbyte* pNewBuffer = new tbyte[m_nCurSize + (nLength + 1)*sizeof(wchar_t)];
			memcpy( pNewBuffer, _GetBuff(), m_nCurSize );
			if( m_nMaxSize > nBufSize )
				delete [] m_Data.m_pBuf;
			const_cast<CVarient*>( this )->m_Data.m_pBuf = pNewBuffer;
			const_cast<CVarient*>( this )->m_nMaxSize = m_nCurSize + ( nLength + 1 )*sizeof(wchar_t);
		}

		memcpy( const_cast<CVarient*>( this )->_GetBuff() + m_nCurSize, szBuffer, ( nLength + 1 )*sizeof(wchar_t) );
		return (const wchar_t*)( _GetBuff() + m_nCurSize );
	}

	const wchar_t* CVarient::Str() const
	{
		if( m_eType == eVT_String || m_eType == eVT_FileName || m_eType == eVT_Memory )
			return (const wchar_t*)_GetBuff();
		if( m_eType == eVT_Integer )
			return Convert2String( Int() );
		if( m_eType == eVT_Float )
			return Convert2String( Float() );
		if( m_eType == eVT_Vector3 )
			return Convert2String( Vec3() );
		if( m_eType == eVT_Color )
			return Convert2String( Color().dwColor );
		if( m_eType == eVT_UnsignedInteger )
			return Convert2String( Uint() );
		if( m_eType == eVT_ScaleRange )
			return Convert2String( Range().m_nCur );
		if( m_eType == eVT_Boolean )
			return Convert2String( Bool() );

		if( m_eType == eVT_ComboBox )
		{
			uint16 nIndex = ComIndex();
			const wchar_t* szItems = ComItems();
			int32 nPreItem = 0;
			int32 nEnd = 0;
			int32 n = 0;
			for( int32 i = 0; szItems[i]; nEnd = ++i )
			{
				if( szItems[i] != L',' )
					continue;
				if( n == nIndex )
					break;
				nPreItem = i + 1;
				++n;
			}
			std::wstring strValue( szItems + nPreItem, nEnd - nPreItem );
			return Convert2String( strValue );
		}

		if( m_eType == eVT_CheckComboBox )
		{
			const tbyte* aryMask = ComMask();
			const wchar_t* szItems = ComItems();
			int32 nPreItem = 0;
			int32 nEnd = 0;
			uint64 n = 0;
			std::wstring strValue;
			for( int32 i = 0; szItems[i]; nEnd = ++i )
			{
				if( szItems[i] != L',' )
					continue;
				if( aryMask[n++] )
				{
					if( !strValue.empty() )
						strValue.push_back( ',' );
					strValue.append( szItems + nPreItem, nEnd - nPreItem );
				}
				nPreItem = i + 1;
			}
			return Convert2String( strValue );
		}

		return NULL;
	}

	int32 CVarient::Int() const
	{
		if( m_eType == eVT_Integer || m_eType == eVT_UnsignedInteger || m_eType == eVT_Color )
			return *( (int32*)_GetBuff() );
		if( m_eType == eVT_ComboBox )
			return *( (uint16*)_GetBuff() );
		if( m_eType == eVT_CheckComboBox )
		{
			uint32 nCount = *( (uint32*)_GetBuff() );
			const tbyte* aryMask = ComMask();
			uint32 nResult = 0;
			for( uint32 i = 0, n = 1; i < nCount; i++, n = n << 1 )
				nResult |= aryMask[i] ? n : 0;
			return (int32)nResult;
		}
		return GammaA2I( Str() );
	}

	float CVarient::Float() const
	{
		if( m_eType == eVT_Float )
			return *( (float*)_GetBuff() );
		return (float)GammaA2F( Str() );
	}

	uint32 CVarient::Uint() const
	{
		if( m_eType == eVT_Integer || m_eType == eVT_UnsignedInteger || m_eType == eVT_Color )
			return *( (uint32*)_GetBuff() );
		return (uint32)Int();
	}

	Gamma::CColor CVarient::Color() const
	{
		if( m_eType == eVT_Color )
			return *( (CColor*)_GetBuff() );
		return Uint();
	}

	Gamma::CVector2f CVarient::Vec2() const
	{
		if( m_eType == eVT_Vector2 )
			return *(CVector2f*)_GetBuff();
		return CVector2f( Float(), 0 );
	}

	Gamma::CVector3f CVarient::Vec3() const
	{
		if( m_eType == eVT_Vector3 )
			return *(CVector3f*)_GetBuff();
		return CVector3f( Vec2(), 0 );
	}

	Gamma::CVector4f CVarient::Vec4() const
	{
		if( m_eType == eVT_Vector4 )
			return *(CVector4f*)_GetBuff();
		return CVector4f( Vec3(), 0 );
	}

	const wchar_t* CVarient::FileName() const
	{
		return Str();
	}

	uint16 CVarient::ComIndex() const
	{
		if( m_eType == eVT_ComboBox )
			return *(uint16*)_GetBuff();
		return (uint16)Int();
	}

	uint32 CVarient::MaskCount() const
	{
		if( m_eType == eVT_CheckComboBox )
			return *(uint32*)_GetBuff();
		return 0;
	}

	const tbyte* CVarient::ComMask() const
	{
		if( m_eType == eVT_CheckComboBox )
			return (const tbyte*)( _GetBuff() + sizeof(uint32) );
		return (const tbyte*)"";
	}

	const wchar_t* CVarient::ComItems() const
	{
		if( m_eType == eVT_ComboBox )
			return (const wchar_t*)( _GetBuff() + sizeof(uint16) );
		if( m_eType == eVT_CheckComboBox )
			return (const wchar_t*)( _GetBuff() + sizeof(uint32) + MaskCount() );
		return L"";
	}

	SScaleRange CVarient::Range() const
	{
		if( m_eType == eVT_ScaleRange )
			return *( (SScaleRange*)_GetBuff() );
		return SScaleRange( Uint() );
	}

	bool CVarient::Bool() const
	{
		if( m_eType == eVT_Boolean )
			return *( (bool*)_GetBuff() );
		return Uint() != 0;
	}

	const void* CVarient::Mem() const
	{
		return _GetBuff();
	}

	size_t CVarient::MemSize() const
	{
		return m_nCurSize;
	}
}
