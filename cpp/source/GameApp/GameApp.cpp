#include "GameApp/GameApp.h"
#include "pystring/pystring.h"

#include <iostream>

CGameApp::CGameApp()
{
}

CGameApp::~CGameApp()
{
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
        std::string strName = m_result["name"].as<std::string>();
        if (strName.empty()) {
            throw std::runtime_error("name is empty");
            return;
        }

        auto vecNames = pystring::split(strName, "_");
        m_strClassName = vecNames[0];
        m_nServerID = std::stoi(vecNames[1]);

    } catch (const cxxopts::OptionException &e) {
        std::cerr << "Error parsing options: " << e.what() << std::endl;
        std::exit(1);
    }
}

namespace fs = std::filesystem;
void CGameApp::InitPathConfig() {
    if (m_strDevopsPath.empty()) {
        throw std::runtime_error("devops path is empty");
        return;
    }
    if (!fs::exists(m_strDevopsPath)) {
        throw std::runtime_error("devops path " + m_strDevopsPath + " not exists");
        return;
    }
    m_strWorkPath = m_strDevopsPath + "/work";
    m_strDataPath = m_strDevopsPath + "/data";
    m_strEtcPath = m_strDevopsPath + "/etc";
}
void CGameApp::LoadConfig() {
}

uint32 CGameApp::GetLogLevel() {
    return 0;
}
uint8 CGameApp::GetDumpLevel() {
    return 0;
}
bool CGameApp::IsShowConsole() {
    return false;
}
bool CGameApp::IsOpenLogFile() {
    return false;
}