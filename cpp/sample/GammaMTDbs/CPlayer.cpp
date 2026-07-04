#include "CPlayer.h"
#include <cstring>

CPlayer::CPlayer()
    : m_nId(0)
    , m_szName("")
    , m_nLevel(0)
    , m_nGold(0)
{
}

CPlayer::~CPlayer()
{
}

void CPlayer::SetId(int32_t nId){
    if(nId == m_nId)
        return;
    m_nId = nId;
}

void CPlayer::SetName(const char* szName){
    if(strcmp(szName, m_szName) == 0)
        return;
    strncpy(m_szName, szName, sizeof(m_szName) - 1);
    m_szName[sizeof(m_szName) - 1] = '\0';
}

void CPlayer::SetLevel(uint32_t nLevel){
    if(nLevel == m_nLevel)
        return;
    m_nLevel = nLevel;
}

void CPlayer::SetGold(uint64_t nGold){
    if(nGold == m_nGold)
        return;
    m_nGold = nGold;
}

int32_t CPlayer::GetId() const{
    return m_nId;
}

const char* CPlayer::GetName() const{
    return m_szName;
}

uint32_t CPlayer::GetLevel() const{
    return m_nLevel;
}

uint64_t CPlayer::GetGold() const{
    return m_nGold;
}
