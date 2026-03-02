//===============================================
// TAStar.h 
// 用模板的方式定义了A*算法，将数据结构部分提取出来
// 柯达昭
// 2009-06-28
//===============================================

#ifndef __GAMMA_ASTAR_H__
#define __GAMMA_ASTAR_H__

#include <vector>
#include "GammaCommon/TVector2.h"
#include "GammaCommon/THeapSort.h"

namespace Gamma
{
	//===================================================
	// A*核心算法，用模板的方式将数据和算法剥离开
	//===================================================
	template<typename DataStruct>
	class TAStar
	{
		typedef typename DataStruct::Node				MaskNode;
		typedef typename DataStruct::IndexType			IndexType;
		typedef typename DataStruct::CostType			CostType;

		struct SHeapSortNode
		{
			MaskNode* m_pNode;
			SHeapSortNode(MaskNode* pNode = 0) : m_pNode(pNode) {}
			bool operator< (const SHeapSortNode& r) const
			{
				return (CostType)*m_pNode < (CostType)*r.m_pNode;
			}
		};

		struct SHeapSortVector : public THeapSortVector<SHeapSortNode>
		{
		public:
			void SetData(size_t n, const SHeapSortNode& e)
			{
				THeapSortVector<SHeapSortNode>::SetData(n, e);
				e.m_pNode->Open((uint32)n);
			}
		};

		typedef THeapSort<SHeapSortVector>	OpenList;

		OpenList			m_vecOpen;
		IndexType			m_nodeEnd;
		DataStruct*			m_pMask;
		MaskNode*			m_pClosestNode;

		//=========================================
		// 选择一个合适的地方插入新的open节点，
		// 使得消耗最小的节点排在数组的最后面
		//=========================================
		void InsertOpenNode(MaskNode* pNode, MaskNode* pParent)
		{
			// 新节点直接插入到相应的位置
			if (pNode->IsNew())
			{
				pNode->CalculateCost(pParent, m_pMask, m_nodeEnd);
				m_vecOpen.Insert( SHeapSortNode( pNode ) );
			}
			else
			{
				// 当新的父节点比之前的父节点到达此节点消耗更小，
				// 则替换之前的父节点，并且根据消耗调整节点在数组中的位置
				if (pNode->CalculateCost(pParent, m_pMask, m_nodeEnd))
					m_vecOpen.CheckUp(pNode->GetOpenIndex());
			}

			// 记录离终点最近的节点
			if (pNode->CompareClose(m_pClosestNode))
				m_pClosestNode = pNode;

#ifdef DEBUG_AStar
			CDC* pDC = ::AfxGetMainWnd()->GetDC();
			pDC->SetPixel(pNode->GetNode(), 0x00ff0000);
			::AfxGetMainWnd()->ReleaseDC(pDC);
#endif
		}

		//================================================================
		// 在数组中选择消耗最小的节点，将其加入close列表，并且遍历其子节点
		//================================================================
		bool CheckOpenNode()
		{
			MaskNode* pCurNode = m_vecOpen.RemoveFront().m_pNode;
			pCurNode->Close();

#ifdef DEBUG_AStar
			CDC* pDC = ::AfxGetMainWnd()->GetDC();
			pDC->SetPixel(pCurNode->GetNode(), 0x000000ff);
			::AfxGetMainWnd()->ReleaseDC(pDC);
			MSG msg; PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
#endif

			if (pCurNode->GetNode() == m_nodeEnd)
				return true;

			IndexType nodeOpen;
			for (int32 i = 0; (i = m_pMask->GetNextNode(pCurNode->GetNode(), i, nodeOpen)) >= 0; i++)
			{
				MaskNode* pOpenNode = m_pMask->GetNode(nodeOpen);
				if (!pOpenNode->IsClosed())
					InsertOpenNode(pOpenNode, pCurNode);
			}

			return false;
		}

	public:
		TAStar()
		{
		}

		//==================================
		// 启动A*搜索
		//==================================
		MaskNode* Search(const IndexType& nodeStart, const IndexType& nodeEnd, DataStruct* pMask, uint32 nSearchDepth = 100000)
		{
			m_vecOpen.GetArrayData().Clear();

			m_nodeEnd = nodeEnd;
			m_pMask = pMask;
			MaskNode* pStartNode = m_pMask->GetNode(nodeStart);
			m_pClosestNode = pStartNode;

			InsertOpenNode(pStartNode, NULL);

			size_t i = 0;
			while (i < nSearchDepth && !m_vecOpen.IsEmpty() && !CheckOpenNode())
				i++;
			return m_pClosestNode;
		}
	};

	//===================================================
	// A*核心算法用到的格子地图搜索路径的的数据结构
	// 符合上面A*核心算法的数据结构需要包某些特定函数
	//===================================================
	template<typename BarrierChecker, typename DistanceType = int32>
	class TGridData
	{
		enum { eRegionWidth = 16 };
		friend class TAStar< TGridData< BarrierChecker, DistanceType > >;
		typedef TVector2<DistanceType> IndexType;
		typedef DistanceType CostType;

		struct Node
		{
			enum EState { eNew = -2, eClosed = -1 };
			uint8			m_xOffset;
			uint8			m_yOffset;
			CostType		m_nCostFromBegin;
			CostType		m_nCostTotal;
			Node*			m_pParent;
			uint32			m_nOpenIndex;
			uint32			m_nSearchID;

		public:
			Node() : m_nOpenIndex((uint32)eNew), m_nSearchID(0), m_pParent(NULL) {}
			operator CostType() const { return m_nCostTotal; }
			bool			IsClosed() const { return m_nOpenIndex == (uint32)eClosed; }
			bool			IsNew()	const { return m_nOpenIndex == (uint32)eNew; }
			uint32			GetOpenIndex() const { return m_nOpenIndex; }
			void			Open( uint32 nIndex ) { m_nOpenIndex = nIndex; }
			void			Close() { m_nOpenIndex = (uint32)eClosed; }

			//==================================
			// 代价判断，重要的函数之一
			// 距离估价尽量使用整数（速度快），
			// 但是为了保留一定的计算精度，
			// 将估价值提高1000倍
			//==================================
			static CostType Judge(IndexType Node1, IndexType Node2)
			{
				CostType xDelta = (Node1.x < Node2.x) ? (Node2.x - Node1.x) : (Node1.x - Node2.x);
				CostType yDelta = (Node1.y < Node2.y) ? (Node2.y - Node1.y) : (Node1.y - Node2.y);
				return (xDelta + yDelta) << 10; // * 1024;
			}

			//==================================
			// 获取绝对坐标
			//==================================
			IndexType GetNode()
			{ 
				CostType nIndex = m_yOffset * eRegionWidth + m_xOffset;
				Region* pRegion = (Region*)(void*)(this - nIndex);
				return pRegion->m_vStartPos + IndexType( m_xOffset, m_yOffset );
			}

			//==================================
			// 计算消耗，考虑拐弯带来的消耗
			//==================================
			bool CalculateCost(Node* pParent, TGridData* pGridData, const IndexType& posEnd)
			{
				IndexType CurNode = GetNode();
				if (!pParent)
				{
					m_nCostFromBegin = CostType();
					m_pParent = pParent;
					m_nCostTotal = Judge( posEnd, CurNode );
					return true;
				}
				else
				{
					IndexType ParentNode = pParent->GetNode();
					IndexType vDelta = ParentNode - CurNode;
					CostType nCostFromParent = (vDelta.x && vDelta.y) ? 1448 : 1024;
					CostType nCostFromBegin = nCostFromParent + pParent->m_nCostFromBegin;

					if (!m_pParent || nCostFromBegin < m_nCostFromBegin)
					{
						if( !m_pParent )
							m_nCostTotal = nCostFromBegin + Judge( posEnd, CurNode );
						else
							m_nCostTotal = m_nCostTotal - m_nCostFromBegin + nCostFromBegin;

						m_nCostFromBegin = nCostFromBegin;
						m_pParent = pParent;
						return true;
					}
				}

				return false;
			}

			//==================================
			// 判断是否比参数指定节点更靠近终点
			//==================================
			bool CompareClose(const Node* pNode)
			{
				return m_nCostTotal - m_nCostFromBegin < pNode->m_nCostTotal - pNode->m_nCostFromBegin;
			}
		};

		struct Region
		{
			Region() : m_pNextRegion(nullptr)
			{
				for( uint32 i = 0; i < eRegionWidth * eRegionWidth; i++ )
				{
					m_aryNode[i].m_xOffset = i % eRegionWidth;
					m_aryNode[i].m_yOffset = i / eRegionWidth;
				}
			}
			Node				m_aryNode[eRegionWidth*eRegionWidth];
			IndexType			m_vStartPos;
			Region*			m_pNextRegion;
		};

		uint32					m_nWidth;
		uint32					m_nDepth;
		uint32					m_nWidthInRegion;
		uint32					m_nCurSearchID;
		BarrierChecker			m_BarrierChecker;
		Region*					m_pFreeRegion;
		Region*					m_pUsingRegion;
		std::vector<Region*>	m_vecRegions;

	public:
		TGridData() 
			: m_nWidth(0)
			, m_nDepth(0)
			, m_nWidthInRegion(0)
			, m_nCurSearchID(0)
			, m_pFreeRegion(nullptr)
			, m_pUsingRegion(nullptr)
		{
		}

		~TGridData()
		{
			while( m_pFreeRegion )
			{
				Region* pRegion = m_pFreeRegion;
				m_pFreeRegion = pRegion->m_pNextRegion;
				delete pRegion;
			}
			GammaAst( m_pUsingRegion == nullptr );
		}

		//==================================
		// 得到指定格子节点
		//==================================
		Node* GetNode(const IndexType& node)
		{
			CostType xInRegion = node.x / eRegionWidth;
			CostType yInRegion = node.y / eRegionWidth;
			CostType nRegionIndex = yInRegion * m_nWidthInRegion + xInRegion;
			GammaAst(nRegionIndex < m_vecRegions.size());
			Region*& pRegion = m_vecRegions[nRegionIndex];
			if( pRegion == nullptr )
			{
				if( m_pFreeRegion == nullptr )
					m_pFreeRegion = new Region;
				pRegion = m_pFreeRegion;
				m_pFreeRegion = pRegion->m_pNextRegion;
				pRegion->m_pNextRegion = m_pUsingRegion;
				m_pUsingRegion = pRegion;
				pRegion->m_vStartPos.x = xInRegion*eRegionWidth;
				pRegion->m_vStartPos.y = yInRegion*eRegionWidth;
			}

			CostType xOffset = node.x % eRegionWidth;
			CostType yOffset = node.y % eRegionWidth;
			CostType nNodeIndex = yOffset * eRegionWidth + xOffset;
			Node* pNode = &pRegion->m_aryNode[nNodeIndex];
			if (pNode->m_nSearchID == m_nCurSearchID)
				return pNode;
			pNode->m_nSearchID = m_nCurSearchID;
			pNode->m_nOpenIndex = (uint32)Node::eNew;
			pNode->m_pParent = NULL;
			return pNode;
		}

		//==================================
		// 得到下一个子节点，
		// 返回子节点的索引，
		// 没有子节点返回-1
		//==================================
		int32 GetNextNode(const IndexType& nodeParent, int32 nChildIndex, IndexType& nodeChild)
		{
			static const IndexType nodeNext[8] =
			{
				IndexType(-1,  1), IndexType(0,  1), IndexType(1,  1),
				IndexType(-1,  0), 				      IndexType(1,  0),
				IndexType(-1, -1), IndexType(0, -1), IndexType(1, -1)
			};

			for (; nChildIndex < 8; nChildIndex++)
			{
				nodeChild = nodeParent + nodeNext[nChildIndex];
				if ((uint32)nodeChild.x < m_nWidth &&
					(uint32)nodeChild.y < m_nDepth &&
					m_BarrierChecker(nodeChild.x, nodeChild.y) == false)
					return nChildIndex;
			}

			return -1;
		}

		//==========================================
		// 调用A*核心算法在格子结构的地图上搜索路径
		//==========================================
		template<typename AStarAlgorithm>
		Node* SearchEndNode( 
			AStarAlgorithm& AStar, BarrierChecker funCheckBarrier,
			const IndexType& nodeStart, const IndexType& nodeEnd,
			uint32 nWidth, uint32 nDepth, uint32 nSearchDepth = 100000)
		{
			m_nCurSearchID++;
			m_BarrierChecker = funCheckBarrier;
			GammaAst( m_pUsingRegion == nullptr );

			if (nodeStart == nodeEnd)
				return NULL;

			// 0、重新初始化节点数组
			m_nWidthInRegion = AligenUp( nWidth, eRegionWidth ) / eRegionWidth;
			if (m_nWidth != nWidth || m_nDepth != nDepth)
			{
				m_nWidth = nWidth;
				m_nDepth = nDepth;
				m_vecRegions.clear();
				size_t nDepthInRegion = AligenUp( nDepth,eRegionWidth) / eRegionWidth;
				m_vecRegions.resize( m_nWidthInRegion * nDepthInRegion );
			}

			// 搜索路径
			Node* pResult = AStar.Search(nodeStart, nodeEnd, this, nSearchDepth);

			// 归还所有区域
			while( m_pUsingRegion )
			{
				Region* pCurRegion = m_pUsingRegion;
				m_pUsingRegion = pCurRegion->m_pNextRegion;
				pCurRegion->m_pNextRegion = m_pFreeRegion;
				m_pFreeRegion = pCurRegion;
				CostType xRegion = pCurRegion->m_vStartPos.x / eRegionWidth;
				CostType yRegion = pCurRegion->m_vStartPos.y / eRegionWidth;
				size_t nRegionIndex = yRegion * m_nWidthInRegion + xRegion;
				GammaAst( m_vecRegions[nRegionIndex] == pCurRegion );
				m_vecRegions[nRegionIndex] = nullptr;
			}
			return pResult;
		}

		//==========================================
		// 调用A*核心算法在格子结构的地图上搜索路径
		//==========================================
		template<typename AStarAlgorithm>
		uint32 Search( AStarAlgorithm& AStar, BarrierChecker funCheckBarrier,
			std::function<void( uint32 nIndex, const IndexType& )> funAddPathNode,
			const IndexType& nodeStart, const IndexType& nodeEnd,
			uint32 nWidth, uint32 nDepth, uint32 nSearchDepth = 100000)
		{
			// 1、搜索路径
			Node* pEndNode = SearchEndNode( AStar, funCheckBarrier, 
				nodeStart, nodeEnd, nWidth, nDepth, nSearchDepth);
			if (!pEndNode)
			{
				funAddPathNode( 0, nodeStart );
				funAddPathNode( 1, nodeEnd );
				return 2;
			}

			// 2、倒转路径
			Node* pCurNode = pEndNode;
			Node* pLeadNode = pCurNode->m_pParent;
			pCurNode->m_pParent = nullptr;
			while( pLeadNode )
			{
				Node* pParent = pLeadNode->m_pParent;
				pLeadNode->m_pParent = pCurNode;
				pCurNode = pLeadNode;
				pLeadNode = pParent;
			}

#ifdef DEBUG_AStar
			CDC* pDC = ::AfxGetMainWnd()->GetDC();
			CPen penLine( PS_SOLID, 1, 0x0000ffff );
			pDC->SelectObject( &penLine );
			pDC->MoveTo( pCurNode->m_Node );
#endif

			// 3、回调
			uint32 nCount = 0;
			while( pCurNode )
			{
#ifdef DEBUG_AStar
				pDC->LineTo( pCurNode->m_Node );
#endif
				funAddPathNode( nCount++, pCurNode->GetNode() );
				pCurNode = pCurNode->m_pParent;
			}

#ifdef DEBUG_AStar
			::AfxGetMainWnd()->ReleaseDC(pDC);
#endif
			return nCount;
		}

		//================================================
		// 调用A*核心算法在格子结构的地图上搜索目标最近点
		//================================================
		template<typename AStarAlgorithm>
		IndexType SearchNearDest( 
			AStarAlgorithm& AStar, BarrierChecker funCheckBarrier,
			const IndexType& nodeStart, const IndexType& nodeEnd,
			uint32 nWidth, uint32 nDepth, uint32 nSearchDepth = 100000)
		{
			Node* pEndNode = SearchEndNode( AStar, funCheckBarrier, 
				nodeStart, nodeEnd, nWidth, nDepth, nSearchDepth);
			return pEndNode ? pEndNode->GetNode() : nodeEnd;
		}
	};
}

#endif
