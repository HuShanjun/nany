#include "ModuleGame.h"
#include "GammaCommon/ILog.h"
#include "Interface/IGameApp.h"

void CModuleGame::OnInit() {
    auto pApp = GetGameApp();
    Log::Info("ModuleGame::OnInit moduleId={} serverName={} serverType={} serverId={}",
              GetModuleID(), pApp ? pApp->GetServerName() : std::string{},
              pApp ? pApp->GetServerType() : std::string{}, pApp ? pApp->GetServerID() : 0u);
}

void CModuleGame::UnInit() { Log::Info("ModuleGame::UnInit"); }

int CModuleGame::OnTimer(uint32 /*nInterval*/) { return 0; }

void CModuleGame::OnServerConnect(uint16 nServerID) {
    m_players.AddPeer(nServerID);
    Log::Info("ModuleGame::OnServerConnect serverId={}", nServerID);
}

void CModuleGame::OnServerDisConnect(uint16 nServerID) {
    m_players.RemovePeer(nServerID);
    Log::Info("ModuleGame::OnServerDisConnect serverId={}", nServerID);
}

bool CModuleGame::OnServerStop() {
    Log::Info("ModuleGame::OnServerStop");
    return true;
}

bool CModuleGame::PlayerEnter(uint64_t nPlayerID, uint32 nGateServerID) {
    bool ok = m_players.PlayerEnter(nPlayerID, nGateServerID);
    if (!ok) {
        Log::Error("ModuleGame::PlayerEnter failed playerId={} gateServerId={}", nPlayerID,
                   nGateServerID);
        return false;
    }
    Log::Info("ModuleGame::PlayerEnter playerId={} gateServerId={}", nPlayerID, nGateServerID);
    return true;
}

void CModuleGame::PlayerLeave(uint64_t nPlayerID) {
    m_players.PlayerLeave(nPlayerID);
    Log::Info("ModuleGame::PlayerLeave playerId={}", nPlayerID);
}
