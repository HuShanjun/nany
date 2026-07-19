#include "CGameConnFromClient.h"
#include "CNetComp.h"
#include "GameApp/GameApp.h"
#include "GameApp/ModuleMgr.h"
#include "GammaApp/CBaseApp.h"

using namespace std;

DEFINE_DYNAMIC_CLASS(CGameConnFromClient, 2048, GET_CLASS_ID(CGameConnFromClient))

static CNetComp *GetNetComp() {
    CBaseApp *pApp = CBaseApp::Inst();
    if (!pApp) {
        return nullptr;
    }
    return static_cast<CNetComp *>(pApp->GetComp(CNetComp::GetID()));
}

//========================================================================
// CGameConnFromClient
//========================================================================
CGameConnFromClient::CGameConnFromClient(void)
    : CClientConnectNode(IDAllocator::Alloc()), m_nPlayerID(0) {
}

CGameConnFromClient::~CGameConnFromClient(void) {
    IDAllocator::Free(GetConnectID());
}

void CGameConnFromClient::OnConnected() {
    uint32 nID = GetConnectID();
    GammaLog << "ClientConn OnConnected( " << this << "," << nID << " ) : "
             << GetRemoteAddress().GetAddress() << ":" << GetRemoteAddress().GetPort();
    GammaLog << "<==>" << GetLocalAddress().GetAddress() << ":" << GetLocalAddress().GetPort()
             << endl;

    if (auto *pNet = GetNetComp()) {
        pNet->AddClientConnect(this);
    }
    CModuleMgr::Instance()->OnClientConnect(nID);
}

void CGameConnFromClient::OnDisConnect() {
    uint32 nID = GetConnectID();
    GammaLog << "ClientConn OnDisConnect( " << this << "," << nID << " ) : "
             << GetRemoteAddress().GetAddress() << ":" << GetRemoteAddress().GetPort();
    GammaLog << "<==>" << GetLocalAddress().GetAddress() << ":" << GetLocalAddress().GetPort()
             << endl;

    CModuleMgr::Instance()->OnClientDisConnect(nID);
    if (auto *pNet = GetNetComp()) {
        pNet->DelClientConnect(this);
    }
}

//====================================================================
// 消息发送
//====================================================================
void CGameConnFromClient::SendShellMsg(const SSendBuf aryBuffer[], uint32 nBufferCount,
                                       bool bUnreliable) {
    CBaseConn::SendShellMsg(aryBuffer, nBufferCount, true);
}

void CGameConnFromClient::SendShellMsg(const void *pData1, uint32 nSize1, bool bUnreliable) {
    SSendBuf SendBuf(pData1, nSize1);
    SendShellMsg(&SendBuf, 1);
}

//====================================================================
// 消息派发
//====================================================================
size_t CGameConnFromClient::OnShellMsg(const void *pCmd, size_t nSize, bool bUnreliable) {
    const CShellPrt *pMsg = (const CShellPrt *)pCmd;
    uint16 nMsgId = pMsg->GetId();
    Log::Info("CGameConnFromClient::OnShellMsg nMsgId = {}", nMsgId);
    return nSize;
}
