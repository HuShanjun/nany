#include "CGateSessionStore.h"

void CGateSessionStore::AddClient(uint32_t nClientID, int64_t nConnectTime) {
    if (nClientID == 0) {
        return;
    }
    SGateClientSession session;
    session.nClientID = nClientID;
    session.nBoundServerID = 0;
    session.nConnectTime = nConnectTime;
    m_mapClients[nClientID] = session;
}

void CGateSessionStore::RemoveClient(uint32_t nClientID) {
    m_mapClients.erase(nClientID);
}

bool CGateSessionStore::HasClient(uint32_t nClientID) const {
    return m_mapClients.find(nClientID) != m_mapClients.end();
}

SGateClientSession *CGateSessionStore::FindClient(uint32_t nClientID) {
    auto it = m_mapClients.find(nClientID);
    return it == m_mapClients.end() ? nullptr : &it->second;
}

const SGateClientSession *CGateSessionStore::FindClient(uint32_t nClientID) const {
    auto it = m_mapClients.find(nClientID);
    return it == m_mapClients.end() ? nullptr : &it->second;
}

void CGateSessionStore::AddServer(uint32_t nServerID) {
    m_peers.Add(nServerID);
}

void CGateSessionStore::RemoveServer(uint32_t nServerID) {
    m_peers.Remove(nServerID);
    for (auto &[id, session] : m_mapClients) {
        (void)id;
        if (session.nBoundServerID == nServerID) {
            session.nBoundServerID = 0;
        }
    }
}

bool CGateSessionStore::HasServer(uint32_t nServerID) const {
    return m_peers.Has(nServerID);
}

bool CGateSessionStore::BindClientToServer(uint32_t nClientID, uint32_t nServerID) {
    if (!HasClient(nClientID) || !HasServer(nServerID)) {
        return false;
    }
    m_mapClients[nClientID].nBoundServerID = nServerID;
    return true;
}

bool CGateSessionStore::CanForwardToServer(uint32_t nClientID, uint32_t nServerID) const {
    return HasClient(nClientID) && HasServer(nServerID);
}

bool CGateSessionStore::CanForwardToClient(uint32_t nClientID) const {
    return HasClient(nClientID);
}
