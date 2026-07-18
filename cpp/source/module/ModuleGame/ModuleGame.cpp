#include "ModuleGame.h"
#include "Interface/IGameApp.h"
#include "GammaCommon/ILog.h"

void CModuleGame::OnInit() {
    auto pApp = GetGameApp();
    Log::Info("ModuleGame::OnInit moduleId={} serverName={} serverType={} serverId={}",
              GetModuleID(),
              pApp ? pApp->GetServerName() : std::string{},
              pApp ? pApp->GetServerType() : std::string{},
              pApp ? pApp->GetServerID() : 0u);
}

void CModuleGame::UnInit() {
    Log::Info("ModuleGame::UnInit");
}

int CModuleGame::OnTimer(uint32 /*nInterval*/) {
    return 0;
}

void CModuleGame::OnServerConnect(uint16 nServerID) {
    Log::Info("ModuleGame::OnServerConnect serverId={}", nServerID);
}

void CModuleGame::OnServerDisConnect(uint16 nServerID) {
    Log::Info("ModuleGame::OnServerDisConnect serverId={}", nServerID);
}

bool CModuleGame::OnServerStop() {
    Log::Info("ModuleGame::OnServerStop");
    return true;
}
