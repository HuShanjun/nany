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
    Log::Info("ModuleGate::OnServerConnect serverId={}", nServerID);
}

void CModuleGate::OnServerDisConnect(uint16 nServerID) {
    Log::Info("ModuleGate::OnServerDisConnect serverId={}", nServerID);
}

bool CModuleGate::OnServerStop() {
    Log::Info("ModuleGate::OnServerStop");
    return true;
}
