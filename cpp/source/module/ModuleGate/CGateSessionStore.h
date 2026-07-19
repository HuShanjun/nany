#pragma once

#include <cstdint>
#include <map>
#include <set>

struct SGateClientSession {
    uint32_t nClientID = 0;
    uint32_t nBoundServerID = 0;
    int64_t nConnectTime = 0;
};

class CGateSessionStore {
public:
    void AddClient(uint32_t nClientID, int64_t nConnectTime);
    void RemoveClient(uint32_t nClientID);
    bool HasClient(uint32_t nClientID) const;
    SGateClientSession *FindClient(uint32_t nClientID);
    const SGateClientSession *FindClient(uint32_t nClientID) const;

    void AddServer(uint32_t nServerID);
    void RemoveServer(uint32_t nServerID);
    bool HasServer(uint32_t nServerID) const;

    bool BindClientToServer(uint32_t nClientID, uint32_t nServerID);
    bool CanForwardToServer(uint32_t nClientID, uint32_t nServerID) const;
    bool CanForwardToClient(uint32_t nClientID) const;

private:
    std::map<uint32_t, SGateClientSession> m_mapClients;
    std::set<uint32_t> m_setServers;
};
