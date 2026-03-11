#pragma once
#include "GammaCommon/TTinyList.h"
#include "GammaNetwork/INetHandler.h"
#include "GammaNetwork/INetworkInterface.h"
#include "GammaConnects/IConnectionMgr.h"

namespace Gamma
{
	class CConnectionMgr;

	/// 监听服务回调
	class CListenHandler 
		: public IListenHandler
		, public TTinyList<CListenHandler>::CTinyListNode
	{
		CConnectionMgr*		m_pMgr;
		IListener*			m_pListener;
		uint32_t				m_nConnClassID;
		EConnType			m_eType;
		void				OnAccept( Gamma::IConnecter& Connect );
	public:
		CListenHandler( CConnectionMgr* pMgr, 
			IListener* pListener, uint32_t nClassID, EConnType eType );
		~CListenHandler();
		uint32_t				GetConnClassID() const;
		const CAddress&		GetAddress() const;
		bool				Match( uint32_t nClassID, const CAddress& Address );
	};
}
