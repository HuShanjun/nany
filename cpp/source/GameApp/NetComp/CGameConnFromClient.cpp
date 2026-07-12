#include "CGameConnFromClient.h"
#include "GameApp/GameApp.h"
#include "CNetComp.h"

using namespace std;

DEFINE_DYNAMIC_CLASS( CGameConnFromClient, 2048, GET_CLASS_ID( CGameConnFromClient ) )

//========================================================================
// CGameConnFromClient
//========================================================================
CGameConnFromClient::CGameConnFromClient(void) 
	: CClientConnectNode( IDAllocator::Alloc() )
	, m_nPlayerID(0)
{
}

CGameConnFromClient::~CGameConnFromClient(void)
{
	IDAllocator::Free(GetConnectID());
}


void CGameConnFromClient::OnConnected()
{
	uint32 nID = GetConnectID();
	GammaLog << "ClientConn OnConnected( " << this << "," << nID << " ) : "
		<< GetRemoteAddress().GetAddress() << ":" << GetRemoteAddress().GetPort();
	GammaLog << "<==>" << GetLocalAddress().GetAddress() << ":" << GetLocalAddress().GetPort() << endl;
	// CGameApp::Inst()->AddClientConnect(this);
	// CGameApp::Inst()->OnClientConnect(this);
}

void CGameConnFromClient::OnDisConnect()
{
	//CGameAppNetgate::Inst()->LogClientDisConnect( this );
	GammaLog << "ClientConn OnDisConnect( " << this << "," << CClientConnectNode::m_Data << " ) : " 
		<< GetRemoteAddress().GetAddress() << ":" << GetRemoteAddress().GetPort();
	GammaLog << "<==>" << GetLocalAddress().GetAddress() << ":" << GetLocalAddress().GetPort() << endl;
	// CGameApp::Inst()->OnClientDisConnect(this);
	// CGameApp::Inst()->DelClientConnect(this);
}


//====================================================================
// 消息发送
//====================================================================
void CGameConnFromClient::SendShellMsg( const SSendBuf aryBuffer[], uint32 nBufferCount, bool bUnreliable)
{
	CBaseConn::SendShellMsg(aryBuffer, nBufferCount, true );
}

void CGameConnFromClient::SendShellMsg( const void* pData1, uint32 nSize1, bool bUnreliable)
{
	SSendBuf SendBuf( pData1, nSize1 );
	SendShellMsg( &SendBuf, 1 );
}

//====================================================================
// 消息派发
//====================================================================
size_t CGameConnFromClient::OnShellMsg( const void* pCmd, size_t nSize, bool bUnreliable )
{
	const CShellPrt* pMsg = (const CShellPrt*)pCmd;
	uint16 nMsgId = pMsg->GetId();
	Log::Info("CGameConnFromClient::OnShellMsg nMsgId = {}", nMsgId);
	// if (IS_STRUCT_MSG(nMsgId))
	// {
	// 	fpMsg* pFn = CGameApp::Inst()->GetMessageFunClient(nMsgId);
	// 	if (pFn != nullptr)
	// 	{
	// 		(*pFn)(pCmd, (uint32)nSize, this);
	// 	}
	// }
	// else
	// {
	// 	CGameApp::Inst()->OnLuaMsgClient(nMsgId, pMsg + 1, (uint32)nSize - sizeof(CShellPrt), this);
	// }
	return nSize;
}

