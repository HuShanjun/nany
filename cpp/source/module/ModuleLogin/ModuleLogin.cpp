#include "ModuleLogin.h"
#include "Interface/IGameApp.h"
#include "GammaCommon/ILog.h"

void CModuleLogin::OnInit() {
    auto pApp = GetGameApp();
    Log::Info("ModuleLogin::OnInit moduleId={} serverName={} serverType={} serverId={}",
              GetModuleID(),
              pApp ? pApp->GetServerName() : std::string{},
              pApp ? pApp->GetServerType() : std::string{},
              pApp ? pApp->GetServerID() : 0u);
}

void CModuleLogin::UnInit() {
    Log::Info("ModuleLogin::UnInit");
}

int CModuleLogin::OnTimer(uint32 /*nInterval*/) {
    return 0;
}

void CModuleLogin::OnServerConnect(uint16 nServerID) {
    Log::Info("ModuleLogin::OnServerConnect serverId={}", nServerID);
}

void CModuleLogin::OnServerDisConnect(uint16 nServerID) {
    Log::Info("ModuleLogin::OnServerDisConnect serverId={}", nServerID);
}

bool CModuleLogin::OnServerStop() {
    Log::Info("ModuleLogin::OnServerStop");
    return true;
}
