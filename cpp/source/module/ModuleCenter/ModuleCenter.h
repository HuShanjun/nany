#pragma once
#include "CCenterRegistryStore.h"
#include "Interface/IConsummer.h"

class CModuleCenter : public IConsummer {
public:
    DEFINE_MODULE(30000);
    CModuleCenter() = default;
    ~CModuleCenter() override = default;

    void OnInit() override;
    void UnInit() override;
    int OnTimer(uint32 nInterval) override;
    void OnServerConnect(uint16 nServerID) override;
    void OnServerDisConnect(uint16 nServerID) override;
    bool OnServerStop() override;

    bool RegisterServer(uint32 nServerID, uint32 nTypeID);

private:
    CCenterRegistryStore m_registry;
};
