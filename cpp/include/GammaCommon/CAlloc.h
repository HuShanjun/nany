//=========================================================================
// CAlloc.h 
// 内存分配算法
// 柯达昭
// 2007-09-19
//=========================================================================
#ifndef __GAMMA_CALLOC_H__
#define __GAMMA_CALLOC_H__

#include <map>
#include <list>
#include <vector>
#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/BinaryFind.h"

#pragma warning(disable:4127)

namespace Gamma
{
    //======================================================================
    // 最佳适应分配策略
    //======================================================================
    /*class CBestFitAlloc
    {
        struct _Block;
        typedef std::map<size_t, _Block>::iterator it_b;

        //块的属性
        struct _Block
        { 
            std::multimap<size_t,it_b>::iterator itFree;      //块在空闲表的位置
            size_t nSize;                                     //块的大小
        };

        size_t                        m_nBegin;
        size_t                        m_nSize;
        std::multimap<size_t,it_b>    m_Free;                 //空闲表，key为块的大小，data为块的起始位置
        std::map<size_t, _Block>      m_Block;                //块的列表，key为块的起始位置，data为块的属性

    public:
        enum                { ALLOC_ERROR = -1 };
        size_t GetStart()   { return m_nBegin; }
        size_t GetSize()    { return m_nSize; }

        CBestFitAlloc( size_t nBegin, size_t nSize )
            : m_nBegin(nBegin), m_nSize(nSize)
        {
            _Block b = { m_Free.insert( std::pair<size_t,it_b>( m_nSize, m_Block.end() ) ), m_nSize };
            it_b itBlock = m_Block.insert( std::pair<size_t, _Block>( m_nBegin, b ) ).first;
            itBlock->second.itFree->second = itBlock;
        }

        ~CBestFitAlloc(void)
        {
        }

        void Reset( size_t nBegin, size_t nSize )
        {
            m_Free.clear();
            m_Block.clear();
            m_nBegin = nBegin;
            m_nSize = nSize;
            _Block b = { m_Free.insert( std::pair<size_t,it_b>( m_nSize, m_Block.end() ) ), m_nSize };
            it_b itBlock = m_Block.insert( std::pair<size_t, _Block>( m_nBegin, b ) ).first;
            itBlock->second.itFree->second = itBlock;
        }

        size_t Alloc( size_t nSize )
        {
            std::multimap<size_t,it_b>::iterator itFree = m_Free.lower_bound( nSize );
            if( itFree == m_Free.end() )
                return (size_t)ALLOC_ERROR;
            it_b itBlock = itFree->second;
            m_Free.erase( itFree )   ;

            GammaAst( itBlock != m_Block.end() );

            if( nSize < (size_t)( itBlock->second.nSize*0.95f ) )
            {
                _Block b = 
                { 
                    m_Free.insert( std::pair<size_t,it_b>( itBlock->second.nSize - nSize, m_Block.end() ) ), 
                    itBlock->second.nSize - nSize
                };

                it_b itNewBlock = m_Block.insert( std::pair<size_t, _Block>( itBlock->first + nSize, b ) ).first;
                itNewBlock->second.itFree->second = itNewBlock;
                itBlock->second.nSize   = nSize;
            }

            itBlock->second.itFree  = m_Free.end();
            return itBlock->first;
        }

        size_t Free( size_t nStart )
        {
            std::map<size_t, _Block>::iterator itBlock = m_Block.find( nStart );
            GammaAst( itBlock != m_Block.end() );

            if( itBlock != m_Block.begin() )
            {
                std::map<size_t, _Block>::iterator itPre = itBlock;
                --itPre;
                if( itPre->second.itFree != m_Free.end() )
                {
                    itPre->second.nSize += itBlock->second.nSize;
                    m_Free.erase( itPre->second.itFree );
                    m_Block.erase( itBlock );
                    itBlock = itPre;
                }
            }

            if( itBlock != ( --m_Block.end() ) )
            {
                std::map<size_t, _Block>::iterator itNext = itBlock;
                ++itNext;
                if( itNext->second.itFree != m_Free.end() )
                {
                    itBlock->second.nSize += itNext->second.nSize;
                    m_Free.erase( itNext->second.itFree );
                    m_Block.erase( itNext );
                }
            }

            itBlock->second.itFree = m_Free.insert( std::pair<size_t,it_b>( itBlock->second.nSize, itBlock ) ); 
            return itBlock->second.nSize;
        }
    };*/

    //======================================================================
    // 定长内存分配策略
    // 用空间换时间
    // 每一个分配的Block需要耗费4个字节，
    // 所以nBlockSize最好尽量大，以提高内存利用率
	//======================================================================

    template<
		size_t sizeBlock, 
		size_t countBlockPerPage, 
		bool bRecycle,
		bool bThrow = true, 
		typename PageAllocFun = void* (*)( size_t ),
		typename PageFreeFun = void (*)( void* ), 
		typename ListAllocFun = void* (*)( size_t ),
		typename ListFreeFun = void (*)( void* ) >
    class TFixSizeAlloc
	{
		struct SPage 
		{
			tbyte*		m_pFirstFreeBlock;	// 第一块空闲块
			size_t		m_nFreeCount;
		};

		// 常量
		const size_t	m_nBlockSize;
		const size_t	m_nBlockPerPage;
		PageAllocFun	m_pPageAllocFun;
		PageFreeFun		m_pPageFreeFun;
		ListAllocFun	m_pListAllocFun;
		ListFreeFun		m_pListFreeFun;

		size_t			m_nPageCount;		// 内存页数量
		SPage**			m_ppPageList;		// 内存页链表
        size_t			m_nFreeCount;		// 空闲表中空闲的Block数量
		size_t			m_nMinFreePage;

        void AddPage()
		{
			SPage* pNewPage = (SPage*)m_pPageAllocFun( m_nBlockPerPage*m_nBlockSize + sizeof(SPage) );
			SPage** ppNewPageList = (SPage**)m_pListAllocFun( sizeof(tbyte*)*( m_nPageCount + 1 ) );
			size_t nInsertPlace = size_t(-1);
			for( size_t i = 0; i < m_nPageCount + 1; i++ )
			{
				if( nInsertPlace != size_t(-1) )
					ppNewPageList[i] = m_ppPageList[i-1];
				else if( i == m_nPageCount )
				{
					nInsertPlace = i;
					ppNewPageList[i] = pNewPage;
				}
				else if( m_ppPageList[i] > pNewPage )
				{
					nInsertPlace = i;
					ppNewPageList[i] = pNewPage;
					ppNewPageList[i + 1] = m_ppPageList[i];
					i++;
				}
				else
					ppNewPageList[i] = m_ppPageList[i];
			}

			if( m_ppPageList ) 
				m_pListFreeFun( m_ppPageList );
			m_ppPageList = ppNewPageList;
			pNewPage->m_pFirstFreeBlock = NULL;
			pNewPage->m_nFreeCount = 0;

			m_nPageCount++;
			m_nFreeCount += m_nBlockPerPage;

			tbyte* pPageBuffer = (tbyte*)( pNewPage + 1 );
			for( size_t i = 0; i < m_nBlockPerPage*m_nBlockSize; i += m_nBlockSize )
			{
				tbyte* pCurBlock = &pPageBuffer[i];
				*(void**)( pCurBlock + m_nBlockSize - sizeof(tbyte*) ) = pNewPage->m_pFirstFreeBlock;
				pNewPage->m_pFirstFreeBlock = pCurBlock;
				pNewPage->m_nFreeCount++;
			}

			if( m_nMinFreePage > nInsertPlace )
				m_nMinFreePage = nInsertPlace;
        }

		void DelPage( size_t nIndex )
		{
			m_pPageFreeFun( m_ppPageList[nIndex] );
			m_nFreeCount -= m_nBlockPerPage;
			m_nPageCount--;
			memcpy( m_ppPageList + nIndex, m_ppPageList + nIndex + 1, ( m_nPageCount - nIndex )*sizeof(SPage*) );
		}

		const TFixSizeAlloc& operator= ( const TFixSizeAlloc& );
		TFixSizeAlloc( const TFixSizeAlloc& );

		static void* _Alloc( size_t nSize ) { return new tbyte[nSize]; }
		static void  _Free( void* pMemory ) { delete [] (tbyte*)pMemory; }

    public:
		enum { ePageHeadSize = sizeof(SPage) };

		TFixSizeAlloc( 
			size_t nBlockSize = sizeBlock, 
			size_t nBlockPerPage = countBlockPerPage,
			PageAllocFun pPageAllocFun = NULL, 
			PageFreeFun pPageFreeFun = NULL,
			ListAllocFun pListAllocFun = NULL, 
			ListFreeFun pListFreeFun = NULL ) 
			: m_nFreeCount(0)
			, m_nPageCount(0)
			, m_ppPageList(NULL)
			, m_nBlockSize( AligenUp( Max<uint32>( (uint32)nBlockSize, 1 ), (uint32)sizeof(tbyte*) ) )
			, m_nBlockPerPage( nBlockPerPage )
			, m_pPageAllocFun( pPageAllocFun ? pPageAllocFun : &_Alloc )
			, m_pPageFreeFun( pPageFreeFun ? pPageFreeFun : &_Free )
			, m_pListAllocFun( pListAllocFun ? pListAllocFun : &_Alloc )
			, m_pListFreeFun( pListFreeFun ? pListFreeFun : &_Free )
			, m_nMinFreePage( size_t(-1) )
        {
        }

        ~TFixSizeAlloc()
        {
			//检查内存泄漏
			if ( bThrow )
			{
				GammaAst( m_nFreeCount == m_nPageCount*m_nBlockPerPage );
			}

			for( size_t i = 0; i < m_nPageCount; i++ )
				m_pPageFreeFun( m_ppPageList[i] );
			m_pListFreeFun( m_ppPageList );
        }

        void* Alloc()
        {
            if( m_nFreeCount == 0 )
			{
				m_nMinFreePage = size_t(-1);
				AddPage();
			}

			SPage* pPage = m_ppPageList[m_nMinFreePage];
			if( pPage->m_nFreeCount == 0 )
			{
				while( m_nMinFreePage < m_nPageCount && m_ppPageList[m_nMinFreePage]->m_nFreeCount == 0 )
					m_nMinFreePage++;
				pPage = m_ppPageList[m_nMinFreePage];
			}

			GammaAst( m_nMinFreePage < m_nPageCount );

			tbyte* pCurBlock = pPage->m_pFirstFreeBlock;
			pPage->m_pFirstFreeBlock = *(tbyte**)( pCurBlock + m_nBlockSize - sizeof(tbyte*) );

			ptrdiff_t nDiffAdress = pPage->m_pFirstFreeBlock - (tbyte*)pPage;
			int32 nPageSize = (int32)( m_nBlockPerPage*m_nBlockSize + sizeof(SPage) );
			if( pPage->m_pFirstFreeBlock && ( nDiffAdress < 0 || nDiffAdress > nPageSize ) )
			{
				GammaLog << "Memory overrun!!!!" << std::endl;
				pPage->m_pFirstFreeBlock = NULL;
				m_nFreeCount -= pPage->m_nFreeCount;
				pPage->m_nFreeCount = 0;
			}
			else
			{
				m_nFreeCount--;
				pPage->m_nFreeCount--;
			}

            return pCurBlock;
        }

        bool Free( void* pMem )
        {
			// 为了分配器更加健壮，释放时检查是否此分配器中分配的内存，不是的话不予释放
			size_t nBegin, nEnd; 
			if( !GetBound( m_ppPageList, m_nPageCount, (SPage*)pMem, nBegin, nEnd ) )
				return false;	

			SPage* pPage = m_ppPageList[nBegin];
			if( ( (tbyte*)pMem - (tbyte*)( pPage + 1 ) )%m_nBlockSize )
				return false;

			GammaAst( m_nFreeCount < m_nPageCount*m_nBlockPerPage );
			tbyte* pCurBlock = (tbyte*)pMem;
			ptrdiff_t nDiffAdress = pCurBlock - (tbyte*)pPage;
			int32 nPageSize = (int32)( m_nBlockPerPage*m_nBlockSize + sizeof(SPage) );
			if( nDiffAdress < 0 || nDiffAdress > nPageSize )
				return false;

			*(void**)( pCurBlock + m_nBlockSize - sizeof(tbyte*) ) = pPage->m_pFirstFreeBlock;
			pPage->m_pFirstFreeBlock = pCurBlock;
			pPage->m_nFreeCount++;
			m_nFreeCount++;

			if( nBegin < m_nMinFreePage )
				m_nMinFreePage = nBegin;

			// 剩余空间太多时不妨减少一些
			if( bRecycle && pPage->m_nFreeCount == m_nBlockPerPage && m_nFreeCount > m_nBlockPerPage*2 )
				DelPage( nBegin );

			return true;
        }

        size_t GetFreeBlockCount() const
        {
            return m_nFreeCount;
        }

		size_t GetPageCount() const
		{
			return m_nPageCount;
		}

		const void* GetPage( size_t nIndex ) const
		{
			if( nIndex >= m_nPageCount )
				return NULL;
			return m_ppPageList[nIndex];
		}

		size_t GetPageSize() const
		{
			return m_nBlockPerPage*m_nBlockSize;
		}

		size_t GetBlockCountPerPage() const
		{
			return m_nBlockPerPage;
		}

		size_t GetTotalBlockCount() const
		{
			return m_nBlockPerPage*m_nPageCount;
		}

		size_t GetBlockSize() const
		{
			return m_nBlockSize;
		}
    };
}

#endif
