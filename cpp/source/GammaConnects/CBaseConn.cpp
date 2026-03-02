
#include "GammaConnects/CBaseConn.h"
#include "CConnection.h"
#include "CConnectionMgr.h"

namespace Gamma
{
	static const CAddress s_Empty;

	CBaseConn::CBaseConn( void ) 
		: m_pConnection(NULL)
	{
	}

	CBaseConn::~CBaseConn( void )
	{
	}

	void CBaseConn::BindConnection( CConnection* pConnection )
	{
		if( !IsValid() )
			return;
		if( m_pConnection )
			m_pConnection->m_pConnHandler = NULL;

		if( pConnection )
		{
			if( pConnection->m_pConnHandler )
				pConnection->m_pConnHandler->m_pConnection = NULL;
			pConnection->m_pConnHandler = this;
		}

		m_pConnection = pConnection;
	}

	void CBaseConn::ForceClose( const char* szLogContext )
	{
		if( !IsValid() || !m_pConnection )
			return;
		m_pConnection->ShutDown( false, szLogContext );
	}

	void CBaseConn::ShellCmdClose( const char* szLogContext )
	{
		if( !IsValid() || !m_pConnection )
			return;
		m_pConnection->ShutDown( true, szLogContext );
	}

	const CAddress& CBaseConn::GetRemoteAddress() const
	{
		if( !IsValid() || !m_pConnection )
			return s_Empty;
		return m_pConnection->GetRemoteAddress();
	}

	const CAddress& CBaseConn::GetLocalAddress() const
	{
		if( !IsValid() || !m_pConnection )
			return s_Empty;
		return m_pConnection->GetLocalAddress();
	}

	bool CBaseConn::IsConnected() const
	{
		if( !IsValid() || !m_pConnection )
			return false;
		return m_pConnection->IsConnected();
	}

	uint32 CBaseConn::GetPingDelay() const
	{
		if( !IsValid() || !m_pConnection )
			return INVALID_32BITID;
		return m_pConnection->GetPingDelay();
	}

	void CBaseConn::EnableSendShellMsg( bool bValue )
	{
		if( !IsValid() || !m_pConnection )
			return;
		m_pConnection->EnableSendShellMsg( bValue );
	}

	void CBaseConn::SetHeartBeatInterval( uint32 nSeconds )
	{
		if( !IsValid() || !m_pConnection )
			return;
		m_pConnection->SetHeartBeatInterval( nSeconds );
	}

	void CBaseConn::SetNetDelay( uint32 nMinDelay, uint32 nMaxDelay )
	{
		if( !IsValid() || !m_pConnection )
			return;
		m_pConnection->SetNetDelay( nMinDelay, nMaxDelay );
	}

	bool CBaseConn::IsMsgDispatchEnable() const
	{
		if( !IsValid() || !m_pConnection )
			return false;
		return m_pConnection->IsMsgDispatchEnable();
	}

	void CBaseConn::EnableMsgDispatch( bool bEnable ) 
	{
		if( !IsValid() || !m_pConnection )
			return;
		m_pConnection->EnableMsgDispatch( bEnable );
	}

	void CBaseConn::EnableProfile( uint8 nSendIDBits, uint8 nRecvIDBits )
	{
		if( !IsValid() || !m_pConnection )
			return;
		m_pConnection->EnableProfile( nSendIDBits, nRecvIDBits );
	}

	void CBaseConn::PrintMsgSize()
	{
		if (!m_pConnection)
			return;
		m_pConnection->PrintMsgSize();
	}

	size_t CBaseConn::GetSendBufferSize( uint16 nShellID )
	{
		if( !IsValid() || !m_pConnection )
			return 0;
		return m_pConnection->GetSendBufferSize( nShellID );
	}

	size_t CBaseConn::GetRecvBufferSize( uint16 nShellID )
	{
		if( !IsValid() || !m_pConnection )
			return 0;
		return m_pConnection->GetRecvBufferSize( nShellID );
	}

	size_t CBaseConn::GetTotalSendSize()
	{
		if( !IsValid() || !m_pConnection )
			return 0;
		return m_pConnection->GetTotalSendSize();
	}

	size_t CBaseConn::GetTotalRecvSize()
	{
		if( !IsValid() || !m_pConnection )
			return 0;
		return m_pConnection->GetTotalRecvSize();
	}

	bool CBaseConn::IsGraceClose() const
	{
		if( !IsValid() || !m_pConnection )
			return false;
		return m_pConnection->IsGraceClose();
	}

	bool CBaseConn::IsForceClose() const
	{
		if( !IsValid() || !m_pConnection )
			return false;
		return m_pConnection->IsForceClose();
	}

	const char* CBaseConn::GetCloseLog() const
	{
		if( !IsValid() || !m_pConnection )
			return "";
		return m_pConnection->GetCloseLog();
	}

	Gamma::ECloseType CBaseConn::GetCloseType() const
	{
		if( !IsValid() || !m_pConnection || !m_pConnection->GetConn() )
			return eCE_Unknown;
		return m_pConnection->GetConn()->GetCloseType();
	}

	void CBaseConn::SendShellMsg( const SSendBuf aryBuffer[], uint32 nBufferCount, bool bUnreliable )
	{
		if( !IsValid() || !m_pConnection )
			return;
		m_pConnection->SendShellMsg( !bUnreliable, aryBuffer, nBufferCount );
	}

	void CBaseConn::SendShellMsg( const void* pData1, uint32 nSize1, bool bUnreliable )
	{
		if( !IsValid() || !m_pConnection )
			return;
		SSendBuf SendBuf = SSendBuf( pData1, nSize1 );
		m_pConnection->SendShellMsg( !bUnreliable, &SendBuf, 1 );
	}
}
