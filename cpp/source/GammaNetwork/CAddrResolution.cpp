
#include "CGNetwork.h"
#include "GammaNetworkHelp.h"
#include "CAddrResolution.h"
#include "GammaCommon/GammaTime.h"
#include "GammaCommon/CGammaRand.h"

namespace Gamma
{
	typedef TGammaRBTree<CAddrResolution> CResolutionMap;
	typedef TTinyList<CAddrResolutionDelegate> CAddrResolutionList;
	struct SAddressInfoArray { UAddressInfo aryInfo[1024]; uint32_t nCount; };

	//===================================================================
	// CAddrResolution
	//===================================================================
	class CAddrResolution
		: public gammacstring
		, public CResolutionMap::CGammaRBTreeNode
		, public CAddrResolutionList
	{
		HTHREAD					m_hThread;
		HLOCK					m_hAddrResolutionLock;
		uint32_t					m_nValidSeconds;
		int64_t					m_nPreResolutionTime;
		CAddressInfoList		m_listAddressBuff[2];
		ELoadState				m_eResolutionState;

		void					Process();
	public:
		CAddrResolution( const char* szAddress );
		~CAddrResolution(void);

		Gamma::ELoadState		GetResolutionState() const { return m_eResolutionState; }
		void					AddDelegate( CAddrResolutionDelegate* pDelegate );
		void					RemoveDelegate( CAddrResolutionDelegate* pDelegate );
		void					SetIPValidTime( uint32_t nValidSeconds );
		bool					IsTimeOut() const;
		void					Resolve();
	};

	CAddrResolution::CAddrResolution( const char* szAddress )
		: m_eResolutionState( eLoadState_NotLoad )
		, m_hAddrResolutionLock( GammaCreateLock() )
		, m_nValidSeconds( INVALID_32BITID )
		, m_nPreResolutionTime( 0 )
		, m_hThread( NULL )
	{
		uint32_t nLen = (uint32_t)strlen(szAddress);
		char* szStrBuffer = new char[nLen + 1];
		memcpy( szStrBuffer, szAddress, nLen + 1 );
		gammacstring::assign( szStrBuffer, true );
	}

	CAddrResolution::~CAddrResolution(void)
	{
		Remove();

		const char* szStrBuffer = gammacstring::c_str();
		gammacstring::clear();
		SAFE_DEL_GROUP( szStrBuffer );

		if( m_hThread )
			GammaJoinThread( m_hThread );
		GammaDestroyLock( m_hAddrResolutionLock );
	}

	void CAddrResolution::AddDelegate( CAddrResolutionDelegate* pDelegate )
	{
		GammaLock( m_hAddrResolutionLock );
		PushFront( *pDelegate );
		if( m_eResolutionState > eLoadState_Loading )
			pDelegate->FinishResolution( m_listAddressBuff );
		GammaUnlock( m_hAddrResolutionLock );
	}

	void CAddrResolution::RemoveDelegate( CAddrResolutionDelegate* pDelegate )
	{
		GammaLock( m_hAddrResolutionLock );
		pDelegate->CAddrResolutionList::CTinyListNode::Remove();
		GammaUnlock( m_hAddrResolutionLock );
	}

	void CAddrResolution::Resolve()
	{
		GammaLock( m_hAddrResolutionLock );
		if( m_hThread )
			return GammaUnlock( m_hAddrResolutionLock );
			
		typedef CAddrResolution CMyType;
		struct _{ static void t( CMyType* p ) { p->Process(); } };
		GammaCreateThread( &m_hThread, 0, (THREADPROC)&_::t, this );
		GammaUnlock( m_hAddrResolutionLock );
	}

	void CAddrResolution::Process()
	{	
		GammaSetThreadName( "CAddrResolution" );
		if( m_eResolutionState == eLoadState_NotLoad ||
			m_eResolutionState == eLoadState_Failed )
			m_eResolutionState = eLoadState_Loading;

		SAddressInfoArray aryResult[2];
		aryResult[0].nCount = aryResult[1].nCount = 0;
		int32_t arySocketType[2] = { SOCK_STREAM, SOCK_DGRAM };
		for( uint32_t i = 0; i < ELEM_COUNT(arySocketType); i++ )
		{
			addrinfo Info;
			memset( &Info, 0, sizeof(Info) );
			Info.ai_family = AF_UNSPEC;
			Info.ai_socktype = arySocketType[i];

			addrinfo* pResult = NULL;
			const char* szAddress = gammacstring::c_str();
			int ret = getaddrinfo( szAddress, NULL, &Info, &pResult );
			if( ret == 0 )
			{
				while( pResult )
				{
					if( pResult->ai_family == AF_INET || 
						pResult->ai_family == AF_INET6 )
					{
						uint32_t nIndex = aryResult[i].nCount++;
						UAddressInfo& AddrInfo = aryResult[i].aryInfo[nIndex];
						memset( &AddrInfo, 0, sizeof(UAddressInfo) );
						memcpy( &AddrInfo, pResult->ai_addr, pResult->ai_addrlen );
					}
					pResult = pResult->ai_next;
				}
				continue;
			}

			struct hostent* hp = gethostbyname( szAddress );
			if( hp && hp->h_addr_list[0] && hp->h_addrtype == AF_INET )
			{
				for( int32_t j = 0; hp->h_addr_list[j]; j++ )
				{
					uint32_t nIndex = aryResult[i].nCount++;
					UAddressInfo& AddrInfo = aryResult[i].aryInfo[nIndex];
					memset( &AddrInfo, 0, sizeof(UAddressInfo) );
					AddrInfo.IPv4.sin_addr = *(in_addr*)hp->h_addr_list[j];
					AddrInfo.IPv4.sin_family = hp->h_addrtype;
				}
				continue;
			}
		}

		if( !aryResult[0].nCount && !aryResult[1].nCount )
			m_eResolutionState = eLoadState_Failed;
		else 
			m_eResolutionState = eLoadState_Succeeded;

		for( uint32_t i = 0; i < ELEM_COUNT(aryResult); i++ )
		{
			UAddressInfo* pFirst = aryResult[i].aryInfo;
			UAddressInfo* pEnd = pFirst + aryResult[i].nCount;
			m_listAddressBuff[i].assign( pFirst, pEnd );
		}

		GammaLock( m_hAddrResolutionLock );
		CAddrResolutionDelegate* pFirst = GetFirst();
		while( pFirst )
		{
			pFirst->FinishResolution( m_listAddressBuff );
			pFirst = pFirst->CAddrResolutionList::CTinyListNode::GetNext();
		}

		m_hThread = NULL;
		m_nPreResolutionTime = GetNatureTime()/1000;
		GammaUnlock( m_hAddrResolutionLock );
	}

	void CAddrResolution::SetIPValidTime( uint32_t nValidSeconds )
	{
		m_nValidSeconds = nValidSeconds;
	}

	bool CAddrResolution::IsTimeOut() const
	{
		return GetNatureTime()/1000 > m_nPreResolutionTime + m_nValidSeconds;
	}

	//===================================================================
	// CAddresolutionDelegate
	//===================================================================
	CAddrResolutionDelegate::CAddrResolutionDelegate( const char* szAddr, 
		HLOCK hFinishLock, CResolutionDelegateList& listFinished )
		: m_pResolution( NULL )
		, m_hNetworkFinishLock( hFinishLock )
		, m_listFinished( listFinished )
	{
		struct SResolutionMap : public CResolutionMap
		{ ~SResolutionMap(void){ while( GetFirst() ) delete GetFirst(); }; }; 

		static CLock s_ResolutionMapLock;
		static SResolutionMap s_mapResolution;

		gammacstring strKey( szAddr, true );
		s_ResolutionMapLock.Lock();
		m_pResolution = s_mapResolution.Find( strKey );
		if( m_pResolution == NULL )
			s_mapResolution.Insert( *( m_pResolution = new CAddrResolution( szAddr ) ) );
		s_ResolutionMapLock.Unlock();

		m_pResolution->AddDelegate( this );
		*static_cast<gammacstring*>( this ) = *m_pResolution;
	}

	CAddrResolutionDelegate::~CAddrResolutionDelegate( void )
	{
		m_pResolution->RemoveDelegate( this );
	}

	bool CAddrResolutionDelegate::IsResolveSuceeded() const
	{
		return m_listAddressBuff[0].size() || m_listAddressBuff[1].size();
	}

	const UAddressInfo* CAddrResolutionDelegate::GetAddressBuff( bool bUdp ) const
	{
		GammaLock( m_hNetworkFinishLock );
		if( m_listAddressBuff[bUdp].empty() )
		{
			GammaUnlock( m_hNetworkFinishLock );
			return NULL;
		}
		uint32_t nIndex = CGammaRand::Rand<uint32_t>( 0, (uint32_t)m_listAddressBuff[bUdp].size() );
		const UAddressInfo* pInfo = &m_listAddressBuff[bUdp][nIndex];
		GammaUnlock( m_hNetworkFinishLock );
		return pInfo;
	}

	void CAddrResolutionDelegate::SetIPValidTime( uint32_t nValidSeconds )
	{
		return m_pResolution->SetIPValidTime( nValidSeconds );
	}

	void CAddrResolutionDelegate::Resolve()
	{
		GammaLock( m_hNetworkFinishLock );
		ELoadState eResolutionState = m_pResolution->GetResolutionState();
		if( !CResolutionDelegateList::CGammaListNode::IsInList() )
		{
			// 如果解析器代理不在列表中需要添加到队列，等待回调
			// 如果之前有解析成功过可以直接加入成功队列
			// 否则只能加入处理队列
			if( IsResolveSuceeded() )
				m_listFinished.PushBack( *this );
			else
				m_listProcessing.PushBack( *this );
		}
		GammaUnlock( m_hNetworkFinishLock );

		// 如果解析器之前的状态已经在加载中，那么不需要启动解析器
		// 即便有当前状态已经变成了加载成功或者加载失败也没关系，因为
		// 如果之前IsResolveSuceeded()为true，必定会被处理，
		// 如果之前IsResolveSuceeded()为false，那么this加入m_listProcessing
		// 队列时FinishResolution函数肯定还没将this加入m_listFinished
		if( eResolutionState == eLoadState_Loading )
			return;

		// 如果解析器之前的状态成功并且域名解析尚未超时，那是必定要返回的
		if( eResolutionState == eLoadState_Succeeded &&
			m_pResolution->IsTimeOut() == false )
			return;

		// 重新进行域名解析
		m_pResolution->Resolve();
	}

	void CAddrResolutionDelegate::FinishResolution( CAddressInfoList aryResult[2] )
	{
		GammaLock( m_hNetworkFinishLock );

		uint32_t nPreSize = (uint32_t)(m_listAddressBuff[0].size() + m_listAddressBuff[1].size());
		for( uint32_t i = 0; i < ELEM_COUNT(m_listAddressBuff); i++ )
		{
			if( !aryResult[i].size() )
				continue;
			m_listAddressBuff[i] = aryResult[i];
		}

		if( !nPreSize )
		{
			CResolutionDelegateList::CGammaListNode::Remove();
			m_listFinished.PushBack( *this );
		}

		GammaUnlock( m_hNetworkFinishLock );
	}
}