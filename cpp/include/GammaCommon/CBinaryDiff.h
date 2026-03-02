//=====================================================================
// CPkgFile.h 
// 定义兼容从文件包读取文件以及直接读取系统文件
// 柯达昭
// 2007-09-15
//=======================================================================

#ifndef _BINARY_DIFF_IMPL_H__
#define _BINARY_DIFF_IMPL_H__

#include "GammaCommon/GammaCommonType.h"
#include "GammaCommon/BinaryFind.h"
#include "GammaCommon/CPkgFile.h"
#include <vector>

namespace Gamma
{
	#define PATCH_FILE_EXTRA	_T(".gmz")

	enum EPatchVersion
	{
		ePV_Org			= 10001,
		ePV_Count		,
		ePV_Cur			= ePV_Count - 1,
	};

	struct SFileHead
	{
		uint32 m_nFlag;
		uint32 m_nVersion;
		uint32 m_nDirectoryCount;
		uint32 m_nFilePatchCount;
		uint32 m_nOrgSize;
		uint32 m_nCompressSize;
		uint32 m_nPropsSize;
	};

	struct SFilePatchHeader
	{
		uint32 m_nFileNameLen;	// 文件名长度
		uint32 m_nChunkCount;	// 文件内部含有多少个数据包的补丁
		uint32 m_nFileSize;		// 文件总大小
	};

	struct SChunkInfo
	{
		uint32 m_nSize;			// 块大小
		uint32 m_nOffset;		// 块偏移
		operator uint32() const{ return m_nOffset; }
	};

	class CBinaryDiff
	{
		uint32					m_nMaxSize;
		std::vector<SChunkInfo>	m_vecDiffChunk;
		std::vector<uint32>		m_vecDiffOffset;	// 块偏移
		std::vector<tbyte>		m_vecDiffData;

		template< typename File, typename FunRead, typename FunSeek >
		void ReadDiffDataFromSrc( File& fileRead, FunRead funRead, FunSeek funSeek );

		bool FindNextMerChunk( const std::vector<SChunkInfo>& vecChunk, uint32& nChunkEnd, uint32& nIndex )
		{
			uint32 nStart = nIndex;
			for( ; nIndex < vecChunk.size() && vecChunk[nIndex].m_nOffset < nChunkEnd; nIndex++ )
				nChunkEnd = vecChunk[nIndex].m_nOffset + vecChunk[nIndex].m_nSize;
			return nStart != nIndex;
		}

	public:
		CBinaryDiff(void){};
		~CBinaryDiff(void){};

		uint32 GetMaxSize() const { return m_nMaxSize; }

		template< typename File, typename FunRead, typename FunSize, typename FunSeek >
		void BuildDiff( File& fileRead1, File& fileRead2, FunRead funRead, FunSize funSize, FunSize funSeek );

		template< typename File, typename FunRead, typename FunSeek >
		void Optimize( File& fileRead, FunRead funRead, FunSeek funSeek );

		template< typename File, typename FunRead, typename FunSeek >
		void Merge( const CBinaryDiff& SrcBinaryDiff, File& fileRead, FunRead funRead, FunSeek funSeek );

		template< typename File, typename FunRead >
		void Load( File& fileRead, FunRead funRead, uint32 nChunk, uint32 nMaxSize );

		template< typename File, typename FunWrite >
		void Save( File& fileRead, FunWrite funWrite );

		void Patch( void* pBuf, uint32 nOffset, uint32 nSize );

		bool IsNeedPatch( uint32 nOffset, uint32 nSize ) const;

		uint32 Init( uint32 nMaxSize, uint32 nDiffChunkCount, const tbyte* pDiffChunk );
	};
}

#include "GammaCommon/CBinaryDiff.inl"

#endif
