#pragma once
#include <new>
#include <cstddef>
#include <cassert>
#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/GammaMemory.h"

#define _DEBUG 1

namespace Gamma
{
	template<typename InstanceClass, size_t nInitSize = 256>
	class TTinyObjectPool
	{
		enum
		{
			eInitSize = TMax<nInitSize, 256>::eValue,
			ePageSize = TAligenUpTo2Power<nInitSize*sizeof( InstanceClass )>::eValue,
			eAllocCount = ePageSize/ sizeof( InstanceClass )
		};

		typedef TFixedPageAlloc<ePageSize> CPageAlloc;
		struct SFreeNode { SFreeNode* pNextFree; };
		SFreeNode* m_pFreeNode;
		uint32_t m_nTotalCount;
		uint32_t m_nUseCount;

	public:
		TTinyObjectPool() 
			: m_pFreeNode( nullptr )
			, m_nTotalCount( 0 )
			, m_nUseCount( 0 )
		{
		}

		static TTinyObjectPool& Inst()
		{
			static TTinyObjectPool s_Instance;
			return s_Instance;
		}

		template<typename... ConstructParam>
		InstanceClass* Alloc( ConstructParam...p )
		{
			if( m_pFreeNode == nullptr )
			{
				m_nTotalCount += eAllocCount;
				InstanceClass* aryInst = (InstanceClass*)CPageAlloc::Alloc();
				memset( aryInst, 0, ePageSize );
				for( size_t i = 0; i < eAllocCount; i++ )
				{
					SFreeNode* pNode = (SFreeNode*)(void*)&aryInst[i];
					pNode->pNextFree = m_pFreeNode;
					m_pFreeNode = pNode;
				}
			}

			++m_nUseCount;
			SFreeNode* pNew = m_pFreeNode;
			m_pFreeNode = m_pFreeNode->pNextFree;
			pNew->pNextFree = nullptr;
			return new( pNew ) InstanceClass( p... );
		}

		void Free( InstanceClass* pFree )
		{
			--m_nUseCount;
			pFree->~InstanceClass();
			SFreeNode* pNode = (SFreeNode*)(void*)pFree;
			pNode->pNextFree = m_pFreeNode;
			m_pFreeNode = pNode;
		}
	};

	template <class T, size_t PageSize = 4096>
	class TTinyObjectPool2 {
	public:
		TTinyObjectPool2() : m_FreeList(nullptr), m_PageList(nullptr), m_UseCount(0) {}
		~TTinyObjectPool2() {
			Clear();
		}

		// 禁止拷贝
		TTinyObjectPool2(const TTinyObjectPool2&) = delete;
		TTinyObjectPool2& operator=(const TTinyObjectPool2&) = delete;

		// 分配对象
		T* Alloc() {
			if (!m_FreeList) AllocatePage();

			FreeNode* node = m_FreeList;
			m_FreeList = node->next;

			T* obj = new (&node->storage) T();
			++m_UseCount;
				return obj;
		}

		// 释放对象
		void Free(T* obj) {
			if (!obj) return;

			obj->~T(); // 调用析构

			// 使用 offsetof 计算回 FreeNode 起始地址
			FreeNode* node = reinterpret_cast<FreeNode*>(
				reinterpret_cast<char*>(obj) - offsetof(FreeNode, storage)
			);

			node->next = m_FreeList;
			m_FreeList = node;

			assert(m_UseCount > 0);
			--m_UseCount;
		}

		void Clear() {
			Page* page = m_PageList;
			while (page) {
				Page* next = page->next;
				::operator delete(page);
				page = next;
			}
			m_PageList = nullptr;
			m_FreeList = nullptr;
			m_UseCount = 0;
		}

		size_t GetUseCount() const { return m_UseCount; }

	private:
		struct FreeNode {
			FreeNode* next;
			alignas(T) unsigned char storage[sizeof(T)];
		};

		struct Page {
			Page* next;
			size_t count;      // nodes 数量
			FreeNode nodes[1]; // 变长尾数组
		};

		void AllocatePage() {
			// Page 头部到 nodes 偏移
			size_t header = offsetof(Page, nodes);
			size_t count = (PageSize - header) / sizeof(FreeNode);
			if (count < 1) count = 1;

			size_t pageSize = header + count * sizeof(FreeNode);
			Page* page = reinterpret_cast<Page*>(::operator new(pageSize));

			page->next = m_PageList;
			page->count = count;
			m_PageList = page;

			for (size_t i = 0; i < count; ++i) {
				FreeNode* node = &page->nodes[i];
				node->next = m_FreeList;
				m_FreeList = node;
			}
		}

		FreeNode* m_FreeList;
		Page* m_PageList;
		size_t m_UseCount;
	};
}
