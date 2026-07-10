#include "GameApp/CNetComp.h"

namespace IDAllocator {
    static uint32_t s_nID = 0;
    static std::vector<uint32_t> s_vecFreeIDs;

    uint32_t Alloc() {
        if (s_vecFreeIDs.empty()) {
            return s_nID++;
        }
        uint32_t nID = s_vecFreeIDs.back();
        s_vecFreeIDs.pop_back();
        return nID;
    }

    void Free(uint32_t nID) {
        s_vecFreeIDs.push_back(nID);
    }

    void Reset() {
        s_nID = 0;
        s_vecFreeIDs.clear();
    }
};

