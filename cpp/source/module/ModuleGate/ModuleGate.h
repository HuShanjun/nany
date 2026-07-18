#pragma once
#include "Interface/IConsummer.h"

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
    bool OnServerStop() override;
};
