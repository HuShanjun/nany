#include "ModuleCenter.h"
#include "GammaCommon/ILog.h"
#include "Interface/IGameApp.h"

void CModuleCenter::OnInit() {
    auto pApp = GetGameApp();
    Log::Info("ModuleCenter::OnInit moduleId={} serverName={} serverType={} serverId={}",
              GetModuleID(), pApp ? pApp->GetServerName() : std::string{},
              pApp ? pApp->GetServerType() : std::string{}, pApp ? pApp->GetServerID() : 0u);
}

void CModuleCenter::UnInit() { Log::Info("ModuleCenter::UnInit"); }

int CModuleCenter::OnTimer(uint32 /*nInterval*/) { return 0; }

void CModuleCenter::OnServerConnect(uint16 nServerID) {
    m_registry.AddPeer(nServerID);
    uint32 nTypeID = static_cast<uint32>(nServerID) / 100;
    bool ok = m_registry.RegisterServer(nServerID, nTypeID);
    Log::Info("ModuleCenter::OnServerConnect serverId={} typeId={} registered={}", nServerID,
              nTypeID, ok);
}

void CModuleCenter::OnServerDisConnect(uint16 nServerID) {
    m_registry.RemovePeer(nServerID);
    Log::Info("ModuleCenter::OnServerDisConnect serverId={}", nServerID);
}

bool CModuleCenter::OnServerStop() {
    Log::Info("ModuleCenter::OnServerStop");
    return true;
}

bool CModuleCenter::RegisterServer(uint32 nServerID, uint32 nTypeID) {
    bool ok = m_registry.RegisterServer(nServerID, nTypeID);
    if (!ok) {
        Log::Error("ModuleCenter::RegisterServer failed serverId={} typeId={}", nServerID, nTypeID);
        return false;
    }
    Log::Info("ModuleCenter::RegisterServer serverId={} typeId={}", nServerID, nTypeID);
    return true;
}
