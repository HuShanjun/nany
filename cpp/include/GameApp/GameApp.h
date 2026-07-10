#pragma once

#include "GammaApp/CBaseApp.h"

class CGameApp : public CBaseApp
{
public:
    CGameApp();
    ~CGameApp();

    virtual void OnReady() override;
    virtual void InitCmdLine(int argc, const char **argv) override;
    virtual void LoadConfig() override;
    virtual void InitPathConfig() override;
    virtual uint32 GetLogLevel() override;
    virtual uint8 GetDumpLevel() override;
    virtual bool IsShowConsole() override;
    virtual bool IsOpenLogFile() override;
};
