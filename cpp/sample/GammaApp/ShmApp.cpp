#include "ShmApp.h"
#include "GammaApp/CBaseApp.h"
#include "GammaCommon/CPathMgr.h"
#include "toml++/toml.hpp"
#include "Net/ConnFromClient.h"

#include <iostream>

namespace fs = std::filesystem;
CShmApp::CShmApp() {
}

CShmApp::~CShmApp() {
}

bool CShmApp::Init(int32 nArg, const char **szCmdLine) {
    if (!CBaseApp::Init(nArg, szCmdLine)) {
        return false;
    }
    m_pCharDataMgr = new CCharDataMgr();
    m_pCharDataMgr->OnInit();
    m_pCharDataMgr->Start();

    GammaNote << LogHeader()<<" Init success" << std::endl;
    return true;
}

void CShmApp::InitCmdLine(int argc, const char **argv) {
    cxxopts::Options options("ShmApp", "ShmApp");
    options.add_options()("n,name", "Name of the program",
                          cxxopts::value<std::string>()->default_value("ShmApp"))(
        "d,devops", "devops path of the program", cxxopts::value<std::string>());
    try {
        m_result = options.parse(argc, argv);
        if (m_result.count("help")) {
            std::cout << options.help() << std::endl;
            std::exit(0);
        }

        m_devopsPath = m_result["d"].as<std::string>();
        if (!fs::exists(m_devopsPath)) {
            std::cout << "devops path " << m_devopsPath << " not exists" << std::endl;
            std::exit(0);
        }
        m_strClassName = m_result["n"].as<std::string>();
        if (m_strClassName.empty()) {
            m_strClassName = GetExeName();
        }
    } catch (cxxopts::exceptions::option_has_no_value &e) {
        std::cout << "Option parse exception: " << e.what() << std::endl;
        std::cout << "Please use --help to see the available options" << std::endl;
        std::cout << options.help() << std::endl;
        std::exit(0);
    }
}

void CShmApp::InitPathConfig() {
    m_strBinPath = GetExePath();
    m_strLogPath = (m_devopsPath / "log").string();
    m_strDataPath = (m_devopsPath / "data").string();
    m_strEtcPath = (m_devopsPath / "etc").string();
    fs::path etc_path = m_devopsPath / "etc";
    m_strAppConfig = (etc_path / "config.toml").string();
    m_strMainConfig = (etc_path / "main.toml").string();
    m_strWorkPath = (m_devopsPath / m_strClassName).string();
    if (!fs::exists(m_strWorkPath)) {
        fs::create_directories(m_strWorkPath);
    }
    if (!fs::exists(m_strLogPath)) {
        fs::create_directories(m_strLogPath);
    }
}

void CShmApp::LoadConfig() {
    m_AppConfig = toml::parse_file(m_strAppConfig);
    m_MainConfig = toml::parse_file(m_strMainConfig);
}

uint32 CShmApp::GetLogLevel() {
    return 0;
}

bool CShmApp::IsShowConsole() {
    return m_AppConfig["AppSetting"]["ShowConsole"].value<int32>() == 1;
}

bool CShmApp::IsOpenLogFile() {
    return m_AppConfig["AppSetting"]["OpenLogFile"].value<int32>() == 1;
}

uint8 CShmApp::GetDumpLevel() {
    return *m_AppConfig["AppSetting"]["DumpLevel"].value<uint8>();
}

void CShmApp::OnStarted() {
    m_pConnMgr->StartService("127.0.0.1", 1234, GET_CLASS_ID(ConnFromClient), eConnType_TCP_Raw);
}

void CShmApp::OnCheckQuit() {
    delete m_pCharDataMgr;
    m_pCharDataMgr = nullptr;
    CBaseApp::OnCheckQuit();
}