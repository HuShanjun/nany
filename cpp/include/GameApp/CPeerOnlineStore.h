#pragma once

#include <cstdint>
#include <set>

class CPeerOnlineStore {
public:
    void Add(uint32_t nServerID) {
        if (nServerID == 0) {
            return;
        }
        m_set.insert(nServerID);
    }

    void Remove(uint32_t nServerID) { m_set.erase(nServerID); }

    bool Has(uint32_t nServerID) const { return m_set.find(nServerID) != m_set.end(); }

    size_t Size() const { return m_set.size(); }

private:
    std::set<uint32_t> m_set;
};
