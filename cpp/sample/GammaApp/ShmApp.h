#pragma once

#include "GammaApp/CBaseApp.h"
#include <filesystem>
#include "Shm/CCharDataMgr.h"

namespace fs = std::filesystem;

class CShmApp : public Gamma::CBaseApp
{
    using path = std::filesystem::path;
public:
    CShmApp();
    ~CShmApp();

    virtual bool Init(int32 nArg, const char** szCmdLine) override;
    /* 重写基类方法 */
    virtual void OnReady() override {}
    virtual void InitCmdLine(int argc, const char** argv) override;
    virtual void LoadConfig() override;
    virtual void InitPathConfig() override;
    virtual uint32 GetLogLevel() override;
    virtual uint8 GetDumpLevel() override;
    virtual bool IsShowConsole() override;
    virtual bool IsOpenLogFile() override;
    virtual void OnStarted() override;
    virtual void OnCheckQuit() override;

    CCharDataMgr* GetCharDataMgr() { return m_pCharDataMgr; }

private:
    fs::path m_devopsPath;
    CCharDataMgr* m_pCharDataMgr;
};