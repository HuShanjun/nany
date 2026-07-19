#include "CCenterRegistryStore.h"

void CCenterRegistryStore::AddPeer(uint32_t nServerID) { m_peers.Add(nServerID); }

void CCenterRegistryStore::RemovePeer(uint32_t nServerID) {
    UnregisterServer(nServerID);
    m_peers.Remove(nServerID);
}

bool CCenterRegistryStore::RegisterServer(uint32_t nServerID, uint32_t nTypeID) {
    if (!m_peers.Has(nServerID)) {
        return false;
    }
    SRegisteredServer info;
    info.nServerID = nServerID;
    info.nTypeID = nTypeID;
    m_mapServers[nServerID] = info;
    return true;
}

void CCenterRegistryStore::UnregisterServer(uint32_t nServerID) { m_mapServers.erase(nServerID); }

bool CCenterRegistryStore::IsRegistered(uint32_t nServerID) const {
    return m_mapServers.find(nServerID) != m_mapServers.end();
}
