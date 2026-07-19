#pragma once

#include "GameApp/CPeerOnlineStore.h"

#include <cstdint>
#include <map>

struct SGamePlayer {
    uint64_t nPlayerID = 0;
    uint32_t nGateServerID = 0;
};

class CGamePlayerStore {
public:
    void AddPeer(uint32_t nServerID);
    void RemovePeer(uint32_t nServerID);
    bool PlayerEnter(uint64_t nPlayerID, uint32_t nGateServerID);
    void PlayerLeave(uint64_t nPlayerID);
    bool HasPlayer(uint64_t nPlayerID) const;

private:
    CPeerOnlineStore m_peers;
    std::map<uint64_t, SGamePlayer> m_mapPlayers;
};
