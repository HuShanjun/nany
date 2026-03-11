//=========================================================================================
// TSingleList.h 
// 定义一个单向List类
// 柯达昭
// 2016-02-14
//=========================================================================================
#ifndef __TSINGLE_LIST_H__
#define __TSINGLE_LIST_H__

#include "GammaCommon/GammaHelp.h"


namespace Gamma
{
	template<typename ImpClass>
	class TSingleList
	{
		TSingleList( const TSingleList& );
		const TSingleList& operator = ( const TSingleList& );
	public:		
		class CSingleListNode
		{
			ImpClass*	m_pNextNode;
			friend class TSingleList<ImpClass>;

		public:
			CSingleListNode() : m_pNextNode(NULL)
			{
			}

			~CSingleListNode()
			{ 
				GammaAst( !m_pNextNode );
			}

			ImpClass* GetNext() const
			{
				return m_pNextNode;
			}
		};

		void _InsertAfter( CSingleListNode& Node, CSingleListNode* pLearder )
		{
			GammaAst( !IsInList( Node ) );

			ImpClass*& pNode = pLearder ? pLearder->m_pNextNode : m_pNodeHead;
			Node.m_pNextNode = pNode;
			pNode = static_cast<ImpClass*>( &Node );
			m_pNodeTail = pLearder == m_pNodeTail ? pNode : m_pNodeTail; 
		}

	public:
		TSingleList() : m_pNodeHead(NULL), m_pNodeTail(NULL)
		{
		}

		~TSingleList()
		{
			GammaAst( !m_pNodeHead );
		}

		void InsertAfter( ImpClass& Node, ImpClass* pLearder )
		{
			_InsertAfter( Node, pLearder );
		}

		void PushFront( ImpClass& Node )
		{
			_InsertAfter( Node, NULL );
		}

		void PushBack( ImpClass& Node )
		{
			_InsertAfter( Node, m_pNodeTail );
		}

		ImpClass* RemoveFirst()
		{
			GammaAst( m_pNodeHead );
			CSingleListNode* pNode = m_pNodeHead;
			m_pNodeHead = pNode->m_pNextNode;
			pNode->m_pNextNode = NULL;
			if( m_pNodeHead == NULL )
				m_pNodeTail = NULL;
			return static_cast<ImpClass*>( pNode );
		}

		ImpClass* GetFirst() const
		{
			return m_pNodeHead;
		}

		ImpClass* GetLast() const
		{
			return m_pNodeTail;
		}

		bool IsInList( CSingleListNode& Node )
		{ 
			return Node.m_pNextNode != NULL || m_pNodeTail == &Node; 
		}

	private:
		ImpClass*	m_pNodeHead;
		ImpClass*	m_pNodeTail;
	};
}

#endif
