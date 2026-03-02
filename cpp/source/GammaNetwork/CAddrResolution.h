#pragma once
#include "GammaCommon/TTinyList.h"
#include "GammaCommon/TGammaList.h"
#include "GammaCommon/TGammaRBTree.h"
#include "GammaCommon/IGammaFileMgr.h"
#include "GammaCommon/TConstString.h"
#include "GammaCommon/CThread.h"

namespace Gamma
{
	struct SAddressInfoArray;
	class CConnectingNode;
	class CAddrResolution;
	class CAddrResolutionDelegate;
	typedef TGammaList<CAddrResolutionDelegate> CResolutionDelegateList;
	typedef TGammaRBTree<CAddrResolutionDelegate> CResolutionDelegateTree;
	union UAddressInfo { sockaddr_in IPv4; sockaddr_in6 IPv6; };
	typedef std::vector<UAddressInfo> CAddressInfoList;

	//===================================================================
	// CAddrResolutionDelegate
	//===================================================================
	class CAddrResolutionDelegate
		: public gammacstring
		, public TTinyList<CAddrResolutionDelegate>::CTinyListNode
		, public TGammaRBTree<CAddrResolutionDelegate>::CGammaRBTreeNode
		, public TGammaList<CAddrResolutionDelegate>::CGammaListNode
		, public TGammaList<CConnectingNode>
	{
		CAddrResolution*			m_pResolution;
		HLOCK						m_hNetworkFinishLock;
		CResolutionDelegateList		m_listProcessing;
		CResolutionDelegateList& 	m_listFinished;
		CAddressInfoList			m_listAddressBuff[2];
	public:
		CAddrResolutionDelegate( const char* szAddress, 
			HLOCK hFinishLock, CResolutionDelegateList& listFinished );
		~CAddrResolutionDelegate(void);

		bool						IsResolveSuceeded() const;
		const UAddressInfo*			GetAddressBuff( bool bUdp ) const;
		void						Resolve();
		void						SetIPValidTime( uint32 nValidSeconds );
		void						FinishResolution( CAddressInfoList aryResult[2] );
	};
}


