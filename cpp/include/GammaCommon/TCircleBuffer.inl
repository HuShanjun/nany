 #include "TCircleBuffer.h"
namespace Gamma
{
	template<class BufferAlloc, bool bFreeEmptyBuffer>
	TCircleBuffer<BufferAlloc, bFreeEmptyBuffer>::TCircleBuffer( BufferAlloc _Alloc )
		: m_Alloc( _Alloc )
		, m_pReadBuffer( NULL )
		, m_pWriteBuffer( NULL )
		, m_nPushCount( 0 )
		, m_nPopCount( 0 )
	{
		// 至少两个buffer，否则当write回环追上read时，无法插入新的buffer
		m_pWriteBuffer = (SQuerryBuffer*)m_Alloc.Alloc();
		m_pWriteBuffer->m_pNextBuffer = (SQuerryBuffer*)m_Alloc.Alloc();
		m_pWriteBuffer->m_pNextBuffer->m_pNextBuffer = m_pWriteBuffer;
		m_pReadBuffer = m_pWriteBuffer;
		m_pWriteBuffer->m_nWritePos = 0;
		m_pWriteBuffer->m_nReadPos = 0;
	}

	template<class BufferAlloc, bool bFreeEmptyBuffer>
	TCircleBuffer<BufferAlloc, bFreeEmptyBuffer>::~TCircleBuffer()
	{
		SQuerryBuffer* pBuffer = m_pWriteBuffer;

		while( true )
		{
			SQuerryBuffer* pNextBuffer = pBuffer->m_pNextBuffer;
			m_Alloc.Free( pBuffer );
			if( pNextBuffer == m_pWriteBuffer )
				break;
			pBuffer = pNextBuffer;
		}
		m_pReadBuffer = NULL;
		m_pWriteBuffer = NULL;
	}

	template<class BufferAlloc, bool bFreeEmptyBuffer>
	inline void TCircleBuffer<BufferAlloc, bFreeEmptyBuffer>::WriteBuffer(
			SQuerryNodePtr& pWriteBuffer, uint32& nWritePos,
			SQuerryNodePtr pReadBuffer, const void* pBuffer, size_t nSize )
	{
		const tbyte* pCurBuffer = (const tbyte*)pBuffer;

		while( nSize )
		{
			uint32 nLeftSize = nNodeBufferSize - nWritePos;
			uint32 nWriteSize = (uint32)Min<size_t>( nSize, nLeftSize );
			tbyte* pTargetBuf = pWriteBuffer->m_aryBuffer + nWritePos;
			memcpy( pTargetBuf, pCurBuffer, nWriteSize );
			pCurBuffer += nWriteSize;
			nWritePos += nWriteSize;
			nSize -= nWriteSize;

			if( nWritePos != nNodeBufferSize )
			{
				SQuerryNodePtr pNextBuffer = pWriteBuffer->m_pNextBuffer;
				if( bFreeEmptyBuffer && pNextBuffer != pReadBuffer && pNextBuffer != pWriteBuffer)
				{
					SQuerryNodePtr pCheckBuffer = pNextBuffer->m_pNextBuffer;
					while( pCheckBuffer != pReadBuffer && pCheckBuffer != pWriteBuffer)
					{
						SQuerryNodePtr pFreeBuffer = pCheckBuffer;
						pCheckBuffer = pFreeBuffer->m_pNextBuffer;
						pNextBuffer->m_pNextBuffer = pCheckBuffer;
						m_Alloc.Free( pFreeBuffer );
					}
				}
				break;
			}

			if( pWriteBuffer->m_pNextBuffer == pReadBuffer )
			{
				SQuerryNodePtr pNewBuffer = (SQuerryBuffer*)m_Alloc.Alloc();
				pNewBuffer->m_pNextBuffer = pReadBuffer;
				pWriteBuffer->m_pNextBuffer = pNewBuffer;
			}

			pWriteBuffer = pWriteBuffer->m_pNextBuffer;
			nWritePos = 0;
		}
	}

	template<class BufferAlloc, bool bFreeEmptyBuffer>
	inline void TCircleBuffer<BufferAlloc, bFreeEmptyBuffer>::ReadBuffer (
		SQuerryNodePtr& pReadBuffer, uint32& nReadPos,
		void* pBuffer, size_t nSize )
	{
		tbyte* pCurBuffer = (tbyte*)pBuffer;
		while( nSize )
		{
			uint32 nWritePos = pReadBuffer->m_nWritePos;
			uint32 nLeftSize = nWritePos - nReadPos;
			GammaAst( nLeftSize > 0 );
			uint32 nReadSize = (uint32)nSize;
			if( nSize > nLeftSize )
			{
				GammaAst( nWritePos == nNodeBufferSize );
				nReadSize = nLeftSize;
			}
			const tbyte* pSrcBuf = pReadBuffer->m_aryBuffer + nReadPos;
			memcpy( pCurBuffer, pSrcBuf, nReadSize );
			pCurBuffer += nReadSize;
			nReadPos += nReadSize;
			nSize -= nReadSize;
			if( nReadPos != nNodeBufferSize )
				break;
			pReadBuffer = pReadBuffer->m_pNextBuffer;
			nReadPos = 0;
		}
	}

	template<class BufferAlloc, bool bFreeEmptyBuffer>
	template<class ContextType>
	inline void TCircleBuffer<BufferAlloc, bFreeEmptyBuffer>::PushBuffer( 
		ContextType nContext, const SSendBuf aryBuffer[], uint32 nBufferCount, bool bMerge )
	{
		SQuerryNodePtr pBufferStart = m_pWriteBuffer;
		SQuerryNodePtr pBufferEnd = m_pWriteBuffer;
		SQuerryNodePtr pReadBuffer = m_pReadBuffer;
		uint32 nWritePos = pBufferStart->m_nWritePos;
		WriteBuffer( pBufferEnd, nWritePos, pReadBuffer, &nContext, sizeof(nContext) );

		if( bMerge && nBufferCount )
		{
			size_t nCount = 1;
			WriteBuffer( pBufferEnd, nWritePos, pReadBuffer, &nCount, sizeof(uint32) );
			size_t nTotalSize = 0;
			for( uint32 i = 0; i < nBufferCount; i++ )
				nTotalSize += aryBuffer[i].second;
			WriteBuffer( pBufferEnd, nWritePos, pReadBuffer, &nTotalSize, sizeof(uint32) );
			for( uint32 i = 0; i < nBufferCount; i++ )
			{
				WriteBuffer( pBufferEnd, nWritePos, pReadBuffer, 
				aryBuffer[i].first, aryBuffer[i].second );
			}
		}
		else
		{
			WriteBuffer( pBufferEnd, nWritePos, pReadBuffer, &nBufferCount, sizeof(uint32) );
			for( uint32 i = 0; i < nBufferCount; i++ )
			{
				WriteBuffer( pBufferEnd, nWritePos, pReadBuffer, 
					&aryBuffer[i].second, sizeof(uint32) );
				WriteBuffer( pBufferEnd, nWritePos, pReadBuffer, 
					aryBuffer[i].first, aryBuffer[i].second );
			}
		}

		// 保证原子操作，一次query完整写入
		while( pBufferStart != pBufferEnd )
		{
			pBufferStart->m_nWritePos = nNodeBufferSize;
			pBufferStart = pBufferStart->m_pNextBuffer;
		}
		pBufferEnd->m_nWritePos = nWritePos;
		m_pWriteBuffer = pBufferEnd;
		m_nPushCount++;
	}

	template<class BufferAlloc, bool bFreeEmptyBuffer>
	template<class ContextType>
	inline bool TCircleBuffer<BufferAlloc, bFreeEmptyBuffer>::PopBuffer( 
		ContextType& nContext, SSendBuf aryBuffer[], uint32& nBufferCount )
	{
		SQuerryNodePtr pWriteBuffer = m_pWriteBuffer;
		uint32 nWritePos = pWriteBuffer->m_nWritePos;

		// 尚未写入完毕（当前已经写入结束的Buffer的位置不可能是nNodeBufferSize）
		if( nWritePos == nNodeBufferSize )
			return false;

		SQuerryNodePtr pBufferEnd = m_pReadBuffer;
		uint32 nReadPos = pBufferEnd->m_nReadPos;
		// 没有写入完整的数据可读
		if( pBufferEnd == pWriteBuffer && 
			nReadPos == nWritePos )
			return false;

		uint32 nCount;
		m_strBuffer.clear();
		ReadBuffer( pBufferEnd, nReadPos, &nContext, sizeof(nContext) );
		ReadBuffer( pBufferEnd, nReadPos, &nCount, sizeof(uint32) );
		GammaAst( nCount <= nBufferCount );
		nBufferCount = nCount;

		size_t nTotalSize = 0;
		for( uint32 i = 0; i < nBufferCount; i++ )
		{
			ReadBuffer( pBufferEnd, nReadPos, &aryBuffer[i].second, sizeof(uint32) );
			size_t nNeedSize = nTotalSize + aryBuffer[i].second;
			if( m_strBuffer.size() < nNeedSize )
				m_strBuffer.resize( nNeedSize );
			ReadBuffer( pBufferEnd, nReadPos, 
				&m_strBuffer[nTotalSize], (uint32)aryBuffer[i].second );
			nTotalSize = nNeedSize;
		}

		for( size_t i = 0, n = 0; i < nBufferCount; n += aryBuffer[i++].second )
			aryBuffer[i].first = &m_strBuffer[n];

		// 保证原子操作，一次query完整读出
		pBufferEnd->m_nReadPos = nReadPos;
		m_pReadBuffer = pBufferEnd;
		m_nPopCount++;
		return true;
	}

	template<class BufferAlloc, bool bFreeEmptyBuffer>
	void TCircleBuffer<BufferAlloc, bFreeEmptyBuffer>::PushRaw( 
		const SSendBuf aryBuffer[], uint32 nBufferCount )
	{
		if( nBufferCount == 0 )
			return;
		SQuerryNodePtr pBufferStart = m_pWriteBuffer;
		SQuerryNodePtr pBufferEnd = m_pWriteBuffer;
		SQuerryNodePtr pReadBuffer = m_pReadBuffer;
		uint32 nWritePos = pBufferStart->m_nWritePos;

		for( uint32 i = 0; i < nBufferCount; i++ )
		{
			WriteBuffer( pBufferEnd, nWritePos, pReadBuffer,
				aryBuffer[i].first, aryBuffer[i].second );
		}

		// 保证原子操作，一次query完整写入
		while( pBufferStart != pBufferEnd )
		{
			pBufferStart->m_nWritePos = nNodeBufferSize;
			pBufferStart = pBufferStart->m_pNextBuffer;
		}
		pBufferEnd->m_nWritePos = nWritePos;
		m_pWriteBuffer = pBufferEnd;
		m_nPushCount++;
	}

	template<class BufferAlloc, bool bFreeEmptyBuffer>
	void TCircleBuffer<BufferAlloc, bFreeEmptyBuffer>::PushRaw(
		const void* pBuffer, uint32 nBufferSize )
	{
		SSendBuf SendBuff( pBuffer, nBufferSize );
		PushRaw( &SendBuff, 1 );
	}

	template<class BufferAlloc, bool bFreeEmptyBuffer>
	uint32 TCircleBuffer<BufferAlloc, bFreeEmptyBuffer>::PopRaw(
		void* pBuffer, uint32 nBufferSize )
	{
		SQuerryNodePtr pWriteBuffer = m_pWriteBuffer;
		uint32 nWritePos = pWriteBuffer->m_nWritePos;

		// 尚未写入完毕（当前已经写入结束的Buffer的位置不可能是nNodeBufferSize）
		if( nWritePos == nNodeBufferSize )
			return 0;

		SQuerryNodePtr pReadBuffer = m_pReadBuffer;
		uint32 nReadPos = pReadBuffer->m_nReadPos;
		// 没有写入完整的数据可读
		if( pReadBuffer == pWriteBuffer &&
			nReadPos == nWritePos )
			return 0;

		tbyte* pCurBuffer = (tbyte*)pBuffer;
		while( nBufferSize )
		{
			uint32 nWritePos = pReadBuffer->m_nWritePos;
			uint32 nLeftSize = nWritePos - nReadPos;
			if( nLeftSize == 0 )
				break;
			uint32 nReadSize = Min( nBufferSize, nLeftSize );
			const tbyte* pSrcBuf = pReadBuffer->m_aryBuffer + nReadPos;
			memcpy( pCurBuffer, pSrcBuf, nReadSize );
			pCurBuffer += nReadSize;
			nReadPos += nReadSize;
			nBufferSize -= nReadSize;
			if( nReadPos != nNodeBufferSize )
				break;
			pReadBuffer = pReadBuffer->m_pNextBuffer;
			nReadPos = 0;
		}

		// 保证原子操作，一次query完整读出
		pReadBuffer->m_nReadPos = nReadPos;
		m_pReadBuffer = pReadBuffer;
		m_nPopCount++;
		return (uint32)( pCurBuffer - (tbyte*)pBuffer );
	}

	template<class BufferAlloc, bool bFreeEmptyBuffer>
	bool Gamma::TCircleBuffer<BufferAlloc, bFreeEmptyBuffer>::CanPop() const
	{
		SQuerryNodePtr pWriteBuffer = m_pWriteBuffer;
		size_t nWritePos = pWriteBuffer->m_nWritePos;

		// 尚未写入完毕（当前已经写入结束的Buffer的位置不可能是nNodeBufferSize）
		if( nWritePos == nNodeBufferSize )
			return false;

		SQuerryNodePtr pBufferEnd = m_pReadBuffer;
		size_t nReadPos = pBufferEnd->m_nReadPos;
		// 没有写入完整的数据可读
		if( pBufferEnd == pWriteBuffer &&
			nReadPos == nWritePos )
			return false;
		return true;
	}

	template<class BufferAlloc, bool bFreeEmptyBuffer>
	uint32 Gamma::TCircleBuffer<BufferAlloc, bFreeEmptyBuffer>::GetWaitingBufferCount() const
	{
		return (uint32)( m_nPushCount - m_nPopCount );
	}
}
