#include "GameApp/GameApp.h"
#include "GammaApp/CBaseApp.h"
#include "GameApp/ModuleMgr.h"
#include "NetComp/CNetComp.h"

#include "GammaCommon/ILog.h"
#include "toml++/toml.hpp"
#include "Interface/IComp.h"
#include "NetComp/CNetComp.h"


#include <chrono>
#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>

namespace fs = std::filesystem;

CGameApp::CGameApp()
    : CBaseApp()
{
    SetGameApp(this);
}

CGameApp::~CGameApp()
{
}

int64 CGameApp::GetCurTime() const {
    return CBaseApp::GetCurTime();
}

uint32 CGameApp::GetCurFrame() const {
    return CBaseApp::GetCurFrame();
}

uint32 CGameApp::GetBaseCyc() const {
    return CBaseApp::GetBaseCyc();
}

void CGameApp::Register(CTick *pTick, uint32 uCyc, uint16 nTickID) {
    CBaseApp::Register(pTick, uCyc, nTickID);
}

void CGameApp::Register(CTick *pTick, uint32 nStart, uint32 uCyc, uint16 nTickID) {
    CBaseApp::Register(pTick, nStart, uCyc, nTickID);
}

void CGameApp::UnRegister(CTick *pTick) {
    CBaseApp::UnRegister(pTick);
}

int64 CGameApp::GetCurTickTime() const {
    return CBaseApp::GetCurTickTime();
}

IConnectionMgr *CGameApp::GetConnMgr() const {
    return CBaseApp::GetConnMgr();
}

const char* CGameApp::GetBinPath() {
    // GetExePath caches into CBaseApp::m_strBinPath
    static_cast<void>(GetExePath());
    return CBaseApp::GetBinPath();
}

void CGameApp::OnReady() {
}

void CGameApp::InitCmdLine(int argc, const char **argv) {
    cxxopts::Options options("GameApp", "GameApp");
    options.add_options()
        ("d,devops", "devops path of the program", cxxopts::value<std::string>())
        ("n, name", "name of the program", cxxopts::value<std::string>())
    ;
    try {
        m_result = options.parse(argc, argv);
        if (m_result.count("help")) {
            std::cout << options.help() << std::endl;
            std::exit(0);
        }
        if (m_result.count("devops")) {
            m_strDevopsPath = m_result["devops"].as<std::string>();
        }
        m_strServerName = m_result["name"].as<std::string>();
        if (m_strServerName.empty()) {
            throw std::runtime_error("name is empty");
            return;
        }
    } catch (std::exception& e) {
        std::cerr << "Error parsing options: " << e.what() << std::endl;
        std::exit(1);
    }
}

void CGameApp::InitPathConfig() {
    if (m_strDevopsPath.empty()) {
        throw std::runtime_error("devops path is empty");
        return;
    }
    if (!fs::exists(m_strDevopsPath)) {
        throw std::runtime_error("devops path " + m_strDevopsPath + " not exists");
        return;
    }
    fs::path pathDevops = fs::path(m_strDevopsPath);
    m_strWorkPath   = pathDevops / m_strServerName;
    m_strDataPath   = pathDevops / "data";
    m_strEtcPath    = pathDevops / "etc";
    m_strLogPath    = pathDevops / "log";

    // 创建工作路径
    if (!fs::exists(m_strWorkPath)) {
        fs::create_directory(m_strWorkPath);
    }
}

void CGameApp::RegisterComp() {
    IComp *pNetComp = CBaseApp::AddComp<CNetComp>(*m_AppConfig["Server"].as_table());
}

void CGameApp::LoadConfig() {
    fs::path pathEtc = fs::path(m_strEtcPath);
    std::string strMachineConfig = (pathEtc / "machine.toml").string();
    std::string strProcessModuleConfig = (pathEtc / "process_module.toml").string();
    m_AppConfig = toml::parse_file(strMachineConfig);
}

uint32 CGameApp::GetLogLevel() {
    return m_AppConfig["AppSetting"]["LogLevel"].value<uint32>().value_or(Gamma::ELogLevel::eLL_Info);
}

uint8 CGameApp::GetDumpLevel() {
    return 0;
}

bool CGameApp::IsShowConsole() {
    return m_AppConfig["AppSetting"]["ShowConsole"].value<bool>().value_or(false);
}

bool CGameApp::IsOpenLogFile() {
    return m_AppConfig["AppSetting"]["OpenLogFile"].value<bool>().value_or(false);
}


bool CGameApp::OnInit() {
    if(!LoadServerInfo()){
        return false;
    }
    // 注册组件
    // AddComp()
    // 加载模块
    fs::path etcPath = m_strEtcPath;
    std::string strProcessModuleConfig = (etcPath / "process_module.toml").string();
    if (!CModuleMgr::Instance()->LoadDll(m_strServerType.c_str(), strProcessModuleConfig)) {
        Log::Error("CGameApp::OnInit LoadDll failed for type {}", m_strServerType);
        return false;
    }
    return true;
}

bool CGameApp::OnUnInit() {
    return true;
}
bool CGameApp::LoadServerInfo() {
    toml::table* server_list = m_AppConfig["Server"].as_table();
    if(!server_list || !server_list->contains(m_strServerName)){
        Log::Error("server {} not found", m_strServerName);
        return false;
    }

    auto ServerInfo = server_list->at(m_strServerName).as_table();
    if (!ServerInfo) {
        Log::Error("server {} not found", m_strServerName);
        return false;
    }
    std::string strServerType = ServerInfo->at("type").value_or("");
    uint32 nServerID = ServerInfo->at("ServerID").value_or(0);

    uint32 nServerTypeID = nServerID / 100;
    m_nServerType = nServerTypeID;
    m_strServerType = strServerType;
    m_nServerID = nServerID;

    Log::Info("CGameApp::LoadServerInfo ServerNam = {}, ServerType = {}, ServerID = {}, TypeID = {}", 
                m_strServerName, strServerType, nServerID, nServerTypeID);

    return true;
}
