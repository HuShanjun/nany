#pragma once
#include "GammaCommon/TVector2.h"

#define MAXDIS 2048

namespace Gamma
{

	enum EOptimizeLevel
	{
		eOL_Null		= 0,
		eOL_Normal		= 1,
		eOL_Ultra		= 2
	};


	template< class T, int BUFSIZE = 100 >
	class CSearcher
	{
		T*			m_MapMask;

		CPos16	m_EndPos;
		CPos16	m_PathBuf[BUFSIZE];
		CPos16	m_LeftBuf[BUFSIZE/2 + 2];
		CPos16	m_RightBuf[BUFSIZE/2 + 2];
		int32_t	m_nNearest;
		int32_t	m_nNrstJugde;
		int32_t	m_nBufSize;

		struct ____
		{
			T*			m_MapMask;
			CPos16*	m_pPath;
			int*		m_pStep;
			CPos16*   m_pBarrier;

			bool Do( int32_t x, int32_t y )
			{
				if( m_MapMask->IsBarrier( x, y ) ) 
				{
					*m_pBarrier = CPos16( (int16_t)x, (int16_t)y );
					return false;
				}
				else if( m_pPath )
				{
					m_pPath[(*m_pStep)++] = CPos16( (int16_t)x, (int16_t)y );
				}
				return true;
			}
		};

		bool IsSeeThePoint( CPos16& SrcPos, CPos16& DestPos, CPos16& Barrier, CPos16* pPath = NULL, int* pStep = NULL )
		{
			____ A = { m_MapMask, pPath, pStep, &Barrier };
			if( pPath )
				pPath[(*pStep)++] = SrcPos;

			return LineTo( SrcPos.x, SrcPos.y, DestPos.x, DestPos.y, A );
		}

		bool FindNextPos( CPos16& CurPos, CPos16& CrossPos, CPos16& Barrier, bool bRight )
		{
			const CPos16 Direct[8] = 
			{ 
				CPos16(  1,  1 ),CPos16(  1,  0 ),CPos16(  1, -1 ),CPos16(  0, -1 ),
				CPos16( -1, -1 ),CPos16( -1,  0 ),CPos16( -1,  1 ),CPos16(  0,  1 )
			};
			char Inver[3][3] = { { 4, 3, 2 }, { 5, 8, 1 }, { 6, 7, 0 } };

			CPos16 Temp = Barrier - CurPos;

			int nDelta = bRight ? -1 : 1;
			int n = 0;
			for( int j = Inver[ Temp.y + 1 ][ Temp.x + 1 ] + 8; n < 8 ; j += nDelta, n++ )
			{
				CPos16 MovePos = CurPos + Direct[j&7];
				if( !m_MapMask->IsBarrier( MovePos.x, MovePos.y ) )
				{
					CurPos = CurPos + Direct[ ( j + ( bRight ? 1 : 7 ) )&7 ];
					if( m_MapMask->IsBarrier( CurPos.x, CurPos.y ) )
					{
						Barrier = CurPos;
						CrossPos = CurPos = MovePos;
					}
					else
						CrossPos = MovePos;
					return true;
				}
				Barrier = MovePos; 
			}

			return false;
		}

		int Judge( CPos16 Pos1, CPos16 Pos2 )
		{
			CPos16 Delta = Pos2 - Pos1;
			int x = abs( Delta.x );
			int y = abs( Delta.y );
			return ( x > y ) ? x + (y>>2): y + (x>>2);
		}

		int SearchFromCur( CPos16& CurPos, int CurStep )
		{
			CPos16 PreBar, Barrier;
			//CPos16 DeltaStep = m_EndPos - CurPos;

			if( IsSeeThePoint( CurPos, m_EndPos, Barrier, m_PathBuf, &CurStep ) )
			{
				m_PathBuf[CurStep++] = m_EndPos;
				return CurStep;
			}

			CPos16 posLeft  = m_PathBuf[ CurStep - 1 ];
			CPos16 posRight = m_PathBuf[ CurStep - 1 ];
			CPos16 barLeft  = Barrier;
			CPos16 barRight = Barrier;

			int nLeft = 0;
			int nRight = 0;
			int nMinJudge = 0x7fffffff;
			int nPos = 0;
			bool bLeft = true;

			for(;;)
			{
				CPos16 CrossPos;
				CPos16 posBar;
				int nJugeLeft	= Judge( posLeft, m_EndPos );
				int nJugeRight	= Judge( posRight, m_EndPos );
				if( nJugeLeft < nJugeRight )
				{
					if( nLeft + CurStep + 1 > m_nBufSize )
						return -1;
					if( nJugeLeft < nMinJudge )
					{
						nMinJudge	= nJugeLeft;
						nPos		= nLeft;
						bLeft		= true;
					}
					if( !FindNextPos( posLeft, CrossPos, barLeft, false ) )
						return -1;
					if( posLeft != CrossPos )
						m_LeftBuf [nLeft ++]	= CrossPos;
					m_LeftBuf [nLeft ++]	= posLeft;

					//防止绕边
					if( nLeft > m_nBufSize/2 )
						break;
				}
				else
				{
					if( nRight + CurStep + 1 > m_nBufSize )
						return -1;
					if( nJugeRight < nMinJudge )
					{
						nMinJudge	= nJugeRight;
						nPos		= nRight;
						bLeft		= false;
					}
					if( !FindNextPos( posRight, CrossPos, barRight, true ) )
						return -1;
					if( posRight != CrossPos )
						m_RightBuf[nRight++]	= CrossPos;
					m_RightBuf[nRight++]	= posRight;

					//防止绕边
					if( nRight > m_nBufSize/2 )
						break;
				}

				//if( nLeft + nRight > 1 && abs( posLeft.x - posRight.x ) + abs( posLeft.y - posRight.y ) < 2 )
				//	break;

				if( posLeft == posRight )
				{
					if( nRight > 1 )
					{
						CPos16 pos = posLeft;
						CPos16 bar = barLeft;
						FindNextPos( pos, CrossPos, bar, false );
						if( m_RightBuf[ nRight - 2 ] == CrossPos )
							break;
					}
					else if( nLeft > 1 )
					{
						CPos16 pos = posRight;
						CPos16 bar = barRight;
						FindNextPos( pos, CrossPos, bar, true );
						if( m_LeftBuf[ nLeft - 2 ] == CrossPos )
							break;
					}
				}
			}

			if( nMinJudge < m_nNrstJugde )
			{
				m_nNearest = CurStep + nPos;
				m_nNrstJugde = nMinJudge;
			}

			if( bLeft )
			{
				memcpy( m_PathBuf + CurStep, m_LeftBuf, ( nPos + 1 )*sizeof( CPos16 ) );
				return SearchFromCur( m_LeftBuf[nPos], CurStep + nPos + 1 );
			}
			else
			{
				memcpy( m_PathBuf + CurStep, m_RightBuf, ( nPos + 1 )*sizeof( CPos16 ) );
				return SearchFromCur( m_RightBuf[nPos], CurStep + nPos + 1 );
			}
		}

		int Optimize( int Step )
		{
			int i,j,k,n;
			CPos16 posBar;
			CPos16 PathBuf[BUFSIZE];

			j = 1;
			PathBuf[0] = m_PathBuf[Step - 1];
			for( i = Step - 1; i > 0; i = n, j++ )
			{
				for( n = 0; n < i - 1; n++ )
				{			
					if( IsSeeThePoint( m_PathBuf[n], m_PathBuf[i], posBar ) )
						break;
				}

				PathBuf[j] = m_PathBuf[n];
			}

			for( k = 0; k < j; k++ )
				m_PathBuf[k] = PathBuf[ j - 1 - k ];

			return j;
		}

		int UltraOptimize( int Step )
		{
			int i,j,k,n;
			CPos16 posBar;
			CPos16 PathBuf[BUFSIZE];
			CPos16 TempBuf[BUFSIZE];

			j = 0;
			for( i = Step - 1; i > 0; i = n )
			{
				int nTempStep=0;
				for( n = 0; n < i; n++ )
				{
					nTempStep=0;
					if( IsSeeThePoint( m_PathBuf[n], m_PathBuf[i], posBar, TempBuf, &nTempStep ) )
						break;
				}

				for( k = nTempStep - 1; k > 0; k--, j++ )
					PathBuf[j] = TempBuf[k];
			}

			for( k = 0; k < j; k++ )
				m_PathBuf[k] = PathBuf[ j - 1 - k ];
			Step = j;

			for( i = 0, j = 1; i< Step - 1; i += n, j++  )
			{
				for( n = Step - i - 1; n > 1; n-- )
				{			
					if( IsSeeThePoint( m_PathBuf[i], m_PathBuf[ i + n ], posBar ) )
						break;
				}
				m_PathBuf[j] = m_PathBuf[ i + n ];
			}

			return j;
		}


	public:
		CSearcher() : m_nBufSize(BUFSIZE){};

		void SetMaxBufSize( int32_t nBufSize )
		{
			m_nBufSize = Limit( nBufSize, 0, BUFSIZE );				
		}

		const CPos16* SearchRoad( T* pMapMask, CPos16 StartPos, CPos16 EndPos, 
			int32_t& StepNum, EOptimizeLevel eOptimize)
		{
			m_MapMask	  = pMapMask;
			m_EndPos      = EndPos;
			m_nNearest	  = 0;
			m_nNrstJugde  = 0x7fffffff;
			m_PathBuf[0]  = StartPos;

			if( !m_MapMask->IsBarrier( StartPos.x, StartPos.y ) )
				StepNum = SearchFromCur( StartPos, 1 );
			else
				StepNum = -1;

			if(StepNum == -1)
				StepNum = m_nNearest;

			if(StepNum == 0)
				StepNum = 1;
			else if( eOptimize == eOL_Normal )
				StepNum = Optimize( StepNum );
			else if( eOptimize == eOL_Ultra )
				StepNum = UltraOptimize( StepNum );
			return m_PathBuf;
		}

		CPos16 SearchRoad( T* pMapMask, CPos16 StartPos, CPos16 EndPos )
		{
			m_MapMask	  = pMapMask;
			m_EndPos      = EndPos;
			m_nNearest	  = 0;
			m_nNrstJugde  = 0x7fffffff;

			CPos16 Bar;
			m_PathBuf[0]  = StartPos;
			IsSeeThePoint( StartPos, EndPos, Bar, m_PathBuf, &m_nNearest );
			return m_PathBuf[ m_nNearest - 1 ];
		}
	};
}

