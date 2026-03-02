//==============================================================
// CRunLength.h 
// 定义行程压缩算法
// 柯达昭
// 2008-12-17
//==============================================================

#ifndef _GAMMA_RUNLENGHT_H_
#define _GAMMA_RUNLENGHT_H_

#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/CPkgFile.h"
#include <string>

namespace Gamma
{
	template<typename FlagType>
	class TRunLength
	{
		enum 
		{ 
			eDiffFlag  = 1 << ( sizeof(FlagType)*8 - 1 ),
			eMaxLength = eDiffFlag - 1,
		};
	public:
		//==============================================================
		// 行程压缩
		//==============================================================
		template< class OutFile, class OutFun, class InFile, class InFun >
		static void Compress( OutFile& fileWrite, OutFun funWrite, InFile& fileRead, InFun funRead, uint32 nUnitSize )
		{
			tbyte		nRefBuf[256];
			tbyte		nCurBuf[256];
			uint32		nSameCount = 0;
			std::string	szRunDiffBuf;

			//先读一个数据块作为参考数据
			int32		nRead  = ( fileRead.*funRead )( (char*)nRefBuf, nUnitSize );

			while( nRead > 0 )
			{
				// 默认当读源文件失败时写入文件
				bool bFlush = true;
				bool bSame  = false;
				nRead		= ( fileRead.*funRead )( (char*)nCurBuf, nUnitSize );

				if( nRead > 0 )
				{
					// 默认当读源文件成功时不写入文件
					bFlush = false;
					bSame  = !memcmp( nRefBuf, nCurBuf, nUnitSize );

					if( bSame )
					{
						// 如果之前有从源文件读入数据的每个单元数据内容都是不同的，要写入之前的数据
						if( szRunDiffBuf.size() )
						{
							szRunDiffBuf.erase( szRunDiffBuf.size() - nUnitSize, nUnitSize );
							bFlush = true;
							nSameCount++;
						}
						// 如果nSameCount为0，说明是第一次读数据，要将nRefBuf算进相同数量中
						else if( nSameCount == 0 )
							nSameCount++;
						nSameCount++;
					}
					else
					{
						// 如果之前有从源文件读入数据的每个单元数据内容都是相同的，要写入之前的数据
						if( nSameCount )
							bFlush = true;
						// 如果szRunDiffBuf为空，说明是第一次读数据，要将nRefBuf压入szRunDiffBuf
						else if( szRunDiffBuf.empty() )
							szRunDiffBuf.append( (const char*)nRefBuf, nUnitSize );

						// 如果不需要写数据，那么需要将参考数据nRefBuf改为当前数据nCurBuf
						if( !bFlush )
							memcpy( nRefBuf, nCurBuf, nUnitSize );
						// 将当前的数据压入szRunBuf
						szRunDiffBuf.append( (const char*)nCurBuf, nUnitSize );
					}
				}

				if( bFlush )
				{
					if( ( nSameCount && ( nRead <= 0 || !bSame ) ) || ( !nSameCount && szRunDiffBuf.empty() ) )
					{
						uint32 nUnitCount = Max<uint32>( nSameCount, 1 );
						for( uint32 i = 0; i < nUnitCount; i += eMaxLength )
						{
							FlagType nRunFlag = (FlagType)Min<uint32>( nUnitCount - i, eMaxLength );
							( fileWrite.*funWrite )( (const char*)&nRunFlag, sizeof(nRunFlag) );
							( fileWrite.*funWrite )( nRefBuf, nUnitSize );
						}
						nSameCount = 0;
					}
					
					if( szRunDiffBuf.size() && ( nRead <= 0 || bSame ) )
					{
						uint32 nUnitCount = (uint32)( szRunDiffBuf.size()/nUnitSize );
						for( uint32 i = 0; i < nUnitCount; i += eMaxLength )
						{
							FlagType nRunFlag = (FlagType)( Min<uint32>( nUnitCount - i, eMaxLength )|eDiffFlag );
							( fileWrite.*funWrite )( (const char*)&nRunFlag, sizeof(nRunFlag) );
							( fileWrite.*funWrite )( szRunDiffBuf.c_str() + i*nUnitSize, ( nRunFlag&eMaxLength )*nUnitSize );
						}
						szRunDiffBuf.clear();
					}

					// 将参考数据nRefBuf改为当前数据nCurBuf
					memcpy( nRefBuf, nCurBuf, nUnitSize );
				}	
			}
		}

		//==============================================================
		// 特化，当InFile为BufferFile时不需要szRunDiffBuf
		//==============================================================
		template< class OutFile, class OutFun, class InFun >
		static void Compress( OutFile& fileWrite, OutFun funWrite, CBufFile& fileRead, InFun /*funRead*/, uint32 nUnitSize )
		{
			uint32			nSameCount = 0;
			const tbyte*	pDiffStart = NULL;
			uint32			nDiffCount = 0;

			//先读一个数据块作为参考数据
			const tbyte*	pPreUnitStart = fileRead.GetBuf();

			while( fileRead.GetPos() < fileRead.GetBufSize() )
			{
				// 默认当读源文件失败时写入文件
				bool bFlush = true;
				bool bSame  = false;
				fileRead.SeekFromCur( nUnitSize );
				const tbyte* pCurUnitStart = fileRead.GetBuf() + fileRead.GetPos();

				bool bRemainData = fileRead.GetPos() < fileRead.GetBufSize();
				if( bRemainData )
				{
					// 默认当读源文件成功时不写入文件
					bFlush = false;
					bSame  = !memcmp( pPreUnitStart, pCurUnitStart, nUnitSize );

					if( bSame )
					{
						// 如果之前有从源文件读入数据的每个单元数据内容都是不同的，要写入之前的数据
						if( nDiffCount )
						{
							nDiffCount = nDiffCount - nUnitSize;
							bFlush = true;
							nSameCount++;
						}
						// 如果nSameCount为0，说明是第一次读数据，要将nRefBuf算进相同数量中
						else if( nSameCount == 0 )
							nSameCount++;
						nSameCount++;
					}
					else
					{
						// 如果之前有从源文件读入数据的每个单元数据内容都是相同的，要写入之前的数据
						if( nSameCount )
						{
							pDiffStart = pCurUnitStart;
							bFlush = true;
						}
						// 如果szRunDiffBuf为空，说明是第一次读数据，要将nRefBuf压入szRunDiffBuf
						else if( nDiffCount == 0 )
						{
							pDiffStart = pPreUnitStart;
							nDiffCount = nDiffCount + nUnitSize;
						}

						// 如果不需要写数据，那么需要将参考数据nRefBuf改为当前数据nCurBuf
						if( !bFlush )
							pPreUnitStart = pCurUnitStart;
						// 将当前的数据压入szRunBuf
						nDiffCount = nDiffCount + nUnitSize;
					}
				}

				if( bFlush )
				{
					if( ( nSameCount && ( !bRemainData || !bSame ) ) || ( !nSameCount && nDiffCount == 0 ) )
					{
						uint32 nUnitCount = Max<uint32>( nSameCount, (uint32)1 );
						for( uint32 i = 0; i < nUnitCount; i += eMaxLength )
						{
							FlagType nRunFlag = (FlagType)Min<uint32>( nUnitCount - i, eMaxLength );
							( fileWrite.*funWrite )( (const char*)&nRunFlag, sizeof(nRunFlag) );
							( fileWrite.*funWrite )( pPreUnitStart, nUnitSize );
						}
						nSameCount = 0;
					}

					if( nDiffCount && ( !bRemainData || bSame ) )
					{
						uint32 nUnitCount = nDiffCount/nUnitSize;
						for( uint32 i = 0; i < nUnitCount; i += eMaxLength )
						{
							FlagType nRunFlag = (FlagType)( Min<uint32>( nUnitCount - i, eMaxLength )|eDiffFlag );
							( fileWrite.*funWrite )( (const char*)&nRunFlag, sizeof(nRunFlag) );
							( fileWrite.*funWrite )( pDiffStart + i*nUnitSize, ( nRunFlag&eMaxLength )*nUnitSize );
						}
						nDiffCount = 0;
					}

					// 将参考数据nRefBuf改为当前数据nCurBuf
					pPreUnitStart = pCurUnitStart;
				}	
			}
		}

		template< class OutFile, class OutFun, class InFun >
		static void Compress( OutFile& fileWrite, OutFun funWrite, CBufFileEx& fileRead, InFun /*funRead*/, uint32 nUnitSize )
		{
			return Compress( fileWrite, funWrite, (CBufFile&)fileRead, 0, nUnitSize );
		}

		//==============================================================
		// 行程解压
		//==============================================================
		template< class OutFile, class OutFun, class InFile, class InFun >
		static void Decompress( OutFile& fileWrite, OutFun funWrite, InFile& fileRead, InFun funRead, uint32 nUnitSize )
		{
			FlagType nRunFlag;
			tbyte nBuf[256];

			while( ( fileRead.*funRead )( (char*)&nRunFlag, sizeof(FlagType) ) > 0 )
			{
				FlagType nRunLen = (FlagType)( nRunFlag&eMaxLength );
				if( nRunFlag == nRunLen )
				{
					if( ( fileRead.*funRead )( nBuf, nUnitSize ) < 0 )
						GammaThrow( "error format!!" );
					for( FlagType i = 0; i < nRunLen; i++ )
						( fileWrite.*funWrite )( nBuf, nUnitSize );
				}
				else
				{
					for( FlagType i = 0; i < nRunLen; i++ )
					{
						if( ( fileRead.*funRead )( nBuf, nUnitSize ) < 0 )
							GammaThrow( "error format!!" );
						( fileWrite.*funWrite )( nBuf, nUnitSize );
					}
				}
			}
		}
	};

	typedef TRunLength<uint16>	CRunLength16;
	typedef TRunLength<uint8>	CRunLength08;
}

#endif
