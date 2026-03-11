//=========================================================================================
// TTinyList.h 
// 定义一个双向List类
// 优点：插入和删除节点不需要进行内存的分配和释放，节点操作速度极快
//		 节点可以自删除，不需要知道所处的链
// 缺点：数据需要从指定的节点类继承
// 柯达昭
// 2008-02-27
//=========================================================================================
#ifndef __TGAMMA_LIST_H__
#define __TGAMMA_LIST_H__

#include "GammaCommon/GammaHelp.h"

namespace Gamma
{

	template<typename ImpClass>
	class TGammaList
	{
		const TGammaList& operator= ( const TGammaList& );
		TGammaList( const TGammaList& );
	public:
		typedef ImpClass CNode;
		class CGammaListNode
		{
			CGammaListNode*	m_pPreNode;
			CGammaListNode*	m_pNextNode;
			friend class TGammaList<CNode>;
#ifdef _DEBUG
			int* m_pSize;
#endif

		public:
			CGammaListNode() : m_pPreNode(NULL), m_pNextNode(NULL)
#ifdef _DEBUG
				,m_pSize(NULL)
#endif
			{
			}

			~CGammaListNode()
			{ 
				Remove();
			}

			bool IsInList() const
			{ 
				return m_pPreNode != NULL; 
			}
			//只是从链表中脱离  delete需要自己调用
			void Remove()
			{
				if( !IsInList() )
					return;
				m_pPreNode->m_pNextNode = m_pNextNode;
				m_pNextNode->m_pPreNode = m_pPreNode;
				m_pPreNode = NULL;
				m_pNextNode = NULL;
#ifdef _DEBUG
				if (m_pSize)
					--(*m_pSize);
#endif
			}

			CNode* GetPre() const
			{
				return m_pPreNode && m_pPreNode->m_pPreNode ? static_cast<CNode*>( m_pPreNode ) : NULL;
			}

			CNode* GetNext() const
			{
				return m_pNextNode && m_pNextNode->m_pNextNode ? static_cast<CNode*>( m_pNextNode ) : NULL;
			}

			void InsertBefore( CGammaListNode& Node )
			{
				GammaAst( !IsInList() );
				GammaAst( &Node != this );
				m_pPreNode = Node.m_pPreNode;
				m_pNextNode = &Node;
				Node.m_pPreNode->m_pNextNode = this;
				Node.m_pPreNode = this;
#ifdef _DEBUG
				if (m_pSize)
					++(*m_pSize);
#endif
			}

			void InsertAfter( CGammaListNode& Node )
			{
				GammaAst( !IsInList() );
				GammaAst( &Node != this );
				m_pNextNode = Node.m_pNextNode;
				m_pPreNode = &Node;
				Node.m_pNextNode->m_pPreNode = this;
				Node.m_pNextNode = this;
#ifdef _DEBUG
				if (m_pSize)
					++(*m_pSize);
#endif
			}
		};

	public:
		class iterator
		{
			CNode* m_pNode;
		public:
			iterator() : m_pNode( NULL ){}
			iterator( CNode* pNode ) : m_pNode( pNode ){}
			iterator( const iterator& rhs ) : m_pNode( rhs.m_pNode ){}
			iterator operator= ( CNode* pNode ) { m_pNode = pNode; return *this; }
			iterator operator= ( const iterator& rhs ) { m_pNode = rhs.m_pNode; return *this; }
			bool operator == ( CNode* pNode ) const { return m_pNode == pNode; }
			bool operator == ( const iterator& rhs ) const { return m_pNode == rhs.m_pNode; }
			bool operator != ( CNode* pNode ) const { return m_pNode != pNode; }
			bool operator != ( const iterator& rhs ) const { return m_pNode != rhs.m_pNode; }
			iterator& operator++() { m_pNode = m_pNode ? m_pNode->CGammaListNode::GetNext() : NULL; return *this; }
			iterator operator++( int ) { iterator i = *this; ++*this; return i; }
			iterator& operator--() { m_pNode = m_pNode ? m_pNode->CGammaListNode::GetPre() : NULL; return *this; }
			iterator operator--( int ) { iterator i = *this; ++*this; return i; }
			CNode& operator* () const { return *m_pNode; }
		};

		TGammaList()
		{
			m_NodeHead.m_pNextNode = &m_NodeTail;
			m_NodeTail.m_pPreNode = &m_NodeHead;
#ifdef _DEBUG
			m_size = 0;
#endif
		}

		~TGammaList()
		{
			GammaAst( IsEmpty() );
			m_NodeHead.m_pNextNode = NULL;
			m_NodeTail.m_pPreNode = NULL;
		}

		bool IsEmpty() const
		{
			return m_NodeHead.m_pNextNode == &m_NodeTail;
		}

		uint32_t Size() const
		{
			uint32_t nSize = 0;
			for( CGammaListNode* pNode = GetFirst(); pNode; pNode = pNode->GetNext() )
				nSize++;
			return nSize;
		}

		bool IsInList( CGammaListNode& Node ) const
		{
			if( !Node.IsInList() )
				return false;			
			for( auto pNode = GetFirst(); pNode; pNode = pNode->GetNext() )
				if( pNode == &Node )
					return true;
			return false;
		}

		void PushFront( CGammaListNode& Node )
		{
			InsertAfter( Node, NULL );
		}

		void PushBack( CGammaListNode& Node )
		{
			InsertBefore( Node, NULL );
		}

		void InsertBefore( CGammaListNode& Node, CGammaListNode* pNodePos )
		{
			GammaAst( !Node.IsInList() );
			GammaAst( &Node != pNodePos );

			pNodePos = pNodePos ? pNodePos : &m_NodeTail;
			Node.m_pPreNode  = pNodePos->m_pPreNode;
			Node.m_pNextNode = pNodePos;
			pNodePos->m_pPreNode->m_pNextNode = &Node;
			pNodePos->m_pPreNode = &Node;
#ifdef _DEBUG
			m_size++;
			Node.m_pSize = &m_size;
#endif
		}

		void InsertAfter( CGammaListNode& Node, CGammaListNode* pNodePos )
		{
			GammaAst( !Node.IsInList() );
			GammaAst( &Node != pNodePos );

			pNodePos = pNodePos ? pNodePos : &m_NodeHead;
			Node.m_pNextNode  = pNodePos->m_pNextNode;
			Node.m_pPreNode = pNodePos;
			pNodePos->m_pNextNode->m_pPreNode = &Node;
			pNodePos->m_pNextNode = &Node;
#ifdef _DEBUG
			m_size++;
			Node.m_pSize = &m_size;
#endif
		}

		CNode* GetFirst() const
		{
			return IsEmpty() ? NULL : static_cast<CNode*>( m_NodeHead.m_pNextNode );
		}

		CNode* GetLast() const
		{
			return IsEmpty() ? NULL : static_cast<CNode*>( m_NodeTail.m_pPreNode );
		}
		
		iterator begin()
		{
			return iterator( GetFirst() );
		}

		iterator end()
		{
			return iterator();
		}

		iterator rbegin()
		{
			return iterator( GetLast() );
		}

		iterator rend()
		{
			return iterator();
		}
	private:
		CGammaListNode	m_NodeHead;
		CGammaListNode	m_NodeTail;
#ifdef _DEBUG
		int m_size;
#endif
	};

	template<typename DataType>
	class TGammaListNode : 
		public TGammaList< TGammaListNode<DataType> >::CGammaListNode
	{
		DataType m_Data;
	public:
		TGammaListNode() {}
		TGammaListNode( const DataType& v ) : m_Data( v ) {}
		const DataType& Get() { return m_Data; }
		void Set( const DataType& v ) { m_Data = v; }
	};
}

#endif
