
#include "GammaApp/CBaseApp.h"

#include "GammaApp/IComp.h"
#include "GammaApp/IGameApp.h"
#include "GammaCommon/CGammaWindow.h"
#include "GammaCommon/CPathMgr.h"
#include "GammaCommon/CPkgFile.h"
#include "GammaCommon/CThread.h"
#include "GammaCommon/GammaCodeCvs.h"
#include "GammaCommon/GammaProfile.h"
#include "GammaCommon/GammaSignal.h"
#include "GammaCommon/ILog.h"
#include "GammaCommon/cxxopts.hpp"

#include <assert.h>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <format>
#include <signal.h>
#include <stdlib.h>
#include <string>

#ifndef _WIN32
#include <cerrno>
#include <dirent.h>
#else
#include <direct.h>
#endif

IGameAppPtr g_pApp = nullptr;

namespace Gamma {
static CBaseApp *ls_pInst = NULL;

using namespace std;
CBaseApp::CBaseApp(const char *szConfigFile, uint32 nMaxLoopInterval, bool bLogFile,
                   bool bEnableDump)
    : m_pTickMgr(NULL),
      m_bQuit(false),
      m_bSignal(false),
      m_strAppConfig(szConfigFile),
      m_nMaxLoopInterval(nMaxLoopInterval),
      m_tickQuit(this, &CBaseApp::OnCheckQuit),
      m_pAppLog(NULL),
      m_nLogicTime(GetGammaTime()),
      m_pErrLog(NULL),
      m_nCurTime(GetGammaTime()),
      m_nCurFrame(0),
      m_bStarted(false),
      m_nDumpLevel(bEnableDump) {
    GammaAst(!ls_pInst);
    ls_pInst = this;
    g_pApp = this;
}

CBaseApp::~CBaseApp() {
    ls_pInst = NULL;
}

CBaseApp *CBaseApp::Inst() {
    return ls_pInst;
}

bool CBaseApp::Init(int32 nArg, const char **szCmdLine) {
    InstallSigHandler();
    InitCmdLine(nArg, szCmdLine);
    InitPathConfig();
    LoadConfig();
    InitLog();

    m_pTickMgr = new CTickMgr(gammacstring("AppTick", true));
    // 切换到工程路径下
    CPathMgr::SetCurPath(m_strWorkPath.c_str());
    CreateConnMgr();
    return true;
}

int32 CBaseApp::UnInit() {
    // TODO: 实现UnInit
    return 0;
}

void CBaseApp::InitLog() {
    m_nDumpLevel = GetDumpLevel();
    SetLogLevel(GetLogLevel());
    if (IsShowConsole()) {
        Redirect2StdConsole(true);
    }

    if (!filesystem::exists(m_strLogPath)) {
        filesystem::create_directories(m_strLogPath);
    }
    SetLogPath(m_strLogPath.c_str());
    if (IsOpenLogFile()) {
        string szLogFileName = m_strClassName;
        m_pAppLog = GetLogFile(szLogFileName.c_str(), GammaGetCurrentProcessID(), eLPT_All);
        SetGlobLogFun(&CBaseApp::AppLog);

        string szLogErrFileName = szLogFileName + "_Err";
        m_pErrLog = GetLogFile(szLogErrFileName.c_str(), GammaGetCurrentProcessID(), eLPT_All);
        SetGlobErrFun(&CBaseApp::ErrLog);
    }
}

const char *CBaseApp::GetCmdLineByType(const char *szType) {
    // TODO: 实现GetCmdLineByType
    return m_result.count(szType) == 0 ? "" : m_result[szType].as<std::string>().c_str();
}

uint32 CBaseApp::GetCmdLineCount() {
    int count = 0;
    for (auto &it : m_result) {
        count++;
    }
    return count;
}

void CBaseApp::InstallSigHandler() {
    struct SSignalHandler {
        static void OnTerminateSignal(int32 nSignum, void *pContext) {
            CBaseApp::Inst()->m_bSignal = true;
        }

        static void OnCoreDumpSignal(int32 nSignum, void *pContext) {
            CBaseApp::Inst()->SigHandler(nSignum, pContext);
        }
    };

    int32 arySignal[1024];
    uint32 nDumpSignalCount = GetDumpSignal(arySignal, 1024);
    for (uint32 i = 0; i < nDumpSignalCount; i++)
        InstallSignalHandler(arySignal[i], &SSignalHandler::OnCoreDumpSignal);
    uint32 nIgnoreSignalCount = GetIgnoreSignal(arySignal, 1024);
    for (uint32 i = 0; i < nIgnoreSignalCount; i++)
        IgnoreSignal(arySignal[i]);
    uint32 nTerminateSignalCount = GetTerminateSignal(arySignal, 1024);
    for (uint32 i = 0; i < nTerminateSignalCount; i++)
        InstallSignalHandler(arySignal[i], &SSignalHandler::OnTerminateSignal);
}

uint32 CBaseApp::GetDumpSignal(int32 arySignal[], uint32 nCount) {
    int32 nInstallSignal[] = {
#ifdef _WIN32
        SIGSEGV
#else
#ifndef _IOS
        SIGSTKFLT, SIGPOLL, SIGPWR,  SIGSYS,
#endif
        SIGILL,    SIGSEGV, SIGABRT, SIGBUS,
        SIGTRAP,   SIGXCPU, SIGXFSZ
#endif
    };

    nCount = Min<uint32>(nCount, ELEM_COUNT(nInstallSignal));
    memcpy(arySignal, nInstallSignal, nCount * sizeof(nCount));
    return nCount;
}

uint32 CBaseApp::GetIgnoreSignal(int32 arySignal[], uint32 nCount) {
    int32 nIgnoreSignal[] = {
        SIGFPE,
#ifndef _WIN32
        SIGQUIT, SIGUSR1, SIGUSR2, SIGALRM, SIGVTALRM, SIGPROF, SIGIO, SIGHUP, SIGPIPE,
#ifndef _IOS
        SIGPWR,
#endif
#endif
    };

    nCount = Min<uint32>(nCount, ELEM_COUNT(nIgnoreSignal));
    memcpy(arySignal, nIgnoreSignal, nCount * sizeof(nCount));
    return nCount;
}

uint32 CBaseApp::GetTerminateSignal(int32 arySignal[], uint32 nCount) {
    if (nCount == 0)
        return nCount;
    arySignal[0] = SIGTERM;
    return 1;
}

void CBaseApp::AppLog(const char *szLog, size_t nLen, uint32 nLevel) {
    if (Inst() && !Inst()->m_bQuit) {
        static std::string pStrLevel[] = {"", "note", "warn", "info", "error"};
        time_t nCurTime;
        time(&nCurTime);
#ifdef _WIN32
        tm tmCurTime = *::localtime(&nCurTime);
#else
        tm tmCurTime;
        ::localtime_r(&nCurTime, &tmCurTime);
#endif

        char szBuf[256];
        sprintf(szBuf, "[%4d-%02d-%02d_%02d:%02d:%02d-%10d][%s]", tmCurTime.tm_year + 1900,
                tmCurTime.tm_mon + 1, tmCurTime.tm_mday, tmCurTime.tm_hour, tmCurTime.tm_min,
                tmCurTime.tm_sec, (uint32)GetProcessTime(), pStrLevel[nLevel].c_str());
        static size_t nPrefixLen = strlen(szBuf);
        auto pLog = nLevel == eLL_Error ? Inst()->m_pErrLog : Inst()->m_pAppLog;
        pLog->Write(szBuf, nPrefixLen);
        pLog->Write(szLog, nLen);
    }
}

void CBaseApp::ErrLog(const char *szLog, size_t nLen) {
    if (Inst()) {
        time_t nCurTime;
        time(&nCurTime);
#ifdef _WIN32
        tm tmCurTime = *::localtime(&nCurTime);
#else
        tm tmCurTime;
        ::localtime_r(&nCurTime, &tmCurTime);
#endif

        char szBuf[256];
        sprintf(szBuf, "[%4d-%02d-%02d_%02d:%02d:%02d][error]", tmCurTime.tm_year + 1900,
                tmCurTime.tm_mon + 1, tmCurTime.tm_mday, tmCurTime.tm_hour, tmCurTime.tm_min,
                tmCurTime.tm_sec);
        static size_t nPrefixLen = strlen(szBuf);

        Inst()->m_pErrLog->Write(szBuf, nPrefixLen);
        Inst()->m_pErrLog->Write(szLog, nLen);
    }
}

void CBaseApp::OnStarted() {
}

bool CBaseApp::OnClose() {
    return true;
}

void CBaseApp::SetVersion(uint64 nVersion) {
    m_Version = nVersion;
}

void CBaseApp::SigHandler(int32 nSignum, void *pContext) {
    GammaLog << "recv signal " << nSignum << endl;
    GammaLog << "Gen core dump with signal: " << nSignum << endl;

    char szDumpFileName[256];
    time_t nCurTime;
    time(&nCurTime);
#ifdef _WIN32
    tm tmCurTime = *::localtime(&nCurTime);
#else
    tm tmCurTime;
    ::localtime_r(&nCurTime, &tmCurTime);
#endif
    const auto &version = Inst()->GetShellVersion();
    std::string strDumpFileName = std::format(
        "{}_{}.{}.{}.{}.{}_{}_{}-{}-{}-{}.{}.{}.core", m_strClassName, version.GetReserve(),
        version.GetMajor(), version.GetMinor(), version.GetRevision(), version.GetBuild(),
        GammaGetCurrentProcessID(), tmCurTime.tm_year + 1900, tmCurTime.tm_mon + 1,
        tmCurTime.tm_mday, tmCurTime.tm_hour, tmCurTime.tm_min, tmCurTime.tm_sec);
    if (m_nDumpLevel)
        DebugGenMiniDump(pContext, strDumpFileName.c_str(), m_nDumpLevel > 1);
    PrintStack(2048, __LINE__, GammaLog);
    // RaiseSignal( 9 );
}

bool CBaseApp::OnCheckFrame() {
    return !m_bQuit;
}

void CBaseApp::OnCheckSysMsg() {
    STATCK_LOG(this);
    CGammaWindow::MessagePump();
    PROCESS_LOG;
}

void CBaseApp::OnQuit() {
    m_pConnMgr->StopAllService();
    m_CompMgr.OnQuit();
    SAFE_RELEASE(m_pConnMgr);
}

void CBaseApp::Prepare() {
    GammaLog << "CBaseApp::OnPreStart() end......\t" << m_nLogicTime << endl;
    m_bStarted = true;
    m_CompMgr.OnStarted();
    OnStarted();
}

void CBaseApp::Cleanup() {
    GammaLog << "Cleanup FileMgr" << endl;
    GetGammaFileMgr().Exit();
    FlushAllLog();

    SAFE_DELETE(m_pTickMgr);
    // 重要的位置需要将Log即刻保存
    FlushAllLog();
    SAFE_RELEASE(m_pAppLog);
    SAFE_RELEASE(m_pErrLog);
    Redirect2StdConsole(false);
}

bool CBaseApp::UpdateOneFrame() {
    STATCK_LOG(this);
    PROCESS_LOG;
    if (!OnCheckFrame())
        return false;

    PROCESS_LOG;
    PROFILE_START(CApp_OnCheckSysMsg);
    OnCheckSysMsg();
    PROCESS_LOG;
    PROFILE_END(CApp_OnCheckSysMsg);

    if (!IsStarted())
        return true;

    OnLoop();

    PROCESS_LOG;
    if (m_bSignal) {
        Close();
        m_bSignal = false;
    }

    PROCESS_LOG;

#ifdef GAMMA_CHECK_PROFILE
    GetProfileMgr().FrameMove();
#endif
    return true;
}

void CBaseApp::Run() {
    m_pTickMgr->Reset();
    m_nLogicTime = GetGammaTime();

    Prepare();
    PROFILE_START(CApp_Run);
    while (UpdateOneFrame())
        NULL;
    PROFILE_END(CApp_Run);
    m_pTickMgr->Reset();
    // 重要的位置需要将Log即刻保存
    FlushAllLog();
    GammaLog << "CBaseApp::OnQuit() end......" << endl;
    OnQuit();
    GammaLog << "CBaseApp::Cleanup() end......" << endl;

    LogProfile();
    Cleanup();
}

void CBaseApp::OnIdle() {
    GetGammaFileMgr().Flush(10);
}

void CBaseApp::OnLoop() {
    STATCK_LOG(this);
    ++m_nCurFrame;
    uint32 nDelta = GetGammaTime() - m_nCurTime;
    m_nCurTime = GetGammaTime();
    int64 nTimeError = m_nLogicTime - m_nCurTime;

    PROCESS_LOG;
    if (nTimeError <= 0) {
        PROCESS_LOG;
        PROFILE_START(PushLogicTime);
        m_nLogicTime += GetBaseCyc();
        if (!m_bStopTick) {
            m_pTickMgr->Update(GetBaseCyc());
        }
        PROFILE_END(PushLogicTime);
        PROCESS_LOG;
    } else {
        PROCESS_LOG;
        PROFILE_START(OnIdle);
        if (!m_bStopTick) {
            OnIdle();
        }
        PROFILE_END(OnIdle);
        nTimeError = m_nLogicTime - GetGammaTime();

        PROCESS_LOG;
        if (nTimeError > 0 && m_nMaxLoopInterval) {
            if (nTimeError > m_nMaxLoopInterval)
                nTimeError = (uint32)m_nMaxLoopInterval;
            GammaSleep((uint32)nTimeError);
        }
        PROCESS_LOG;
    }

    PROFILE_START(NetCheck);
    m_pConnMgr->Check(0);
    PROCESS_LOG;
    PROFILE_END(NetCheck);
}

bool CBaseApp::Close() {
    if (m_tickQuit.IsRegisted())
        return true;
    if (!OnClose())
        return false;
    m_pTickMgr->AddTick(&m_tickQuit, 33, INVALID_16BITID);
    return true;
}

void CBaseApp::Quit() {
    if (m_tickQuit.IsRegisted())
        return;
    m_pTickMgr->AddTick(&m_tickQuit, 33, INVALID_16BITID);
}

void CBaseApp::OnCheckQuit() {
    if (!m_pConnMgr->StopAllConnect())
        return;
    m_pTickMgr->DelTick(&m_tickQuit);
    m_bQuit = true;
}

void CBaseApp::Register(CTick *pTick, uint32 nStart, uint32 uCyc, uint16 nTickID) {
    m_pTickMgr->AddTick(pTick, nStart, uCyc, nTickID);
}

void CBaseApp::UnRegister(CTick *pTick) {
    pTick->Stop();
}

int64 CBaseApp::GetCurTickTime() const {
    return m_pTickMgr->GetCurUpdateTime();
}

void CBaseApp::SetCurUpdateTime(uint32 nValue) {
    m_pTickMgr->SetCurUpdateTime(nValue);
}

void CBaseApp::LogProfile() {
#ifdef GAMMA_CHECK_PROFILE
    string strLog;
    gammasstream strStream(strLog);
    uint32 nEndFrame;
    uint32 nStarFrame;
    GetProfileMgr().GetFrameRange(nStarFrame, nEndFrame);

    strStream << "<root>\n";
    for (uint32 i = nStarFrame; i <= nEndFrame; i++) {
        uint32 nTotalTime = GetProfileMgr().GetFrameMaxCost(i);
        uint32 nCount = GetProfileMgr().GetFrameProfileCount(i);
        strStream << "\t<frame t=\'" << nTotalTime << "\' n=\'" << i << "\'>\n";
        for (size_t j = 0; j < nCount; j++) {
            uint32 nCostTime;
            uint32 nHitCount;
            CProfile *pProfile = GetProfileMgr().GetFrameProfile(i, j, nCostTime, nHitCount);
            strStream << "\t\t<" << pProfile->GetLabel() << " t=\'" << nCostTime << "\' n=\'"
                      << nHitCount << "\'/>\n";
        }
        strStream << "\t</frame>\n";
    }
    strStream << "</root>\n";

    time_t nCurTime;
    time(&nCurTime);
    char szName[256];
    gammasstream(szName, ELEM_COUNT(szName))
        << GetLogPath() << m_strClassName << "_profile_" << GammaGetCurrentProcessID() << "_"
        << nCurTime << ".xml";
    FILE *pFile = fopen(szName, "wb");
    if (!pFile)
        return;
    fwrite(strLog.c_str(), strLog.size(), 1, pFile);
    fclose(pFile);
#endif
}

std::string CBaseApp::GetExePath() {
    if (m_strBinPath.empty()) {
        char szBinPath[2048];
        GammaGetCurrentProcessPath(szBinPath, 2000);
        GammaString(szBinPath);
        char *szFind = strrchr(szBinPath, '/');
        m_strBinPath = szFind ? string(szBinPath, szFind + 1 - szBinPath) : "./";
        CPathMgr::ShortPath(m_strBinPath);
    }
    return m_strBinPath;
}

std::string CBaseApp::GetExeName() {
    std::string exe_name = GetExePath();
    size_t pos = exe_name.find_last_of('/');
    if (pos != std::string::npos) {
        return exe_name.substr(pos + 1);
    }
    return exe_name;
}

void CBaseApp::CreateConnMgr(uint32 nDisconnectTime) {
    m_pConnMgr = Gamma::CreateConnMgr(nDisconnectTime, true);
}

IComp *CBaseApp::GetComp(uint8_t nCompID) {
    return m_CompMgr.GetComp(nCompID);
}

IComp *CBaseApp::AddComp(uint8_t nCompID, IComp *pComp) {
    return m_CompMgr.AddComp(nCompID, pComp);
}

void CBaseApp::DelComp(uint8_t nCompID) {
    m_CompMgr.DelComp(nCompID);
}

bool CBaseApp::HasAppSetting(const char *szSection, const char *szKey) const {
    return m_AppConfig.contains(szSection)
           && m_AppConfig[szSection].as<toml::table>()->contains(szKey);
}

int32 CBaseApp::GetAppSettingInt(const char *szSection, const char *szKey) const {
    return m_AppConfig[szSection][szKey].value_or(0);
}

std::string CBaseApp::GetAppSetting(const char *szSection, const char *szKey) const {
    return m_AppConfig[szSection][szKey].value_or("");
}
} // namespace Gamma

IGameAppPtr GetGameApp() {
    return CBaseApp::Inst();
}