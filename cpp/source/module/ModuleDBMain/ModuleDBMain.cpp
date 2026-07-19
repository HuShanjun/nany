#include "ModuleDBMain.h"
#include "GammaCommon/ILog.h"
#include "Interface/IGameApp.h"

void CModuleDBMain::OnInit() {
    auto pApp = GetGameApp();
    Log::Info("ModuleDBMain::OnInit moduleId={} serverName={} serverType={} serverId={}",
              GetModuleID(), pApp ? pApp->GetServerName() : std::string{},
              pApp ? pApp->GetServerType() : std::string{}, pApp ? pApp->GetServerID() : 0u);
}

void CModuleDBMain::UnInit() { Log::Info("ModuleDBMain::UnInit"); }

int CModuleDBMain::OnTimer(uint32 /*nInterval*/) { return 0; }

void CModuleDBMain::OnServerConnect(uint16 nServerID) {
    m_jobs.AddPeer(nServerID);
    Log::Info("ModuleDBMain::OnServerConnect serverId={}", nServerID);
}

void CModuleDBMain::OnServerDisConnect(uint16 nServerID) {
    m_jobs.RemovePeer(nServerID);
    Log::Info("ModuleDBMain::OnServerDisConnect serverId={}", nServerID);
}

bool CModuleDBMain::OnServerStop() {
    Log::Info("ModuleDBMain::OnServerStop");
    return true;
}

uint32_t CModuleDBMain::EnqueueQuery(uint32 nFromServerID) {
    uint32_t id = m_jobs.Enqueue(nFromServerID);
    if (id == 0) {
        Log::Error("ModuleDBMain::EnqueueQuery failed fromServerId={}", nFromServerID);
        return 0;
    }
    Log::Info("ModuleDBMain::EnqueueQuery jobId={} fromServerId={}", id, nFromServerID);
    return id;
}

bool CModuleDBMain::CompleteQuery(uint32 nJobID) {
    bool ok = m_jobs.Complete(nJobID);
    if (!ok) {
        Log::Error("ModuleDBMain::CompleteQuery missing jobId={}", nJobID);
        return false;
    }
    Log::Info("ModuleDBMain::CompleteQuery jobId={}", nJobID);
    return true;
}
