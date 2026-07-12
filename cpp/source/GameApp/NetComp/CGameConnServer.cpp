#include "CGameConnServer.h"
#include "GameApp/GameApp.h"

// #include "ModuleMgr.h"
// #include "ShellCommon/NetDefine.h"

using namespace std;
// extern CGameAppPtr g_pApp;

DEFINE_DYNAMIC_CLASS(CGameConnServer, 10, GET_CLASS_ID(CGameConnServer))

//========================================================================
// CGameConnToWorld
//========================================================================
CGameConnServer::CGameConnServer(void) 
	: m_nServerID(0)
	, m_nServerType(-1) {
}

CGameConnServer::~CGameConnServer(void) {
}

void CGameConnServer::OnConnected() {
    GammaLog << "[Net] OnConnected( " << this << " " << GetRemoteAddress().GetAddress() << ":"
             << GetRemoteAddress().GetPort();
    GammaLog << "<==>" << GetLocalAddress().GetAddress() << ":" << GetLocalAddress().GetPort()
             << endl;
    // CN2W_NotifyNetgateID NetMsg;
    // NetMsg.nNetgateID = CGameAppNetgate::Inst()->GetServerID();
    // SendShellMsg( &NetMsg, sizeof(NetMsg) );
    // CS2S_UpdateServerDes msg;
    // msg.nServerID = g_pApp->GetServerID();
    // msg.nServerType = g_pApp->GetServerType();
    // this->SendShellMsg(&msg, sizeof(msg));
}

void CGameConnServer::OnDisConnect() {
    // CGameAppNetgate::Inst()->LogClientDisConnect( this );
    GammaLog << "World OnDisConnect( " << this << GetRemoteAddress().GetAddress() << ":"
             << GetRemoteAddress().GetPort();
    GammaLog << "<==>" << GetLocalAddress().GetAddress() << ":" << GetLocalAddress().GetPort()
             << endl;
    // g_pApp->OnServerDisConnect(m_nServerID, this);
}

//====================================================================
// 消息发送
//====================================================================
void CGameConnServer::SendShellMsg(const SSendBuf aryBuffer[], uint32 nBufferCount) {
    CBaseConn::SendShellMsg(aryBuffer, nBufferCount, true);
}

void CGameConnServer::SendShellMsg(const void *pData1, uint32 nSize1) {
    CBaseConn::SendShellMsg(pData1, nSize1, true);
}

//====================================================================
// 消息派发
//====================================================================
size_t CGameConnServer::OnShellMsg(const void *pCmd, size_t nSize, bool bUnreliable) {
    const CShellPrt *pMsg = (const CShellPrt *)pCmd;
    uint16 nMsgId = pMsg->GetId();
    // if (IS_STRUCT_MSG(nMsgId))
    // {
    // 	if (nMsgId == eS2S_UpdateServerDes)
    // 	{
    // 		CS2S_UpdateServerDes* pmsg = (CS2S_UpdateServerDes * )pCmd;
    // 		m_nServerType = pmsg->nServerType;
    // 		this->SetServerID(pmsg->nServerID);
    // 	}
    // 	fpMsg* pFn = CGameApp::Inst()->GetMessageFun(nMsgId);
    // 	if (pFn != nullptr)
    // 	{
    // 		(*pFn)(pCmd, (uint32)nSize, this);
    // 	}
    // }
    // else
    // {
    // 	 CGameApp::Inst()->OnLuaMsg(nMsgId, pMsg + 1, (uint32)nSize - sizeof(CShellPrt), this);
    // }
    return nSize;
}

void CGameConnServer::SetServerID(uint32 nID) {
    m_nServerID = nID;
	Log::Info("CGameConnServer::SetServerID nID = {}", nID);
    // g_pApp->AddConnectServer(this);
    // if (m_nServerType > 0) {
    //     // CModuleMgr::Instance()->OnServerConnect(m_nServerID);
    //     // g_pApp->OnServerConnect(m_nServerID, this, (uint16)m_nServerType );
    // }
}