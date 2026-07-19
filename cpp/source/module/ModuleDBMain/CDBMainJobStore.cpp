#include "CDBMainJobStore.h"

void CDBMainJobStore::AddPeer(uint32_t nServerID) { m_peers.Add(nServerID); }

void CDBMainJobStore::RemovePeer(uint32_t nServerID) { m_peers.Remove(nServerID); }

uint32_t CDBMainJobStore::Enqueue(uint32_t nFromServerID) {
    if (!m_peers.Has(nFromServerID)) {
        return 0;
    }
    uint32_t id = m_nNextJob++;
    SDBJob job;
    job.nJobID = id;
    job.nFromServerID = nFromServerID;
    job.nState = 0;
    m_mapJobs[id] = job;
    return id;
}

bool CDBMainJobStore::Complete(uint32_t nJobID) {
    auto it = m_mapJobs.find(nJobID);
    if (it == m_mapJobs.end()) {
        return false;
    }
    it->second.nState = 1;
    return true;
}

bool CDBMainJobStore::HasJob(uint32_t nJobID) const {
    return m_mapJobs.find(nJobID) != m_mapJobs.end();
}
