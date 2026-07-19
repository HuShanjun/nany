#pragma once

#include "GameApp/CPeerOnlineStore.h"

#include <cstdint>
#include <map>

struct SDBJob {
    uint32_t nJobID = 0;
    uint32_t nFromServerID = 0;
    uint8_t nState = 0; // 0=queued, 1=done
};

class CDBMainJobStore {
public:
    void AddPeer(uint32_t nServerID);
    void RemovePeer(uint32_t nServerID);
    uint32_t Enqueue(uint32_t nFromServerID);
    bool Complete(uint32_t nJobID);
    bool HasJob(uint32_t nJobID) const;

private:
    CPeerOnlineStore m_peers;
    uint32_t m_nNextJob = 1;
    std::map<uint32_t, SDBJob> m_mapJobs;
};
