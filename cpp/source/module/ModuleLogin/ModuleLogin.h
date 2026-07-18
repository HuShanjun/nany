#pragma once
#include "Interface/IConsummer.h"

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
};
