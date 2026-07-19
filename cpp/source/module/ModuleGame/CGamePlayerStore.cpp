#include "CGamePlayerStore.h"

void CGamePlayerStore::AddPeer(uint32_t nServerID) { m_peers.Add(nServerID); }

void CGamePlayerStore::RemovePeer(uint32_t nServerID) {
    m_peers.Remove(nServerID);
    for (auto it = m_mapPlayers.begin(); it != m_mapPlayers.end();) {
        if (it->second.nGateServerID == nServerID) {
            it = m_mapPlayers.erase(it);
        } else {
            ++it;
        }
    }
}

bool CGamePlayerStore::PlayerEnter(uint64_t nPlayerID, uint32_t nGateServerID) {
    if (nPlayerID == 0 || !m_peers.Has(nGateServerID)) {
        return false;
    }
    SGamePlayer player;
    player.nPlayerID = nPlayerID;
    player.nGateServerID = nGateServerID;
    m_mapPlayers[nPlayerID] = player;
    return true;
}

void CGamePlayerStore::PlayerLeave(uint64_t nPlayerID) { m_mapPlayers.erase(nPlayerID); }

bool CGamePlayerStore::HasPlayer(uint64_t nPlayerID) const {
    return m_mapPlayers.find(nPlayerID) != m_mapPlayers.end();
}
