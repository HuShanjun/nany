#pragma once
#include "CLoginAuthStore.h"
#include "GameApp/CPeerOnlineStore.h"
#include "Interface/IConsummer.h"

#include <string>

class CModuleLogin : public IConsummer {
public:
    DEFINE_MODULE(10000);
    CModuleLogin() = default;
    ~CModuleLogin() override = default;

    void OnInit() override;
    void UnInit() override;
    int OnTimer(uint32 nInterval) override;
    void OnServerConnect(uint16 nServerID) override;
    void OnServerDisConnect(uint16 nServerID) override;
    bool OnServerStop() override;

    uint32_t SubmitAuth(const std::string &account);
    bool CompleteAuth(uint32_t ticket, bool ok);

private:
    CPeerOnlineStore m_peers;
    CLoginAuthStore m_auth;
};
