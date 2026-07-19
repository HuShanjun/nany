#include "ModuleGate.h"
#include "Interface/IGameApp.h"
#include "GammaCommon/ILog.h"

void CModuleGate::OnInit() {
    auto pApp = GetGameApp();
    Log::Info("ModuleGate::OnInit moduleId={} serverName={} serverType={} serverId={}",
              GetModuleID(),
              pApp ? pApp->GetServerName() : std::string{},
              pApp ? pApp->GetServerType() : std::string{},
              pApp ? pApp->GetServerID() : 0u);
}

void CModuleGate::UnInit() {
    Log::Info("ModuleGate::UnInit");
}

int CModuleGate::OnTimer(uint32 /*nInterval*/) {
    return 0;
}

void CModuleGate::OnServerConnect(uint16 nServerID) {
    m_store.AddServer(nServerID);
    Log::Info("ModuleGate::OnServerConnect serverId={}", nServerID);
}

void CModuleGate::OnServerDisConnect(uint16 nServerID) {
    m_store.RemoveServer(nServerID);
    Log::Info("ModuleGate::OnServerDisConnect serverId={}", nServerID);
}

void CModuleGate::OnClientConnect(uint32 nClientID) {
    int64_t t = 0;
    if (auto pApp = GetGameApp()) {
        t = pApp->GetCurTime();
    }
    m_store.AddClient(nClientID, t);
    Log::Info("ModuleGate::OnClientConnect clientId={}", nClientID);
}

void CModuleGate::OnClientDisConnect(uint32 nClientID) {
    m_store.RemoveClient(nClientID);
    Log::Info("ModuleGate::OnClientDisConnect clientId={}", nClientID);
}

bool CModuleGate::OnServerStop() {
    Log::Info("ModuleGate::OnServerStop");
    return true;
}

bool CModuleGate::BindClientToServer(uint32 nClientID, uint32 nServerID) {
    bool ok = m_store.BindClientToServer(nClientID, nServerID);
    if (!ok) {
        Log::Error("ModuleGate::BindClientToServer failed clientId={} serverId={}", nClientID,
                   nServerID);
        return false;
    }
    Log::Info("ModuleGate::BindClientToServer clientId={} serverId={}", nClientID, nServerID);
    return true;
}

bool CModuleGate::ForwardToServer(uint32 nClientID, uint32 nServerID, const void * /*pData*/,
                                  size_t nSize) {
    if (!m_store.CanForwardToServer(nClientID, nServerID)) {
        Log::Error("ModuleGate::ForwardToServer missing client/server clientId={} serverId={}",
                   nClientID, nServerID);
        return false;
    }
    Log::Info("ModuleGate::ForwardToServer stub clientId={} serverId={} size={}", nClientID,
              nServerID, nSize);
    return true;
}

bool CModuleGate::ForwardToClient(uint32 nClientID, const void * /*pData*/, size_t nSize) {
    if (!m_store.CanForwardToClient(nClientID)) {
        Log::Error("ModuleGate::ForwardToClient missing clientId={}", nClientID);
        return false;
    }
    Log::Info("ModuleGate::ForwardToClient stub clientId={} size={}", nClientID, nSize);
    return true;
}
