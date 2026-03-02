//=========================================================================
// TBinTree.h 
// 定义一个改进的二叉空间分割树，可以对非正方体的空间进行比较均匀的切割
// 柯达昭
// 2007-09-19
//=========================================================================
#ifndef __TBINARY_TREE_H__
#define __TBINARY_TREE_H__

#include <set>
#include "GammaCommon/GammaHelp.h"

namespace Gamma
{
    /** 定义一个改进的二叉空间分割树，可以对非正方体的空间进行比较均匀的切割
    @param NodeElem 叶子包含的对象类型
    @param MinSpace 不再分割的最小空间尺寸
    @param MaxElemPerLeaf 叶结点的最大元素数量
    @param ScaleType 数据类型,如float
    */
    template< class NodeElem, int32_t MinSpace, int32_t MaxElemPerLeaf, class ScaleType >
    class TBinTree
    {
    public:

        /// 叶结点中容纳NodeElem（元素节点）的容器
        class LeafElem
        {
            friend TBinTree;
            LeafElem( TBinTree*    pLeaf, const NodeElem& e, ScaleType x, ScaleType y, ScaleType z )
                : pLeaf(pLeaf), Elem(e), x(x), y(y), z(z){}
            TBinTree*    pLeaf;

        public:
            NodeElem     Elem;         ///< 指定要存储的节点
            ScaleType    x;            ///< NodeElem（元素节点）的x方向坐标
            ScaleType    y;            ///< NodeElem（元素节点）的y方向坐标
            ScaleType    z;            ///< NodeElem（元素节点）的z方向坐标

            void GetSpaceRange(    
                ScaleType& xMin,
                ScaleType& yMin,
                ScaleType& zMin,
                ScaleType& xMax,
                ScaleType& yMax,
                ScaleType& zMax ) const
            {
                xMin = pLeaf->m_xMin;
                yMin = pLeaf->m_yMin;
                zMin = pLeaf->m_zMin;
                xMax = pLeaf->m_xMax;
                yMax = pLeaf->m_yMax;
                zMax = pLeaf->m_zMax;
            }
        };

        /// 此二叉树的迭代器，用于遍历查找和删除
        class iterator
        {
			LeafElem* pLeafElem;
			iterator( LeafElem* pLeafElem ) : pLeafElem( pLeafElem ){};
            friend TBinTree;
		public:
			iterator() : pLeafElem(NULL){}
			iterator( const iterator& it ) : pLeafElem( it.pLeafElem ){};

            operator bool    ()                      const    { return pLeafElem != 0; }
            const LeafElem* operator-> ()            const    { return (&**this);    }
            const LeafElem& operator * ()            const    { return *pLeafElem; }
            bool  operator < ( const iterator& it )  const    { return pLeafElem->Elem < it.pLeafElem->Elem; }
        };

    private:
        /// 划分方式
        enum eDivType{ eNone, eX, eY, eZ };
        typedef std::set<iterator> LeafNodeSet;
        struct Chidren{ TBinTree* _child[2]; };

        ScaleType            m_xMin;
        ScaleType            m_yMin;
        ScaleType            m_zMin;
        ScaleType            m_xMax;
        ScaleType            m_yMax;
        ScaleType            m_zMax;
        eDivType             m_eType;
        TBinTree*            m_pParent;
        void*                m_pNode;

        /// 是否叶结点
        bool _isleaf()    
        { 
            return m_eType == eNone;
        }

        /// 得到叶结点
        LeafNodeSet* _node()    
        { 
            return (LeafNodeSet*)m_pNode; 
        }

        /// 得到子树
        Chidren* _children()    
        { 
            return (Chidren*)m_pNode; 
        }

        /// 插入一个值，不能重复插入
        iterator _insert( const NodeElem& Node, ScaleType x, ScaleType y, ScaleType z, LeafElem* pBuf )
        {
            //如果当前节点是叶结点
            if( _isleaf() )
            {
                LeafNodeSet* pNode = _node();
                LeafElem leTemp( this, Node, 0, 0, 0 );
                GammaAst( pNode->find( iterator( &leTemp ) ) == pNode->end() );

                if( !pBuf )
                    pBuf = new LeafElem( this, Node, x, y, z );
                else
                    *pBuf = LeafElem( this, Node, x, y, z );

                //如果叶结点中NodeElem（元素节点）的数量小于指定的最大叶结点元素个数
                //直接插入并且返回
                if( pNode->size() < MaxElemPerLeaf )
                    return *pNode->insert( iterator( pBuf ) ).first;

                //否则叶结点进行分裂
                ScaleType xDel = m_xMax - m_xMin;   
                ScaleType yDel = m_yMax - m_yMin;   
                ScaleType zDel = m_zMax - m_zMin; 
                if( xDel <= MinSpace && yDel <= MinSpace && zDel <= MinSpace )
                    return *pNode->insert( iterator( pBuf ) ).first;

                m_pNode = new Chidren; 
                //如果x方向最长，则进行x方向的分裂
                if( xDel > yDel && xDel > zDel )
                {
                    m_eType = eX;
                    _children()->_child[0] = new TBinTree( this, m_xMin, m_yMin, m_zMin, ( m_xMin + m_xMax )/2, m_yMax, m_zMax );
                    _children()->_child[1] = new TBinTree( this, ( m_xMin + m_xMax )/2, m_yMin, m_zMin, m_xMax, m_yMax, m_zMax );    
                    
                    for( LeafNodeSet::iterator it = pNode->begin(); it != pNode->end(); ++it )
                    {
                        (*it).pLeafElem->pLeaf = _children()->_child[ (*it)->x >= ( m_xMin + m_xMax )/2 ];
                        (*it)->pLeaf->_node()->insert( *it );    
                    }
                }
                //如果y方向最长，则进行y方向的分裂
                else if( yDel > zDel )
                {
                    m_eType = eY;
                    _children()->_child[0] = new TBinTree( this, m_xMin, m_yMin, m_zMin, m_xMax, ( m_yMin + m_yMax )/2, m_zMax );
                    _children()->_child[1] = new TBinTree( this, m_xMin, ( m_yMin + m_yMax )/2, m_zMin, m_xMax, m_yMax, m_zMax );    
                    for( LeafNodeSet::iterator it = pNode->begin(); it != pNode->end(); ++it )
                    {
                        (*it).pLeafElem->pLeaf = _children()->_child[ (*it)->y >= ( m_yMin + m_yMax )/2 ];
                        (*it)->pLeaf->_node()->insert( *it );    
                    }    
                }
                //如果z方向最长，则进行z方向的分裂
                else
                {
                    m_eType = eZ;
                    _children()->_child[0] = new TBinTree( this, m_xMin, m_yMin, m_zMin, m_xMax, m_yMax, ( m_zMin + m_zMax )/2 );
                    _children()->_child[1] = new TBinTree( this, m_xMin, m_yMin, ( m_zMin + m_zMax )/2, m_xMax, m_yMax, m_zMax );    
                    for( LeafNodeSet::iterator it = pNode->begin(); it != pNode->end(); ++it )
                    {
                        (*it).pLeafElem->pLeaf = _children()->_child[ (*it)->z >= ( m_zMin + m_zMax )/2 ];
                        (*it)->pLeaf->_node()->insert( *it );    
                    }    
                }

                //删除叶结点内容
                delete pNode;
                return _insert( Node, x, y, z, pBuf );
            }
            //如果不是叶结点则根据此元素所在位置插入相应的子节点
            else if( m_eType == eX )
                return _children()->_child[ x >= ( m_xMin + m_xMax )/2 ]->_insert( Node, x, y, z, pBuf );
            else if( m_eType == eY )
                return _children()->_child[ y >= ( m_yMin + m_yMax )/2 ]->_insert( Node, x, y, z, pBuf );
            else
                return _children()->_child[ z >= ( m_zMin + m_zMax )/2 ]->_insert( Node, x, y, z, pBuf );
        }

        /// 检查合并节点数量不足的子树
        void _checknode( TBinTree* pTreeNode )
        {
            while( pTreeNode )
            {
                Chidren* pChidren = pTreeNode->_children();
                if( !pChidren->_child[0]->_isleaf() ||
                    !pChidren->_child[1]->_isleaf() )
                    break;

                if( pChidren->_child[0]->_node()->size() + pChidren->_child[1]->_node()->size() > MaxElemPerLeaf )
                    return;

                LeafNodeSet* pNode = new LeafNodeSet;
                for( LeafNodeSet::iterator it = pChidren->_child[0]->_node()->begin(); it != pChidren->_child[0]->_node()->end(); ++it )
                {
                    LeafNodeSet::_Pairib ib = pNode->insert( *it );
                    (*ib.first).pLeafElem->pLeaf = pTreeNode;
                    GammaAst( ib.second );
                }

                for( LeafNodeSet::iterator it = pChidren->_child[1]->_node()->begin(); it != pChidren->_child[1]->_node()->end(); ++it )
                {
                    LeafNodeSet::_Pairib ib = pNode->insert( *it );
                    (*ib.first).pLeafElem->pLeaf = pTreeNode;
                    GammaAst( ib.second );                
                }

                delete pChidren->_child[0];
                delete pChidren->_child[1];
                delete pChidren;
                pTreeNode->m_eType = eNone;
                pTreeNode->m_pNode = pNode;
                pTreeNode = pTreeNode->m_pParent;
            }
        }

    public:
        TBinTree( 
            TBinTree* pParent, 
            ScaleType xMin,
            ScaleType yMin,
            ScaleType zMin,
            ScaleType xMax,
            ScaleType yMax,
            ScaleType zMax )
            : m_xMin( xMin )
            , m_yMin( yMin )
            , m_zMin( zMin )
            , m_xMax( xMax )
            , m_yMax( yMax )
            , m_zMax( zMax )
            , m_pParent( pParent )
            , m_eType( eNone )
            , m_pNode( new LeafNodeSet() )
        {
        }

        ~TBinTree(void)
        {
            if( _isleaf() )
                delete _node();
            else
            {
                Chidren* pChild = _children();
                delete pChild->_child[0];
                delete pChild->_child[1];
                delete pChild;
            }
        }

        bool empty()
        {
            return _isleaf() && _node()->empty();
        }

        /// 查找是否某个值，非常慢
        iterator find( const NodeElem& Node )
        {
            if( _isleaf() )
            {
                LeafNodeSet::iterator it = _node()->find( iterator( &LeafElem( this, Node, 0, 0, 0 ) ) );
                return it == _node()->end() ? iterator( NULL ) : *it;
            }
            else
            {
                iterator it = _children()->_child[0]->find( Node );
                return it ? it : _children()->_child[1]->find( Node );
            }
        }

        /// 插入一个节点，不允许重复插入
        iterator insert( const NodeElem& Node, ScaleType x, ScaleType y, ScaleType z )
        {
            return _insert( Node, x, y, z, NULL );
        }

        /// 删除一个节点
        void erase( iterator it )
        {
            if( !it.pLeafElem )
                return;

            TBinTree* pParent = (*it).pLeaf->m_pParent;
            (*it).pLeaf->_node()->erase( it );
            delete it.pLeafElem;
            _checknode( pParent );
        }

        /// 将节点移动到新的位置
        void move( iterator it, ScaleType x, ScaleType y, ScaleType z )
        {
            if( !it.pLeafElem )
                return;

            if( x >= (*it).pLeaf->m_xMin && x < (*it).pLeaf->m_xMax && 
                y >= (*it).pLeaf->m_yMin && y < (*it).pLeaf->m_yMax && 
                z >= (*it).pLeaf->m_zMin && z < (*it).pLeaf->m_zMax )
            {
                it.pLeafElem->x = x;
                it.pLeafElem->y = y;
                it.pLeafElem->z = z;
            }
            else
            {
                TBinTree* pParent = (*it).pLeaf->m_pParent;
                (*it).pLeaf->_node()->erase( it );
                _insert( (*it).Elem, x, y, z, it.pLeafElem );
                _checknode( pParent );
            }
        }

        /// 遍历一定范围内的节点
        template< class FuncType, class ContextType >
        void enumnode( ScaleType xMin, 
            ScaleType yMin, 
            ScaleType zMin, 
            ScaleType xMax, 
            ScaleType yMax, 
            ScaleType zMax, 
            FuncType Fun, 
            ContextType context )
        {
            if( _isleaf() )
            {
                LeafNodeSet* pNode = _node();
                for( LeafNodeSet::iterator it = pNode->begin(); it != pNode->end(); ++it )
                {
                    if( (*it)->x >= xMin && (*it)->x < xMax && (*it)->y >= yMin && (*it)->y < yMax && (*it)->z >= zMin && (*it)->z < zMax )
                        Fun( *it, context );
                }
            }
            else
            {
				xMin = Limit( xMin, m_xMin, m_xMax );
				xMax = Limit( xMax, m_xMin, m_xMax );
				yMin = Limit( yMin, m_yMin, m_yMax );
				yMax = Limit( yMax, m_yMin, m_yMax );
				zMin = Limit( zMin, m_zMin, m_zMax );
				zMax = Limit( zMax, m_zMin, m_zMax );

				if( xMin == xMax || yMin == yMax || zMin == zMax )
					return;

				_children()->_child[0]->enumnode( xMin, yMin, zMin, xMax, yMax, zMax, Fun, context );
				_children()->_child[1]->enumnode( xMin, yMin, zMin, xMax, yMax, zMax, Fun, context );
            }
        }
    };
}

#endif