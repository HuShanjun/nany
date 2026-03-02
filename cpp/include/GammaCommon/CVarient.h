//=====================================================================
// CVarient.h 
// 定义多类型变量
// 柯达昭
// 2007-10-07
//=====================================================================
#ifndef ___GAMMA_CVARIENT_H___
#define ___GAMMA_CVARIENT_H___

#include "GammaCommon/GammaCommonType.h"
#include "GammaCommon/TVector3.h"
#include "GammaCommon/CColor.h"
#include <string.h>
#include <string>

namespace Gamma
{
    struct SFileData
    {
        const wchar_t*	m_szFileName;
        SFileData( const wchar_t* szFileName ) : m_szFileName( szFileName ){}
    };

    //用','分割m_szComboBoxItems
    struct SComboBoxData
    {
        uint16          m_nComboBoxIndex;
        const wchar_t*	m_szComboBoxItems;
        SComboBoxData( const wchar_t* szComboBoxItems, uint16 nComboBoxIndex ) 
            : m_szComboBoxItems( szComboBoxItems ), m_nComboBoxIndex( nComboBoxIndex )
        {}
	};

	//用','分割m_szComboBoxItems
	struct SCheckComboBoxData
	{
		const tbyte*		m_aryComboBoxMask;
		const wchar_t*	m_szComboBoxItems;
		SCheckComboBoxData( const wchar_t* szComboBoxItems, const tbyte* aryComboBoxMask ) 
			: m_szComboBoxItems( szComboBoxItems ), m_aryComboBoxMask( aryComboBoxMask )
		{}
	};

	struct SMemory
	{
		const void* m_pBuf;
		size_t		m_nSize;
		SMemory( const void* pBuf, size_t nSize )
			: m_pBuf( pBuf ), m_nSize( nSize )
		{}
	};

	struct SScaleRange
	{
		uint32 m_nCur;
		uint32 m_nMin;
		uint32 m_nMax;
		SScaleRange( uint32 nCur, uint32 nMin = 0, uint32 nMax = INVALID_32BITID )
			: m_nMin( nMin ), m_nMax( nMax )
		{
			m_nCur = Limit<uint32>( nCur, m_nMin, m_nMax );
		}

		const SScaleRange& operator= ( uint32 nValue )
		{
			m_nCur = Limit<uint32>( nValue, m_nMin, m_nMax );
            return *this;
		}
	};

    // Enumerations
    enum EVarientType
    {
        eVT_String,
        eVT_FileName,
        eVT_Integer,
		eVT_Float,
		eVT_Vector2,
		eVT_Vector3,
		eVT_Vector4,
		eVT_ComboBox,
		eVT_CheckComboBox,
        eVT_Color,
		eVT_UnsignedInteger,
		eVT_Memory,
		eVT_ScaleRange,
		eVT_Boolean,
    };

    class GAMMA_COMMON_API CVarient
    {
		enum { nBufSize = 32 };

		union UVarientData
		{
			tbyte		m_nBuf[nBufSize];
			tbyte*		m_pBuf;
		};

		EVarientType    m_eType;
		size_t			m_nCurSize;
		size_t			m_nMaxSize;
		UVarientData	m_Data;
		CVarient*		m_pMin;
		CVarient*		m_pMax;

		template<class T>
		const wchar_t*	Convert2String( const T& v ) const;

		void			_Assign( const void* pBuf, size_t size );
		const tbyte*	_GetBuff() const { return m_nMaxSize > nBufSize ? m_Data.m_pBuf : m_Data.m_nBuf; }
		tbyte*			_GetBuff() { return m_nMaxSize > nBufSize ? m_Data.m_pBuf : m_Data.m_nBuf; }

		CVarient( const SScaleRange&, const SScaleRange&, const SScaleRange& );
	public:
		void SetType( EVarientType eType ){ m_eType = eType; }

		// Constructor
		~CVarient();
		CVarient();
		CVarient( const CVarient& a );

        template<class T>
		CVarient( const T& a ) 
			: m_nCurSize(0)
			, m_nMaxSize( nBufSize )
			, m_pMin( NULL )
			, m_pMax( NULL )
		{ *this = a; }

		template<class T>
		CVarient( const T& a, const T& min, const T& max ) 
			: m_nCurSize(0)
			, m_nMaxSize( nBufSize )
			, m_pMin( new CVarient( min ) )
			, m_pMax( new CVarient( max ) )
		{ *this = a; }

        const CVarient& operator=  ( SFileData FileData );
		const CVarient& operator=  ( const char* szStr );
		const CVarient& operator=  ( const wchar_t* szStr );
        const CVarient& operator=  ( int32 nValue );
		const CVarient& operator=  ( float fValue );
		const CVarient& operator=  ( CVector2f vecValue );
		const CVarient& operator=  ( CVector3f vecValue );
		const CVarient& operator=  ( CVector4f vecValue );
		const CVarient& operator=  ( SComboBoxData comValue );
		const CVarient& operator=  ( SCheckComboBoxData comValue );
		const CVarient& operator=  ( uint32 nValue );
		const CVarient& operator=  ( CColor nValue );
		const CVarient& operator=  ( SMemory nValue );
		const CVarient& operator=  ( SScaleRange nValue );
		const CVarient& operator=  ( bool bValue );
		const CVarient& operator=  ( const CVarient& a );
		bool            operator== ( const CVarient& a ) const;
		bool            operator!= ( const CVarient& a ) const;

        EVarientType    Type()const     { return m_eType; }
        const wchar_t*	Str()const;
		int32           Int()const;
		float           Float()const;
		uint32          Uint()const;
		CColor          Color()const;
		CVector2f       Vec2()const;
		CVector3f       Vec3()const;
		CVector4f       Vec4()const;
        const wchar_t*	FileName()const;
		uint16          ComIndex()const;
		uint32			MaskCount()const;
		const tbyte*    ComMask()const;
        const wchar_t*	ComItems()const;
		const void*		Mem()const;
		size_t			MemSize()const;
		SScaleRange		Range()const;
		bool			Bool()const;
    };

    class CVarientEx : public CVarient
    {
        wchar_t m_szName[32];
    public:
        CVarientEx(){}

        template<class T>
        CVarientEx( const wchar_t* szVarientName, const T& a ) 
            : CVarient( a )
        {
            size_t nLen = wcslen( szVarientName );
            if( nLen > 31 )
            	nLen = 31;
            memcpy( m_szName, szVarientName, nLen*sizeof(wchar_t) );
            m_szName[nLen] = 0;
		}

		template<class T>
		CVarientEx( const wchar_t* szVarientName, const T& a, const T& min, const T& max ) 
			: CVarient( a, min, max )
		{
			size_t nLen = wcslen( szVarientName );
			if( nLen > 31 )
				nLen = 31;
			memcpy( m_szName, szVarientName, nLen*sizeof(wchar_t) );
			m_szName[nLen] = 0;
		}

        CVarientEx( const CVarientEx& a ) 
            : CVarient( a )
        {
            memcpy( m_szName, a.m_szName, sizeof(m_szName) );
        }

        bool operator== ( const CVarientEx& a ) const
        {
            if( wcscmp( m_szName, a.m_szName ) )
                return false;
            return CVarient::operator==( a );
        }

        bool operator!= ( const CVarientEx& a ) const
        {
            return !( *this == a );
        }

        std::wstring GetName()const    { return m_szName; }
    };

}

#endif
