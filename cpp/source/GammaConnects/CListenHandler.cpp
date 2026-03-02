
#include "CListenHandler.h"
#include "CConnectionMgr.h"

namespace Gamma
{
	CListenHandler::CListenHandler( CConnectionMgr* pMgr, 
		IListener* pListener, uint32 nClassID, EConnType eType )
		: m_pMgr( pMgr )
		, m_pListener( pListener )
		, m_nConnClassID( nClassID )
		, m_eType( eType )
	{
		m_pListener->SetHandler( this );
	}

	CListenHandler::~CListenHandler()
	{
		m_pListener->SetHandler( NULL );
		SAFE_RELEASE( m_pListener );
	}

	void CListenHandler::OnAccept( Gamma::IConnecter& Connect )
	{
		m_pMgr->OnAccept( Connect, m_nConnClassID, m_eType );
	}

	uint32 CListenHandler::GetConnClassID() const
	{
		return m_nConnClassID;
	}

	const CAddress& CListenHandler::GetAddress() const
	{
		static CAddress s_Empty;
		if( !m_pListener )
			return s_Empty;
		return m_pListener->GetLocalAddress();
	}

	bool CListenHandler::Match( uint32 nClassID, const CAddress& Address )
	{
		if( nClassID != m_nConnClassID )
			return false;
		const CAddress& addrListen = GetAddress();
		if( addrListen.GetIP() != Address.GetIP() )
			return false;
		if( addrListen.GetPort() != Address.GetPort() )
			return false;
		return true;
	}

}
