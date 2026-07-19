#pragma once

#include "GameApp/CPeerOnlineStore.h"

#include <cstdint>
#include <map>

struct SRegisteredServer {
    uint32_t nServerID = 0;
    uint32_t nTypeID = 0;
};

class CCenterRegistryStore {
public:
    void AddPeer(uint32_t nServerID);
    void RemovePeer(uint32_t nServerID);
    bool RegisterServer(uint32_t nServerID, uint32_t nTypeID);
    void UnregisterServer(uint32_t nServerID);
    bool IsRegistered(uint32_t nServerID) const;
    bool HasPeer(uint32_t nServerID) const { return m_peers.Has(nServerID); }

private:
    CPeerOnlineStore m_peers;
    std::map<uint32_t, SRegisteredServer> m_mapServers;
};
