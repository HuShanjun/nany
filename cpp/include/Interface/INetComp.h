#pragma once

#include "Interface/IComp.h"
#include "GameApp/GameAppHelp.h"

#include <cstdint>
#include <map>

class CGameConnection;

class INetComp : public IComp
{
    DEFINE_COMP_ID(NetComp, EGameAppCompID::NetComp);
public:
    virtual void SendMsgToClient(uint32_t nClientID, void* pData, size_t nSize) = 0;
    virtual void SendMsgToCenter(void* pData, size_t nSize) = 0;
    virtual void SendMsgToServer(uint32_t nServerID, void* pData, size_t nSize) = 0;
    virtual void SendMsgToLogin(void* pData, size_t nSize) = 0;
};