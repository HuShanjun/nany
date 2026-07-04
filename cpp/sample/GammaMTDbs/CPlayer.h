#pragma once

#include <cstdint>

class CPlayer{
public:
    CPlayer();
    ~CPlayer();
    void SetId(int32_t nId);
    void SetName(const char* szName);
    void SetLevel(uint32_t nLevel);
    void SetGold(uint64_t nGold);
    int32_t GetId() const;
    const char* GetName() const;
    uint32_t GetLevel() const;
    uint64_t GetGold() const;
private:
    int32_t m_nId;
    char m_szName[64];
    uint32_t m_nLevel;
    uint64_t m_nGold;
};
