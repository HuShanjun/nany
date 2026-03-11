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
		uint32_t m_nFlag;
		uint32_t m_nVersion;
		uint32_t m_nDirectoryCount;
		uint32_t m_nFilePatchCount;
		uint32_t m_nOrgSize;
		uint32_t m_nCompressSize;
		uint32_t m_nPropsSize;
	};

	struct SFilePatchHeader
	{
		uint32_t m_nFileNameLen;	// 文件名长度
		uint32_t m_nChunkCount;	// 文件内部含有多少个数据包的补丁
		uint32_t m_nFileSize;		// 文件总大小
	};

	struct SChunkInfo
	{
		uint32_t m_nSize;			// 块大小
		uint32_t m_nOffset;		// 块偏移
		operator uint32_t() const{ return m_nOffset; }
	};

	class CBinaryDiff
	{
		uint32_t					m_nMaxSize;
		std::vector<SChunkInfo>	m_vecDiffChunk;
		std::vector<uint32_t>		m_vecDiffOffset;	// 块偏移
		std::vector<tbyte>		m_vecDiffData;

		template< typename File, typename FunRead, typename FunSeek >
		void ReadDiffDataFromSrc( File& fileRead, FunRead funRead, FunSeek funSeek );

		bool FindNextMerChunk( const std::vector<SChunkInfo>& vecChunk, uint32_t& nChunkEnd, uint32_t& nIndex )
		{
			uint32_t nStart = nIndex;
			for( ; nIndex < vecChunk.size() && vecChunk[nIndex].m_nOffset < nChunkEnd; nIndex++ )
				nChunkEnd = vecChunk[nIndex].m_nOffset + vecChunk[nIndex].m_nSize;
			return nStart != nIndex;
		}

	public:
		CBinaryDiff(void){};
		~CBinaryDiff(void){};

		uint32_t GetMaxSize() const { return m_nMaxSize; }

		template< typename File, typename FunRead, typename FunSize, typename FunSeek >
		void BuildDiff( File& fileRead1, File& fileRead2, FunRead funRead, FunSize funSize, FunSize funSeek );

		template< typename File, typename FunRead, typename FunSeek >
		void Optimize( File& fileRead, FunRead funRead, FunSeek funSeek );

		template< typename File, typename FunRead, typename FunSeek >
		void Merge( const CBinaryDiff& SrcBinaryDiff, File& fileRead, FunRead funRead, FunSeek funSeek );

		template< typename File, typename FunRead >
		void Load( File& fileRead, FunRead funRead, uint32_t nChunk, uint32_t nMaxSize );

		template< typename File, typename FunWrite >
		void Save( File& fileRead, FunWrite funWrite );

		void Patch( void* pBuf, uint32_t nOffset, uint32_t nSize );

		bool IsNeedPatch( uint32_t nOffset, uint32_t nSize ) const;

		uint32_t Init( uint32_t nMaxSize, uint32_t nDiffChunkCount, const tbyte* pDiffChunk );
	};
}

#include "GammaCommon/CBinaryDiff.inl"

#endif
