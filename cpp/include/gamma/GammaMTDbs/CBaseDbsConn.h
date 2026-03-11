#pragma once
#include "GammaCommon/CDynamicObject.h"
#include "GammaDatabase/IDatabase.h"

namespace Gamma
{
	class IResultHolder;

	class CBaseDbsConn : public CDynamicObject
	{
	public:
		virtual void	OnConnected(uint32_t nThreadIndex, IDbConnection** aryDbsConn, uint8_t nConnCount, bool bIsRecord) = 0;		/// 连接成功
		virtual void	OnDisConnect( IDbConnection** aryDbsConn, uint8_t nConnCount ) = 0;
		virtual void	OnShellMsg( IDbConnection** aryDbsConn, uint8_t nConnCount, IResultHolder* pRH, const void* pCmd, size_t nSize ) = 0;	/// shell层消息
	};
}
