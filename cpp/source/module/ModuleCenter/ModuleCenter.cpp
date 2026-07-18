#include "ModuleCenter.h"
#include "Interface/IGameApp.h"
#include "GammaCommon/ILog.h"

void CModuleCenter::OnInit() {
    auto pApp = GetGameApp();
    Log::Info("ModuleCenter::OnInit moduleId={} serverName={} serverType={} serverId={}",
              GetModuleID(),
              pApp ? pApp->GetServerName() : std::string{},
              pApp ? pApp->GetServerType() : std::string{},
              pApp ? pApp->GetServerID() : 0u);
}

void CModuleCenter::UnInit() {
    Log::Info("ModuleCenter::UnInit");
}

int CModuleCenter::OnTimer(uint32 /*nInterval*/) {
    return 0;
}

void CModuleCenter::OnServerConnect(uint16 nServerID) {
    Log::Info("ModuleCenter::OnServerConnect serverId={}", nServerID);
}

void CModuleCenter::OnServerDisConnect(uint16 nServerID) {
    Log::Info("ModuleCenter::OnServerDisConnect serverId={}", nServerID);
}

bool CModuleCenter::OnServerStop() {
    Log::Info("ModuleCenter::OnServerStop");
    return true;
}
