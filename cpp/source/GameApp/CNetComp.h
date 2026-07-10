#pragma once

#include "Interface/INetComp.h"

class CNetComp : public INetComp
{
    using CConnectionMap = std::map<uint32_t, CGameConnection*>;
public:
    CNetComp();
    ~CNetComp();

    virtual void OnInit() override;
    virtual void OnCleanup() override;
    virtual void OnStarted() override;
    virtual void OnQuit() override;

    void SendMsgToClient(uint32_t nClientID, void* pData, size_t nSize) override;
    void SendMsgToCenter(void* pData, size_t nSize) override;
    void SendMsgToServer(uint32_t nServerID, void* pData, size_t nSize) override;
    void SendMsgToLogin(void* pData, size_t nSize) override;

private:
    CConnectionMap m_mapClient;
    CConnectionMap m_mapServer;
    CConnectionMap m_mapLogin;
    CConnectionMap m_mapCenter;
};