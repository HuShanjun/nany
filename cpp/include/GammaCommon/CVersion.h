//===============================================
// CVersion.h 
// 定义版本号操作
// 柯达昭
// 2009-02-19
//===============================================

#ifndef __GAMMA_VERSION_H__
#define __GAMMA_VERSION_H__
#pragma warning(disable: 4201)
#include <string>
#include <vector>
#include <map>
#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/GammaMath.h"
#include "GammaCommon/TGammaStrStream.h"

namespace Gamma
{
	enum EVersionField
	{
		eVF_Build		= 0,
		eVF_Revision	,
		eVF_Minor		,
		eVF_Major		,
		eVF_Reserve		,
		eVF_Count		,
	};

	class CVersion
	{
		union
		{
			struct  
			{
				uint16	m_nBuild;
				uint16	m_nRevision;
				uint16	m_nMinor;
				uint8	m_nMajor;
				uint8	m_nReserve;
			};
			uint64		m_nVersion;
		};

	public:
		CVersion( uint8 nReserve = 0, uint8 nMajor = 0, uint16 nMinor = 0, uint16 nRevision = 0, uint16 nBuild = 0 )
			: m_nReserve( nReserve ), m_nMajor( nMajor ), m_nMinor( nMinor ), m_nRevision( nRevision ), m_nBuild( nBuild )
		{
		}

		CVersion( const wchar_t* szVersion )
		{
			int32 nNumber[eVF_Count] = { 0, 0, 0, 0, 0 };
			std::vector<std::wstring> vecVersion = SeparateString( szVersion, L'.' );
			for( size_t i = 0; i < vecVersion.size() && i < eVF_Count; i++ )
				nNumber[i] = GammaA2I( vecVersion[i].c_str() );
			*this = CVersion( (uint8)nNumber[0], (uint8)nNumber[1], (uint16)nNumber[2], (uint16)nNumber[3], (uint16)nNumber[4] );
		}

		CVersion( const char* szVersion )
		{
			int32 nNumber[eVF_Count] = { 0, 0, 0, 0, 0 };
			std::pair<const char*, uint32> aryResult[eVF_Count];
			size_t nCount = SeparateStringFast( szVersion, '.', aryResult, eVF_Count );
			for( size_t i = 0; i < nCount; i++ )
			{
				char szBuffer[64];
				strncpy2array_safe( szBuffer, aryResult[i].first, aryResult[i].second );
				nNumber[i] = GammaA2I( szBuffer );
			}
			*this = CVersion( (uint8)nNumber[0], (uint8)nNumber[1], (uint16)nNumber[2], (uint16)nNumber[3], (uint16)nNumber[4] );
		}

		CVersion( uint64 nVersion )
			: m_nVersion( nVersion )
		{
		}

		template<typename CharType>
		operator std::basic_string<CharType>() const
		{
			CharType szBuf[64];
			TGammaStrStream<CharType> ss( szBuf, 64 );
			ss << (uint32)m_nReserve << "." << (uint32)m_nMajor << "." << (uint32)m_nMinor;
			if( m_nBuild )
				ss << "." << (uint32)m_nRevision << "." << (uint32)m_nBuild;
			else if( m_nRevision )
				ss << "." << (uint32)m_nRevision;
			return szBuf;
		}

		operator uint64()		const { return m_nVersion;						}
		uint16 GetBuild()		const { return m_nBuild;						}
		uint16 GetRevision()	const { return m_nRevision;						}
		uint16 GetMinor()		const { return m_nMinor;						}
		uint16 GetMajor()		const { return m_nMajor;						}
		uint16 GetReserve()		const { return m_nReserve;						}
		uint32 GetHigh32()		const { return (uint32)( m_nVersion >> 32 );	}
		uint32 GetLow32()		const { return (uint32)( m_nVersion );			}

        /// xo lua用的比较
        bool Less( const CVersion& v ) const { return m_nVersion < v.m_nVersion; }
        bool Equal( const CVersion& v ) const { return m_nVersion == v.m_nVersion; }
		bool Greater( const CVersion& v ) const { return m_nVersion > v.m_nVersion; }
		void FromString( const char* szVersion ){ *this = CVersion( szVersion );	}
	};

	class CMultVersion
	{
		std::map<CVersion, CVersion>	m_mapVersion;
	public:
		CMultVersion( const char* szVersions )	{ FromString( szVersions );	}
		CMultVersion( const CVersion& Versions ){ m_mapVersion[Versions] = Versions;	}

		void FromString( const char* szVersions )
		{
			if( !szVersions || !szVersions[0] )
			{
				m_mapVersion.clear();
				return;
			}

			std::vector<std::string> vecString = SeparateString( szVersions, ',' );
			for( uint32 i = 0; i < vecString.size(); i++ )
			{
				std::string& strVersion = vecString[i];
				if( strVersion.empty() )
					continue;

				CVersion verBegin;
				CVersion verLast( INVALID_64BITID );
				std::string::size_type nPos = strVersion.find( '-' );
				if( nPos == std::string::npos )
				{
					verBegin.FromString( strVersion.c_str() );
					verLast = verBegin;
				}
				else
				{
					strVersion[nPos] = 0;
					if( nPos )
						verBegin.FromString( strVersion.c_str() );
					if( nPos + 1 < strVersion.size() )
						verLast.FromString( strVersion.c_str() + nPos + 1 );
				}
			}
		}

		bool operator[]( const CVersion& Version ) const
		{
			std::map<CVersion, CVersion>::const_iterator it = m_mapVersion.upper_bound( Version );
			if( it == m_mapVersion.begin() )
				return false;
			--it;
			return Version >= it->first && Version <= it->second;
		}
	};
}
#endif
