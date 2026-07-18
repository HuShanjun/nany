#pragma once
#include "Interface/IConsummer.h"

class CModuleDBMain : public IConsummer {
public:
    DEFINE_MODULE(50000);
    CModuleDBMain() = default;
    ~CModuleDBMain() override = default;

    void OnInit() override;
    void UnInit() override;
    int OnTimer(uint32 nInterval) override;
    void OnServerConnect(uint16 nServerID) override;
    void OnServerDisConnect(uint16 nServerID) override;
    bool OnServerStop() override;
};
