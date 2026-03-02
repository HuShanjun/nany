#pragma once

namespace Gamma
{
	class IResultHolder
	{
	public:
		virtual void Write( const void* pData, size_t nSize ) = 0;
		virtual void Segment() = 0;
	};
}
