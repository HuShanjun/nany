//=========================================================================
// CGNetBuffer.h 
// 定义网络接收Buffer类
// 柯达昭
// 2007-12-05
//=========================================================================
#ifndef __GAMMA_NETBUFFER_H__
#define __GAMMA_NETBUFFER_H__

#include "GammaCommon/GammaMemory.h"
#include "GammaNetwork/GammaNetworkBase.h"

namespace Gamma
{

    //======================================================
    // 网络消息接收缓冲区
    // 缓冲区大小不在类里面保存，
    // 因为所有接收缓冲区的大小都是一样的，
    // 保存在CGNetwork类里面
    //======================================================
    class CGNetRecvBuffer
    {
    protected:
        char*		m_pBuffer;
		size_t		m_nBufSize;
        size_t		m_nDataBegin;
        size_t		m_nDataEnd;

    public:
        CGNetRecvBuffer(void);
        ~CGNetRecvBuffer(void);

        void        Extend( size_t nSize );
		size_t		GetMaxBufferSize();
        char*		GetBuffer();
        size_t		GetBufferSize();
        char*		GetLeftBuffer();
        size_t		GetLeftSize();
        void		Pop( size_t nSize );
        void		Push( size_t nSize );
    };
}

#include "CGNetBuffer.inl"
#endif
