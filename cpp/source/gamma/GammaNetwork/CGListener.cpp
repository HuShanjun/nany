
#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/TConstString.h"
#include "GammaCommon/GammaCodeCvs.h"
#include "GammaNetworkHelp.h"
#include "CGNetwork.h"
#include "CGListener.h"
#include "CGConnecter.h"
#include <sstream>
#include <errno.h>

using namespace std;

namespace Gamma
{
	//========================================================
	// TCP SOCKET监听类构造函数，建立监听
	//========================================================
	CGListener::CGListener( CGNetwork* pNetwork, CGSocket* pSocket )
		: m_pNetwork( pNetwork )
		, m_pSocket( pSocket )
		, m_pHandler(nullptr)
	{
		GammaAst( m_pNetwork && m_pSocket );
		m_pSocket->Bind( this );
	}

	CGListener::~CGListener()
	{
		GammaAst( m_pNetwork && m_pSocket );
		SAFE_RELEASE( m_pSocket );
		m_pNetwork = nullptr;
	}

	bool CGListener::IsValid()
	{
		return m_pNetwork != nullptr;
	}

	void CGListener::Release()
	{
		m_pNetwork->ReleaseListener( this );
	}

	const CAddress& CGListener::GetLocalAddress() const
	{
		return m_LocalAddress;
	}

	EConnecterType CGListener::GetConnectType() const
	{
		return m_pSocket->GetConnectType();
	}

	void CGListener::Start( const char* szAddres, uint16_t nPort )
	{
		m_pSocket->StartListener( szAddres, nPort );
		m_LocalAddress.SetAddress( szAddres );
		m_LocalAddress.SetPort( nPort );
	}
}
