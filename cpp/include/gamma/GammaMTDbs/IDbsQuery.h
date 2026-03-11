#pragma once
#include "GammaCommon/IGammaUnknow.h"
#include "GammaCommon/GammaHelp.h"

namespace Gamma
{
	class IDbsQueryHandler
	{
	public:
		virtual size_t OnQueryResult( const void* pResult, size_t nSize ) = 0;
	};

	class IDbsQuery : public IGammaUnknown
	{
	public:
		virtual void Query( uint32_t nChannel, const SSendBuf aryBuffer[], uint32_t nBufferCount ) = 0;
		virtual void Query(uint32_t nChannel, const void* pData1, size_t nSize1) = 0;
		virtual void Record(const void* pData, size_t nSize) = 0;
	};
}
