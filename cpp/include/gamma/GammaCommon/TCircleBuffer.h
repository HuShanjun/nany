#pragma once
#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/GammaMemory.h"
#include <string>

namespace Gamma
{
	template<uint32_t nBlockSize = 8192>
	class TCircleBufferBlockAlloc
	{
	public:
		enum { eBlockSize = nBlockSize };
		void* Alloc() { return TFixedPageAlloc<eBlockSize>::Alloc(); }
		void Free( void* pBlock ) { TFixedPageAlloc<eBlockSize>::Free( pBlock ); }
	};

	template< class BufferAlloc, bool bFreeEmptyBuffer = false >
	class TCircleBuffer 
	{
	protected:
		struct SQuerryBuffer;
		typedef SQuerryBuffer* SQuerryNodePtr;

		struct SQuerryBufferHead
		{
			volatile uint32_t			m_nReadPos;
			volatile uint32_t			m_nWritePos;
			volatile SQuerryNodePtr	m_pNextBuffer;
		};

		enum 
		{
			eBlockSize = BufferAlloc::eBlockSize,
			nNodeBufferSize = eBlockSize - sizeof(SQuerryBufferHead)
		};

		struct SQuerryBuffer : public SQuerryBufferHead
		{
			tbyte					m_aryBuffer[nNodeBufferSize];
		};

		BufferAlloc					m_Alloc;
		volatile SQuerryNodePtr		m_pReadBuffer;
		volatile SQuerryNodePtr		m_pWriteBuffer;
	public://todo:
		uint64_t						m_nPushCount;  
		uint64_t						m_nPopCount; 
		std::string					m_strBuffer;

		void						WriteBuffer( SQuerryNodePtr& pWriteBuffer, uint32_t& nWritePos,
										SQuerryNodePtr pReadBuffer, const void* pBuffer, size_t nSize );
		void						ReadBuffer( SQuerryNodePtr& pReadBuffer, uint32_t& nReadPos,
										void* pBuffer, size_t nSize );
	public:
		TCircleBuffer( BufferAlloc _Alloc = BufferAlloc() );
		~TCircleBuffer();

		template<class ContextType>
		void						PushBuffer( ContextType nContext, 
										const SSendBuf aryBuffer[], uint32_t nBufferCount, bool bMerge );
		template<class ContextType>
		bool						PopBuffer( ContextType& nContext,
										SSendBuf aryBuffer[], uint32_t& nBufferCount );

		void						PushRaw( const SSendBuf aryBuffer[], uint32_t nBufferCount );
		void						PushRaw( const void* pBuffer, uint32_t nBufferSize );
		uint32_t						PopRaw( void* pBuffer, uint32_t nBufferSize );

		bool						CanPop() const;
		uint32_t						GetWaitingBufferCount() const;
	};

	typedef TCircleBuffer< TCircleBufferBlockAlloc<8192> > CCircleBuffer;
}

#include "GammaCommon/TCircleBuffer.inl"