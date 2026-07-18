#pragma once
#include "Interface/IConsummer.h"

class CModuleGame : public IConsummer {
public:
    DEFINE_MODULE(20000);
    CModuleGame() = default;
    ~CModuleGame() override = default;

    void OnInit() override;
    void UnInit() override;
    int OnTimer(uint32 nInterval) override;
    void OnServerConnect(uint16 nServerID) override;
    void OnServerDisConnect(uint16 nServerID) override;
    bool OnServerStop() override;
};
