#pragma once
#include "Interface/IConsummer.h"
#include "CGateSessionStore.h"

class CModuleGate : public IConsummer {
public:
    DEFINE_MODULE(40000);
    CModuleGate() = default;
    ~CModuleGate() override = default;

    void OnInit() override;
    void UnInit() override;
    int OnTimer(uint32 nInterval) override;
    void OnServerConnect(uint16 nServerID) override;
    void OnServerDisConnect(uint16 nServerID) override;
    void OnClientConnect(uint32 nClientID) override;
    void OnClientDisConnect(uint32 nClientID) override;
    bool OnServerStop() override;

    bool BindClientToServer(uint32 nClientID, uint32 nServerID);
    bool ForwardToServer(uint32 nClientID, uint32 nServerID, const void *pData, size_t nSize);
    bool ForwardToClient(uint32 nClientID, const void *pData, size_t nSize);

    CGateSessionStore &GetSessionStore() { return m_store; }

private:
    CGateSessionStore m_store;
};
