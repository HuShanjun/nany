
namespace Gamma
{    
	template<class Vector2>
	struct TPolygonPoint : public Vector2
	{
		typedef TPolygonPoint<Vector2> MyType;
		typedef std::multimap<double, MyType*> CSortPoints;
		typename CSortPoints::iterator	m_itSort;
		TPolygonPoint*					m_pPre;
		TPolygonPoint*					m_pNext;
	};

	struct SSortKey
	{
		double fBase;
		double fExtra;

		bool operator < ( const SSortKey& rhs ) const
		{
			return fBase == rhs.fBase ? fExtra < rhs.fExtra : fBase < rhs.fBase;
		}
	};

	//===============================================
	// 准备多边形  
	//===============================================
	template<class Iterator, class DataType, class CPolygonPoint, class CSortPoints>
	inline void PrepareConvex( Iterator itBegin, Iterator itEnd, CSortPoints& mapSortOnY )
	{
		typedef TVector2<DataType> CVector2T;
		CPolygonPoint* pFirst = NULL;
		CPolygonPoint* pCur = NULL;

		//===============================================
		// 将点按Y的大小排列，并且一个点连一个点，形成多边形  
		//===============================================
		while( itBegin != itEnd )
		{
			CPolygonPoint* pPoint = new CPolygonPoint;
			*(CVector2T*)pPoint = (CVector2T)( *itBegin ); ++itBegin;
			pPoint->m_itSort = mapSortOnY.insert( std::pair<double, CPolygonPoint*>( pPoint->y, pPoint ) );
			pPoint->m_pPre = pCur;
			if( pCur )
				pCur->m_pNext = pPoint;
			else
				pFirst = pPoint;
			pCur = pPoint;
		}

		//===============================================
		// 最后一个点和最前面的点相连  
		//===============================================
		pCur->m_pNext = pFirst;
		pFirst->m_pPre = pCur;
	}

	//===============================================
	// 多边形分解为三角形  
	//===============================================
	template<class Iterator, class DataType, class ITriangleReciver>
	inline void TriangleDecomposition( Iterator itBegin, Iterator itEnd, ITriangleReciver& Reciver )
	{
		typedef TVector2<DataType> CVector2T;
		typedef TPolygonPoint<CVector2T> CPolygonPoint;
		typedef typename CPolygonPoint::CSortPoints CSortPoints;

		// 准备多边形  
		CSortPoints mapSortOnY;
		PrepareConvex<Iterator, DataType, CPolygonPoint, CSortPoints>( itBegin, itEnd, mapSortOnY );

		//===============================================
		// 生成三角形  
		//===============================================
		while( !mapSortOnY.empty() )
		{
			//===============================================
			// 从最高的点开始  
			//===============================================
			typename CSortPoints::reverse_iterator it = mapSortOnY.rbegin();
			CPolygonPoint* pCur = it->second;		// 最高的点  
			CPolygonPoint* pLeft = pCur->m_pPre;	// 左边的点  
			CPolygonPoint* pRight = pCur->m_pNext;	// 右边的点  
			// 以上三个点形成三角形A  

			// 包络  
			double fMinX = Min( Min( pCur->x, pLeft->x ), pRight->x );
			double fMaxX = Max( Max( pCur->x, pLeft->x ), pRight->x );
			double fMinY = Min( pLeft->y, pRight->y );
			double fK = -DBL_MAX;

			// 考察有没有其他点在三角形A内，  
			// 如果有，将这些点和左边的点连线，求每条线的斜率k，  
			// 收集斜率最大的点，并且将其按x的大小排列，(用斜率最大的直线进行三角形切割)  
			// 用直线相连这些点，并且将三角形A进行切割  
			std::map<SSortKey, CPolygonPoint*> mapSortOnX;
			if( pRight->m_pNext != pLeft )
			{
				while( ++it != mapSortOnY.rend() )
				{
					CPolygonPoint* pNextPoint = it->second;
					if( pNextPoint->y < fMinY )
						break;

					// 忽略掉和三角形三个顶点重合的点  
					if( *(CVector2T*)pNextPoint != *(CVector2T*)pCur &&
						*(CVector2T*)pNextPoint != *(CVector2T*)pRight &&
						*(CVector2T*)pNextPoint != *(CVector2T*)pLeft &&
						pNextPoint->x >= fMinX && pNextPoint->x <= fMaxX )
					{
						// 三角形A内  
						int32 nResult = IsInTriangle( *pNextPoint, *pLeft, *pCur, *pRight );
						if( nResult < 0 )
							continue;

						CPolygonPoint* pNextLeft = pNextPoint->m_pPre;
						CPolygonPoint* pNextRight = pNextPoint->m_pNext;
						// 此点对应的内角在下外角在上，忽略  
						if(	pNextRight->x >= pNextLeft->x )
							continue;

						// 求每条线的斜率k  
						double k = ( pNextPoint->y - pLeft->y )/( pNextPoint->x - pLeft->x );
						if( k < fK )
							continue;

						// 收集斜率最大的点  
						if( k > fK && !mapSortOnX.empty() )
							mapSortOnX.clear();
						fK = k;

						CVector2T vMidPos = ( *(CVector2T*)pNextRight + *(CVector2T*)pNextLeft )/2;
						CVector2T vDel = vMidPos - *pNextPoint;
						SSortKey Key = { pNextPoint->x, vDel.x/vDel.y };
						mapSortOnX[Key] = pNextPoint;
					}
				}
			}

			// 三角形A不需要切割，直接形成三角形  
			// 同时删除当前点，如果三角形A和其他三角形没有连接，则三个点一起删除  
			if( mapSortOnX.empty() )
			{
				Reciver.AddTriangle( *pCur, *pRight, *pLeft );

				if( pRight->m_pNext != pLeft )
				{
					pLeft->m_pNext = pRight;
					pRight->m_pPre = pLeft;
					mapSortOnY.erase( pCur->m_itSort );
					delete pCur;
				}
				else
				{
					mapSortOnY.erase( pCur->m_itSort );
					mapSortOnY.erase( pLeft->m_itSort );
					mapSortOnY.erase( pRight->m_itSort );
					delete pCur;
					delete pLeft;
					delete pRight;
				}
			}
			// 三角形A需要切割，用起点为pLeft，斜率为fK的直线进行切割  
			// 切割过程会形成孤岛多边形或者三角形  
			else
			{
				double k1 = ( pCur->y - pRight->y )/( pCur->x - pRight->x );
				double d1 = pRight->y - k1*pRight->x;
				double d2 = pLeft->y - fK*pLeft->x;
				double x = ( d2 - d1 )/( k1 - fK );
				double y = k1*x + d1;

				// 三角形切割  
				//
				//           /\
				//          /  \
				//         /    \
				//        /      \
				//       ----------
				//      /  /\   /\ \
				//
				Reciver.AddTriangle( *pCur, CVector2T( (DataType)x, (DataType)y ), *pLeft );
				pCur->x = (DataType)x;
				pCur->y = (DataType)y;
				mapSortOnY.erase( pCur->m_itSort );
				pRight = pCur;
				pRight->m_itSort = mapSortOnY.insert( std::pair<double, CPolygonPoint*>( pRight->y, pRight ) );

				for( typename std::map<SSortKey, CPolygonPoint*>::iterator it = mapSortOnX.begin();
					it != mapSortOnX.end(); ++it )
				{
					CPolygonPoint* pCenter = it->second;

					if( pCenter->m_pNext != pLeft )
					{
						CPolygonPoint* pNew = new CPolygonPoint( *pCenter );
						pNew->m_itSort = mapSortOnY.insert( std::pair<float, CPolygonPoint*>( pNew->y, pNew ) );
						pLeft->m_pNext = pNew;
						pNew->m_pPre = pLeft;
						pNew->m_pNext->m_pPre = pNew;
					}
					else
					{
						// 处理以下情况  
						//
						//           /\
						//          /  \
						//         /    \
						//        /      \
						//       ----------
						//            \ /\ \
						//
						mapSortOnY.erase( pLeft->m_itSort );
						delete pLeft;
					}

					pLeft = pCenter;
				}

				pLeft->m_pNext = pRight;
				pRight->m_pPre = pLeft;
			}
		}
	}

	template<class Iterator, class DataType, class IRectangleReciver>
	inline void RectangleDecomposition( Iterator itBegin, Iterator itEnd, IRectangleReciver& Reciver )
	{
		typedef TVector2<DataType> CVector2T;
		typedef TPolygonPoint<CVector2T> CPolygonPoint;
		typedef typename CPolygonPoint::CSortPoints CSortPoints;

		// 准备多边形  
		CSortPoints mapSortOnY;
		PrepareConvex<Iterator, DataType, CPolygonPoint, CSortPoints>( itBegin, itEnd, mapSortOnY );

		//===============================================
		// 生成三角形  
		//===============================================
		while( !mapSortOnY.empty() )
		{
			//===============================================
			// 从最高的点开始  
			//===============================================
			typename CSortPoints::reverse_iterator it = mapSortOnY.rbegin();
			CPolygonPoint* pCur = it->second;
			CPolygonPoint* pTopLeft = pCur->m_pPre->y == pCur->y ? pCur->m_pPre : pCur;
			CPolygonPoint* pTopRight = pTopLeft->m_pNext;

			// 去掉相邻的同一高度的边  
			while( pTopRight->m_pNext->y == pTopLeft->y )
			{
				if( pTopLeft == pTopRight->m_pNext )
				{
					if( pTopLeft != pTopRight )
					{
						mapSortOnY.erase( pTopLeft->m_itSort );
						delete pTopLeft;
					}
					mapSortOnY.erase( pTopRight->m_itSort );
					SAFE_DELETE( pTopRight );
					break;
				}
				pTopLeft->m_pNext = pTopRight->m_pNext;
				pTopRight->m_pNext->m_pPre = pTopLeft;
				mapSortOnY.erase( pTopRight->m_itSort );
				delete pTopRight;
				pTopRight = pTopLeft->m_pNext;
			}

			if( !pTopRight )
				continue;

			// 去掉相邻的同一高度的边  
			while( pTopLeft->m_pPre->y == pTopRight->y )
			{
				if( pTopRight == pTopLeft->m_pPre )
				{
					if( pTopLeft != pTopRight )
					{
						mapSortOnY.erase( pTopLeft->m_itSort );
						delete pTopLeft;
					}
					mapSortOnY.erase( pTopRight->m_itSort );
					SAFE_DELETE( pTopRight );
					break;
				}

				pTopRight->m_pPre = pTopLeft->m_pPre;
				pTopLeft->m_pPre->m_pNext = pTopRight;
				mapSortOnY.erase( pTopLeft->m_itSort );
				delete pTopLeft;
				pTopLeft = pTopRight->m_pPre;
			}

			if( !pTopRight )
				continue;

			CPolygonPoint* pBottomLeft = pTopLeft->m_pPre;	// 左边的点  
			CPolygonPoint* pBottomRight = pTopRight->m_pNext;	// 右边的点  
			// 以上四个点形成矩形A  

			// 包络  
			DataType fMinX = pTopLeft->x;
			DataType fMaxX = pTopRight->x;
			DataType fMinY = Max( pBottomLeft->y, pBottomRight->y );

			// 考察有没有其他点在矩形A内，  
			// 收集y最大的点，并且将其按x的大小排列  
			// 用直线相连这些点，并且将矩形A进行水平切割  
			std::map<DataType, CPolygonPoint*> mapSortOnX;
			if( pBottomRight->m_pNext != pBottomLeft )
			{
				while( ++it != mapSortOnY.rend() )
				{
					CPolygonPoint* pNextPoint = it->second;
					if( pNextPoint->y < fMinY )
						break;

					// 忽略掉和矩形A四个顶点重合的点   
					if( *(CVector2T*)pNextPoint != *(CVector2T*)pTopLeft && 
						*(CVector2T*)pNextPoint != *(CVector2T*)pTopRight && 
						*(CVector2T*)pNextPoint != *(CVector2T*)pBottomLeft && 
						*(CVector2T*)pNextPoint != *(CVector2T*)pBottomRight && 
						pNextPoint->x >= fMinX && pNextPoint->x <= fMaxX )
					{
						mapSortOnX[pNextPoint->x] = pNextPoint;
						fMinY = pNextPoint->y;
					}
				}
			}

			Reciver.AddRectangle( fMinX, fMinY, fMaxX, pTopLeft->y );

			// 矩形切割  
			//
			//     -------------------	         -------------------		          -------------------
			//    | 				  |	        |                   |	             |                   |
			//    | 				  |	        |                   |	             |                   |
			//    |-------------------|	        |------------------------      --------------------------| 
			//    | |    |            |	        |   |    |          		   |   |    |          		 |
			//    |	|    |			  |	        |   |    |          	       |   |    |          		 |
			//
			if( fMinY == pBottomLeft->y )
			{
				mapSortOnY.erase( pTopLeft->m_itSort );
				delete pTopLeft;
				pTopLeft = pBottomLeft;
			}
			else
			{
				mapSortOnY.erase( pTopLeft->m_itSort );
				pTopLeft->y = fMinY;
				pTopLeft->m_itSort = mapSortOnY.insert( std::pair<double, CPolygonPoint*>( pTopLeft->y, pTopLeft ) );
			}

			if( fMinY == pBottomRight->y )
			{
				mapSortOnY.erase( pTopRight->m_itSort );
				delete pTopRight;
				pTopRight = pBottomRight;
			}
			else
			{
				mapSortOnY.erase( pTopRight->m_itSort );
				pTopRight->y = fMinY;
				pTopRight->m_itSort = mapSortOnY.insert( std::pair<double, CPolygonPoint*>( pTopRight->y, pTopRight ) );
			}

			pTopLeft->m_pNext = pTopRight;
			pTopRight->m_pPre = pTopLeft;

			for( typename std::map<DataType, CPolygonPoint*>::iterator it = mapSortOnX.begin();
				it != mapSortOnX.end(); ++it )
			{
				CPolygonPoint* pCenter = it->second;
				if( pCenter == pTopLeft )
					continue;
				if( pCenter == pTopRight )
					break;

				if( pCenter->m_pNext != pTopLeft )
				{
					CPolygonPoint* pNew = new CPolygonPoint( *pCenter );
					pNew->m_itSort = mapSortOnY.insert( std::pair<DataType, CPolygonPoint*>( pNew->y, pNew ) );
					pTopLeft->m_pNext = pNew;
					pNew->m_pPre = pTopLeft;
					pNew->m_pNext->m_pPre = pNew;
				}
				else
				{
					// 处理以下情况  
					//
					//           /\
					//          /  \
					//         /    \
					//        /      \
					//       ----------
					//            \ /\ \
					//
					mapSortOnY.erase( pTopLeft->m_itSort );
					delete pTopLeft;
				}

				pTopLeft = pCenter;
				pTopLeft->m_pNext = pTopRight;
				pTopRight->m_pPre = pTopLeft;
			}
		}
	}
}
