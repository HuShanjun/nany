//=====================================================================
// GammaPackageCommon.h 
// 定义兼容从文件包读取文件以及直接读取系统文件的引擎公共信息
// 柯达昭
// 2011-04-26
//=======================================================================
#pragma once

#include "GammaCommon/GammaCommonType.h"
#include <string>

#define MAX_CONFLICT_CHECK	16

namespace Gamma
{
	struct SPkgNameHeader
	{
		enum{ eFileEncript = 0x1, eFileCompressed = 0x2 };
		uint32 m_nSubPkgzSize;
		uint32 m_nFileNum;
		uint32 m_nDataSize;
		uint32 m_nAttr;

		SPkgNameHeader()
		{
			memset( this, 0, sizeof(SPkgNameHeader) );
		}	
		
		bool IsCompressed() const
		{ return !!( m_nAttr & eFileCompressed ); }

		void SetCompressed( bool bCompressed )
		{
			m_nAttr = bCompressed ? m_nAttr | eFileCompressed : m_nAttr &~eFileCompressed;
		}

		bool IsEncripted() const
		{ return !!( m_nAttr & eFileEncript ); }

		void SetEncript( bool bEncript )
		{
			m_nAttr = bEncript ? m_nAttr | eFileEncript : m_nAttr &~eFileEncript;
		}
	};

	struct SMainData
	{
		enum{ eFileCompletely = 0x1, eFileCompressed = 0x2, MAX_SUB_PKGZ_SIZE = 64 * 1024 * 2  };
		uint32 m_nMainDataOffset;
		uint32 m_nMainDataSize;
		uint32 m_nMainDataOffsetCompress;
		uint32 m_nMainDataSizeCompress;
		uint32 m_Hash;
		uint32 m_nState;
		void SetCompletely( bool bCompletely )
		{
			m_nState = bCompletely ? m_nState | eFileCompletely 
				: m_nState &~eFileCompletely;
		}

		bool IsCompletely() const
		{ 
			return !!( m_nState & eFileCompletely ); 
		}

		bool IsCompressed() const
		{ 
			return !!( m_nState & eFileCompressed ); 
		}

		void SetCompressed( bool bCompressed )
		{
			m_nState = bCompressed ? m_nState | eFileCompressed : m_nState &~eFileCompressed;
		}

	};

	struct SPkgNameInfo
	{
		enum{ eHashLen = 16 };
		
		uint32 m_nIndexOffset;
		uint32 m_nIndexSize;
		uint8  m_bDataExist;

		uint32 GetExtraDataSize() const
		{
			return m_bDataExist ? sizeof(SMainData) : 0;
		}

		SMainData* GetMainData()
		{
			return m_bDataExist ? (SMainData*)( this + 1 ) : NULL;
		}

		uint32 GetMainDataSize() const
		{
			const SMainData* pData = GetMainData();
			if( !pData )
				return 0;
			return pData->m_nMainDataSizeCompress;
		}

		const SMainData* GetMainData() const
		{
			return m_bDataExist ? (const SMainData*)( this + 1 ) : NULL;
		}

		SPkgNameInfo()
		{
			memset( this, 0, sizeof(SPkgNameInfo) );
		}
		
		bool IsCompressed() const
		{ 
			const SMainData* pMainData = GetMainData();
			if( pMainData )
				return pMainData->IsCompressed();
			return false;
		}

		void SetCompressed( bool bCompressed )
		{
			SMainData* pMainData = GetMainData();
			if( pMainData )
				pMainData->SetCompressed( bCompressed );
		}

		bool IsCompletely() const
		{ 
			const SMainData* pMainData = GetMainData();
			if( pMainData )
				return pMainData->IsCompletely();
			return false;
		}

		void SetCompletely( bool bCompletely )
		{
			SMainData* pMainData = GetMainData();
			if( pMainData )
				pMainData->SetCompletely( bCompletely );
		}
	};

	struct SPkgMainDataHeader
	{
		uint32 m_nFreeAreaOffset;
		uint32 m_nFreeAreaCount;
	};

	// 空闲区域信息
	struct SFreeAreaInfo
	{
		uint32 m_nSize;          // 空闲块大小
		uint32 m_nOffset;        // 空闲块偏移
	};

	enum EDiffPackageState
	{
		eDPS_OK = 0,
		eDPS_ErrorPkgName,
		eDPS_MapNameData,
		eDPS_Unkown,
	};
}
