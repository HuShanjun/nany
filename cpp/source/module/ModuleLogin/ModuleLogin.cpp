#include "ModuleLogin.h"
#include "GammaCommon/ILog.h"
#include "Interface/IGameApp.h"

void CModuleLogin::OnInit() {
    auto pApp = GetGameApp();
    Log::Info("ModuleLogin::OnInit moduleId={} serverName={} serverType={} serverId={}",
              GetModuleID(), pApp ? pApp->GetServerName() : std::string{},
              pApp ? pApp->GetServerType() : std::string{}, pApp ? pApp->GetServerID() : 0u);
}

void CModuleLogin::UnInit() { Log::Info("ModuleLogin::UnInit"); }

int CModuleLogin::OnTimer(uint32 /*nInterval*/) { return 0; }

void CModuleLogin::OnServerConnect(uint16 nServerID) {
    m_peers.Add(nServerID);
    Log::Info("ModuleLogin::OnServerConnect serverId={}", nServerID);
}

void CModuleLogin::OnServerDisConnect(uint16 nServerID) {
    m_peers.Remove(nServerID);
    Log::Info("ModuleLogin::OnServerDisConnect serverId={}", nServerID);
}

bool CModuleLogin::OnServerStop() {
    Log::Info("ModuleLogin::OnServerStop");
    return true;
}

uint32_t CModuleLogin::SubmitAuth(const std::string &account) {
    uint32_t id = m_auth.SubmitAuth(account);
    if (id == 0) {
        Log::Error("ModuleLogin::SubmitAuth failed empty account");
        return 0;
    }
    Log::Info("ModuleLogin::SubmitAuth ticketId={} account={}", id, account);
    return id;
}

bool CModuleLogin::CompleteAuth(uint32_t ticket, bool ok) {
    bool done = m_auth.CompleteAuth(ticket, ok);
    if (!done) {
        Log::Error("ModuleLogin::CompleteAuth missing ticketId={}", ticket);
        return false;
    }
    Log::Info("ModuleLogin::CompleteAuth ticketId={} ok={}", ticket, ok);
    return true;
}
