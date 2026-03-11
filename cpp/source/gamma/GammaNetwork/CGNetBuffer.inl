#include "CGNetwork.h"

namespace Gamma
{
    //======================================================
    // 网络消息接收缓冲区
    //======================================================
    inline CGNetRecvBuffer::CGNetRecvBuffer()
        : m_pBuffer( nullptr )
		, m_nBufSize( 0 )
        , m_nDataBegin( 0 )
        , m_nDataEnd( 0 )
    {
        Extend( 1 );
    }
	
	inline CGNetRecvBuffer::~CGNetRecvBuffer(void)
    {
	}

	inline void CGNetRecvBuffer::Extend( size_t nSize )
	{
        if( nSize <= m_nBufSize )
            return;
		nSize = AligenUp( (uint32_t)nSize, 8192 );
		void* pBuffer = ReserveMemoryPage( NULL, nSize );
        uint32_t nFlag = VIRTUAL_PAGE_READ | VIRTUAL_PAGE_WRITE;
		CommitMemoryPage( pBuffer, nSize, nFlag );
        if( m_pBuffer )
		{
			memcpy( pBuffer, m_pBuffer, m_nDataEnd );
			DecommitMemoryPage( m_pBuffer, m_nBufSize );
			FreeMemoryPage( m_pBuffer, m_nBufSize );
		}
		m_nBufSize = nSize;
		m_pBuffer = (char*)pBuffer;
	}

	inline size_t CGNetRecvBuffer::GetMaxBufferSize()
	{
		return m_nBufSize;
	}

    inline char* CGNetRecvBuffer::GetBuffer()
    {
        return m_pBuffer + m_nDataBegin;
    }

    inline size_t CGNetRecvBuffer::GetBufferSize()
    {
        return m_nDataEnd - m_nDataBegin;
    }

    inline char* CGNetRecvBuffer::GetLeftBuffer()
    {
        return m_pBuffer + m_nDataEnd;
    }

    inline size_t CGNetRecvBuffer::GetLeftSize()
    {
        return m_nBufSize - m_nDataEnd;
    }

    inline void CGNetRecvBuffer::Pop( size_t nSize )
    {
        m_nDataEnd = m_nDataEnd - m_nDataBegin - nSize;
        memmove( m_pBuffer, m_pBuffer + m_nDataBegin + nSize, m_nDataEnd );
        m_nDataBegin = 0;
    }

    inline void CGNetRecvBuffer::Push( size_t nSize )
    {
        m_nDataEnd += nSize;
	}
}
