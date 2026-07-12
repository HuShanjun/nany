#pragma once
#include "GammaCommon/CVirtualFun.h"
#include "Interface/IComp.h"
#include "GammaCommon/CGammaWindow.h"
#include "GammaCommon/CDomXml.h"
#include "GammaCommon/CVersion.h"
#include "GammaCommon/CIniFile.h"
#include "GammaCommon/CRefshare.h"
#include "GammaConnects/IConnectionMgr.h"
#include "Interface/IGameApp.h"
#include "cxxopts/cxxopts.hpp"
#include "toml++/toml.hpp"
#include <set>
#include <string>

#if (defined(_WIN32) && defined(GAMMA_DLL))
#if defined(GAMMA_FRAMEWORK_EXPORTS)
#define GAMMA_FRAMEWORK_API __declspec(dllexport)
#else
#define GAMMA_FRAMEWORK_API __declspec(dllimport)
#endif
#else
#define GAMMA_FRAMEWORK_API
#endif

namespace Gamma {
/// 定义Application对象
#define DEFINE_APP(ObjectClass)                                                                    \
    int32 main(int nArg, const char *szArg[]) {                                                    \
        static ObjectClass App;                                                                    \
        App.Init(nArg, (const char **)szArg);                                                      \
        App.Run();                                                                                 \
        return App.UnInit();                                                                       \
    }

#define BASE_FRAME_INTERVAL 33

class GAMMA_FRAMEWORK_API CBaseApp {
    typedef std::vector<gammacstring> CAppCmdList;

protected:
    std::string m_strClassName;
    std::string m_strProgressName;
    cxxopts::ParseResult m_result;
    toml::table m_AppConfig;
    toml::table m_MainConfig;

    uint32 m_nMaxLoopInterval;
    int64 m_nLogicTime;
    int64 m_nCurTime;
    uint32 m_nCurFrame;
    TTickFun<CBaseApp> m_tickQuit;
    CVersion m_Version;
    uint8 m_nDumpLevel;
    bool m_bStarted;

    CTickMgr *m_pTickMgr;

    bool m_bQuit;
    bool m_bSignal;
    bool m_bStopTick;

    std::string m_strDevopsPath;
    std::string m_strBinPath;
    std::string m_strLogPath;
    std::string m_strAppConfig;
    std::string m_strMainConfig;
    std::string m_strWorkPath;
    std::string m_strDataPath;
    std::string m_strEtcPath;

    // log 相关，派生类需要填写这些字段
    ILog *m_pAppLog; /// 系统错误日志
    ILog *m_pErrLog; /// 系统Assert日志

    IConnectionMgr *m_pConnMgr;
    CompMgr m_CompMgr;

    virtual void InstallSigHandler();
    virtual uint32 GetDumpSignal(int32 arySignal[], uint32 nCount);
    virtual uint32 GetIgnoreSignal(int32 arySignal[], uint32 nCount);
    virtual uint32 GetTerminateSignal(int32 arySignal[], uint32 nCount);

    virtual void InitLog();

    // app生命周期相关
    bool UpdateOneFrame();
    virtual void Prepare();
    virtual void Cleanup();
    virtual const char *OnQueryTitle() { return "CApp"; }
    virtual uint32 OnQueryIconID() { return 0; }
    virtual void OnStarted();
    virtual bool OnClose();
    virtual void OnQuit();
    virtual void OnCheckQuit();
    virtual void SigHandler(int32 nSignum, void *pContext);

    CBaseApp(const char *szConfigFile = "config.toml",
             uint32 nMaxLoopInterval = BASE_FRAME_INTERVAL, bool bLogFile = true,
             bool bEnableDump = true);
    virtual ~CBaseApp();

    static void AppLog(const char *szLog, size_t nLen, uint32 nLevel);
    static void ErrLog(const char *szLog, size_t nLen);
    
    /* 具体实现类需要实现的接口 */
    virtual void OnReady() = 0;
    virtual void InitCmdLine(int argc, const char **argv) = 0;
    virtual void LoadConfig() = 0;
    virtual void InitPathConfig() = 0;
    virtual void RegisterComp() = 0;
    virtual uint32 GetLogLevel() = 0;
    virtual uint8 GetDumpLevel() = 0;
    virtual bool IsShowConsole() = 0;
    virtual bool IsOpenLogFile() = 0;

    virtual bool OnInit() = 0;
    virtual bool OnUnInit() = 0;
    /* 具体实现类需要实现的接口 */

    void SetGameApp(IGameAppPtr pGameApp);

public:
    static CBaseApp *Inst();

    virtual bool Init(int32 nArg, const char **szCmdLine);
    virtual int32 UnInit();

    uint32 GetCmdLineCount();
    const char *GetCmdLineByType(const char *szType);

    virtual void LogProfile();
    void SetVersion(uint64 nVersion);

    CTickMgr *GetTickMgr() const { return m_pTickMgr; }
    bool IsStarted() const { return m_bStarted; }

    const char *GetLogPath() const { return m_strLogPath.c_str(); }
    const char *GetBinPath() const { return m_strBinPath.c_str(); }
    const char *GetDataPath() const { return m_strDataPath.c_str(); }
    const char *GetAppConfigName() const { return m_strAppConfig.c_str(); }
    const char *GetEtcPath() const { return m_strEtcPath.c_str(); }
    const char *GetModuleName() const { return m_strClassName.c_str(); }
    const char *GetProgressName() const { return m_strProgressName.c_str(); }
    CVersion GetShellVersion() const { return m_Version; }
    int64 GetCurTime() const { return m_nCurTime; }
    uint32 GetCurFrame() const { return m_nCurFrame; }
    uint32 GetBaseCyc() const { return 33; }
    bool IsQuit() const { return m_bQuit; }
    std::string GetExePath();
    std::string GetExeName();

    virtual void Run();

    virtual bool Close();
    virtual void Quit();

    virtual bool OnCheckFrame();
    virtual void OnCheckSysMsg();
    virtual void OnIdle();
    virtual void OnLoop();

    void StopTick() { m_bStopTick = !m_bStopTick; }

    void Register(CTick *pTick, uint32 uCyc, uint16 nTickID) {
        Register(pTick, uCyc, uCyc, nTickID);
    }
    void Register(CTick *pTick, uint32 nStart, uint32 uCyc, uint16 nTickID);
    void UnRegister(CTick *pTick);
    int64 GetCurTickTime() const;
    void SetCurUpdateTime(uint32 nValue);

    void CreateConnMgr(uint32 nDisconnectTime = 30000);
    virtual IConnectionMgr *GetConnMgr() const { return m_pConnMgr; }

    /* 获取组件 */
    IComp *GetComp(uint8_t nCompID);
    IComp *AddComp(uint8_t nCompID, IComp *pComp);
    void DelComp(uint8_t nCompID);

    template <typename T, typename... Args>
    IComp *AddComp(Args &&...args);

    template <typename T>
    void DelComp();

    // 获取App配置
    bool HasAppSetting(const char *szSection, const char *szKey) const;
    int32 GetAppSettingInt(const char *szSection, const char *szKey) const;
    std::string GetAppSetting(const char *szSection, const char *szKey) const;
    toml::table GetAppSettingTable(const char *szSection) const { return *m_AppConfig[szSection].as_table(); }
};

template <typename T>
inline void CBaseApp::DelComp() {
    DelComp(T::GetID());
}

template <typename T, typename... Args>
inline IComp *CBaseApp::AddComp(Args &&...args) {
    T* pComp = new T(std::forward<Args>(args)...);
    return AddComp(T::GetID(), (IComp *)pComp);
}

} // namespace Gamma
