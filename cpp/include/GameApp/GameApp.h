#pragma once

#include "GammaApp/CBaseApp.h"
#include "GammaCommon/CThread.h"
#include <string>

class CGameApp : public CBaseApp, public IGameApp
{
public:
    CGameApp();
    ~CGameApp();

    // IGameApp 实现的接口
    virtual int64 GetCurTime() const override;
    virtual uint32 GetCurFrame() const override;
    virtual uint32 GetBaseCyc() const override;
    virtual void Register(CTick *pTick, uint32 uCyc, uint16 nTickID) override;
    virtual void Register(CTick *pTick, uint32 nStart, uint32 uCyc, uint16 nTickID) override;
    virtual void UnRegister(CTick *pTick) override;
    virtual int64 GetCurTickTime() const override;
    virtual IConnectionMgr *GetConnMgr() const override;

    // CBaseApp 实现的接口
    virtual void OnReady() override;
    virtual void InitCmdLine(int argc, const char **argv) override;
    virtual void LoadConfig() override;
    virtual void InitPathConfig() override;
    virtual void RegisterComp() override;
    virtual uint32 GetLogLevel() override;
    virtual uint8 GetDumpLevel() override;
    virtual bool IsShowConsole() override;
    virtual bool IsOpenLogFile() override;

    virtual bool OnInit() override;
    virtual bool OnUnInit() override;

    uint32 GetServerID() const override { return m_nServerID; }
    const std::string& GetServerName() const override { return m_strServerName; }
    const std::string& GetServerType() const override { return m_strServerType; }
    const char* GetBinPath() override;

private:
    bool LoadServerInfo();

private:
    uint32 m_nServerID;
    uint32 m_nServerType;
    std::string m_strServerType;
    std::string m_strServerName;
};
