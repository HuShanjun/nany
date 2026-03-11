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
				uint16_t	m_nBuild;
				uint16_t	m_nRevision;
				uint16_t	m_nMinor;
				uint8_t	m_nMajor;
				uint8_t	m_nReserve;
			};
			uint64_t		m_nVersion;
		};

	public:
		CVersion( uint8_t nReserve = 0, uint8_t nMajor = 0, uint16_t nMinor = 0, uint16_t nRevision = 0, uint16_t nBuild = 0 )
			: m_nReserve( nReserve ), m_nMajor( nMajor ), m_nMinor( nMinor ), m_nRevision( nRevision ), m_nBuild( nBuild )
		{
		}

		CVersion( const wchar_t* szVersion )
		{
			int32_t nNumber[eVF_Count] = { 0, 0, 0, 0, 0 };
			std::vector<std::wstring> vecVersion = SeparateString( szVersion, L'.' );
			for( size_t i = 0; i < vecVersion.size() && i < eVF_Count; i++ )
				nNumber[i] = GammaA2I( vecVersion[i].c_str() );
			*this = CVersion( (uint8_t)nNumber[0], (uint8_t)nNumber[1], (uint16_t)nNumber[2], (uint16_t)nNumber[3], (uint16_t)nNumber[4] );
		}

		CVersion( const char* szVersion )
		{
			int32_t nNumber[eVF_Count] = { 0, 0, 0, 0, 0 };
			std::pair<const char*, uint32_t> aryResult[eVF_Count];
			size_t nCount = SeparateStringFast( szVersion, '.', aryResult, eVF_Count );
			for( size_t i = 0; i < nCount; i++ )
			{
				char szBuffer[64];
				strncpy2array_safe( szBuffer, aryResult[i].first, aryResult[i].second );
				nNumber[i] = GammaA2I( szBuffer );
			}
			*this = CVersion( (uint8_t)nNumber[0], (uint8_t)nNumber[1], (uint16_t)nNumber[2], (uint16_t)nNumber[3], (uint16_t)nNumber[4] );
		}

		CVersion( uint64_t nVersion )
			: m_nVersion( nVersion )
		{
		}

		template<typename CharType>
		operator std::basic_string<CharType>() const
		{
			CharType szBuf[64];
			TGammaStrStream<CharType> ss( szBuf, 64 );
			ss << (uint32_t)m_nReserve << "." << (uint32_t)m_nMajor << "." << (uint32_t)m_nMinor;
			if( m_nBuild )
				ss << "." << (uint32_t)m_nRevision << "." << (uint32_t)m_nBuild;
			else if( m_nRevision )
				ss << "." << (uint32_t)m_nRevision;
			return szBuf;
		}

		operator uint64_t()		const { return m_nVersion;						}
		uint16_t GetBuild()		const { return m_nBuild;						}
		uint16_t GetRevision()	const { return m_nRevision;						}
		uint16_t GetMinor()		const { return m_nMinor;						}
		uint16_t GetMajor()		const { return m_nMajor;						}
		uint16_t GetReserve()		const { return m_nReserve;						}
		uint32_t GetHigh32()		const { return (uint32_t)( m_nVersion >> 32 );	}
		uint32_t GetLow32()		const { return (uint32_t)( m_nVersion );			}

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
			for( uint32_t i = 0; i < vecString.size(); i++ )
			{
				std::string& strVersion = vecString[i];
				if( strVersion.empty() )
					continue;

				CVersion verBegin;
				CVersion verLast( static_cast<uint64_t>(INVALID_64BITID) );
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
