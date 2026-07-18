#include "ModuleDBMain.h"
#include "Interface/IGameApp.h"
#include "GammaCommon/ILog.h"

void CModuleDBMain::OnInit() {
    auto pApp = GetGameApp();
    Log::Info("ModuleDBMain::OnInit moduleId={} serverName={} serverType={} serverId={}",
              GetModuleID(),
              pApp ? pApp->GetServerName() : std::string{},
              pApp ? pApp->GetServerType() : std::string{},
              pApp ? pApp->GetServerID() : 0u);
}

void CModuleDBMain::UnInit() {
    Log::Info("ModuleDBMain::UnInit");
}

int CModuleDBMain::OnTimer(uint32 /*nInterval*/) {
    return 0;
}

void CModuleDBMain::OnServerConnect(uint16 nServerID) {
    Log::Info("ModuleDBMain::OnServerConnect serverId={}", nServerID);
}

void CModuleDBMain::OnServerDisConnect(uint16 nServerID) {
    Log::Info("ModuleDBMain::OnServerDisConnect serverId={}", nServerID);
}

bool CModuleDBMain::OnServerStop() {
    Log::Info("ModuleDBMain::OnServerStop");
    return true;
}
