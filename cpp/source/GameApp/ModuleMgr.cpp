#include "GameApp/ModuleMgr.h"
#include "Interface/IGameApp.h"
#include "GammaCommon/CFileUtil.h"
#include "GammaCommon/CThread.h"
#include "GammaCommon/ILog.h"
#include "toml++/toml.hpp"
#include <format>


typedef void (*pInitDllServant)(IGameApp*);
typedef void (*pDllGetConsummer)(IConsummerPtr &);

CDllInfo::CDllInfo()
    : m_pFile(nullptr),
      m_pConsummer(nullptr),
      m_nModuleId(0),
      m_nModuleVersion(0),
      m_strModuleName() {
}

CDllInfo::~CDllInfo() {
    m_pConsummer = nullptr;
    if (m_pFile != nullptr) {
        delete m_pFile;
    }
    m_pFile = nullptr;
}

CModuleMgr::CModuleMgr() : m_Tick(this, &CModuleMgr::Execute) {
    m_mapDll.clear();
    m_strSession.clear();
    m_nProfileModifiedTime = 0;
    m_strProfile.clear();
    m_nStartTime = Gamma::GetNatureTime();
}

CModuleMgr::~CModuleMgr() {
    m_mapDll.clear();
}

bool CModuleMgr::LoadDll(const char *strSettings, const std::string &strTomlFile) {
    auto pGameApp = GetGameApp();
    if (pGameApp == nullptr) {
        return false;
    }
    m_strBinPath = pGameApp->GetBinPath();
    if (m_strBinPath.empty()) {
        Log::Error("CModuleMgr::LoadDll bin path is empty");
        return false;
    }
    if (m_strBinPath.back() != '/') {
        m_strBinPath.push_back('/');
    }
    pGameApp->Register(&m_Tick, FRAME_INTERVAL, INVALID_16BITID);
    m_strSession = strSettings;
    m_strTomlFile = strTomlFile;

    toml::table file_table = toml::parse_file(strTomlFile);
    if (!file_table.contains(strSettings)) {
        Log::Error("CModuleMgr::LoadDll toml section not exist {}", strSettings);
        return false;
    }

    std::vector<CDllInfoPtr> listDllInfo;
    toml::table *settings = file_table[strSettings].as_table();
    if (settings == nullptr) {
        Log::Error("CModuleMgr::LoadDll toml section {} is not a table", strSettings);
        return false;
    }
    for (auto &[key, value] : *settings) {
        auto *value_array = value.as_array();
        if (value_array == nullptr) {
            Log::Error("CModuleMgr::LoadDll module {} is not an array in section {}", key.str(),
                       strSettings);
            return false;
        }
        int nModuleId = static_cast<int>((*value_array)[0].value_or(int64_t{0}));
        int nModuleVer = static_cast<int>((*value_array)[1].value_or(int64_t{0}));
        if (nModuleVer == 0) {
            continue;
        }
        CDllInfoPtr pDllInfo = new CDllInfo();
        pDllInfo->m_nModuleId = nModuleId;
        pDllInfo->m_nModuleVersion = nModuleVer;
        pDllInfo->m_strModuleName = key.str();
        listDllInfo.push_back(pDllInfo);
    }

    std::string strModuleName;
    try {
        for (auto &pDllInfo : listDllInfo) {
            strModuleName = pDllInfo->m_strModuleName;
            if (!LoadDll(pDllInfo)) {
                ThrowException<CCException>("load dll[%s] failed",
                                            pDllInfo->m_strModuleName.c_str());
            }
        }
        for (auto &pDllInfo : listDllInfo) {
            pDllInfo->m_pConsummer->OnInit();
        }
    } catch (CCException & /*e*/) {
        // log_warning("CModuleMgr::loadDll catch ccexception[%s][%s]", e.what(),
        // e.printStackTrace());
        ThrowException<CCException>("load dll[%s] failed", strModuleName.c_str());
    } catch (std::exception & /*e*/) {
        // log_warning("CModuleMgr::loadDll catch exception[%s]", e.what());
        ThrowException<CCException>("load dll[%s] failed", strModuleName.c_str());
    } catch (...) {
        // log_warning("CModuleMgr::loadDll catch ...");
        ThrowException<CCException>("load dll[%s] failed", strModuleName.c_str());
    }

    return true;
}

bool CModuleMgr::StopServer() {
    std::map<uint32, CDllInfoPtr>::iterator iter = m_mapDll.begin();
    DEADLOOP_BREAK_BEGIN(t1);
    IConsummerPtr pMainConsummer;

    for (; iter != m_mapDll.end(); iter++) {
        DEADLOOP_BREAK(t1, 10000);
        if (IS_MAIN_DLL(iter->first)) {
            pMainConsummer = iter->second->m_pConsummer;
            continue;
        }

        if (iter->second->m_pConsummer != NULL && !iter->second->m_pConsummer->OnServerStop()) {
            return false;
        }
    }

    return pMainConsummer == NULL || pMainConsummer->OnServerStop();
}

void CModuleMgr::OnProfileUpdate(const char *strFile) {
    std::map<uint32, CDllInfoPtr>::iterator iterdll = m_mapDll.begin();
    DEADLOOP_BREAK_BEGIN(t2);
    for (; iterdll != m_mapDll.end(); ++iterdll) {
        DEADLOOP_BREAK(t2, 10000);
        if (iterdll->second != NULL) {
            if (iterdll->second->m_pConsummer != NULL) {
                iterdll->second->m_pConsummer->OnProfileUpdate(strFile);
            }
        }
    }
}

bool CModuleMgr::LoadDll(CDllInfoPtr &pDllInfo) {
    std::string strDllName = GetDynamicFileName(pDllInfo->m_strModuleName);
    CDynamicFile *pDynamicFile = new CDynamicFile();
    if (!pDynamicFile->open(strDllName.c_str())) {
        // log_error("open dyanmic file[%s] error [%s]", strdestname.c_str(), pdll->getError());
        ThrowException<CCException>("open dyanmic file[%s] error [%s]", strDllName.c_str(),
                                    pDynamicFile->getError());
        return false;
    }
    // g_Log->info("load dyanmic file[%s] success [%X]", strdestname.c_str(), &pdll);
    pInitDllServant pInitServant = (pInitDllServant)pDynamicFile->getFuncAddr("InitDllServant");
    if (pInitServant == NULL) {
        ThrowException<CCException>("can't found function registServant in dll[%s]",
                                    pDynamicFile->getError());
        return false;
    }
    pDllGetConsummer pGetConsummer = (pDllGetConsummer)pDynamicFile->getFuncAddr("GetConsummer");
    if (pGetConsummer == NULL) {
        ThrowException<CCException>("can't found function getConsummer in dll[%s]",
                                    pDynamicFile->getError());
        return false;
    }

    pGetConsummer(pDllInfo->m_pConsummer);
    pInitServant(GetGameApp().get());
    pDllInfo->m_pFile = pDynamicFile;
    if (pDllInfo->m_pConsummer->GetModuleID() != pDllInfo->m_nModuleId) {
        ThrowException<CCException>("module id not match predefined [%s]", strDllName.c_str());
        return false;
    }
    m_mapDll[pDllInfo->m_nModuleId] = pDllInfo;

    return true;
}

void CModuleMgr::UnLoadDll(uint32 nMoudle) {
    auto iterOld = m_mapDll.find(nMoudle);
    if (iterOld == m_mapDll.end()) {
        return;
    }
    if (IS_MAIN_DLL(nMoudle)) {
        return;
    }
    if (iterOld->second->m_pConsummer != NULL) {
        iterOld->second->m_pConsummer->OnServerStop();
    }
    m_mapDll.erase(nMoudle);
}

void CModuleMgr::Execute(void) {
    m_nFrame++;
    for (auto &[nModuleId, pDllInfo] : m_mapDll) {
        if (pDllInfo != nullptr && pDllInfo->m_pConsummer != nullptr) {
            pDllInfo->m_pConsummer->OnTimer(FRAME_INTERVAL);
        }
    }
    return;
}

IConsummerPtr CModuleMgr::GetConsummer(uint32 nModuleID) {
    auto it = m_mapDll.find(nModuleID);
    if (it != m_mapDll.end()) {
        return it->second->m_pConsummer;
    }
    return nullptr;
}

// 服务器连接成功
void CModuleMgr::OnServerConnect(uint16 nServerID) {
    if (nServerID == 0)
        return;
    for (auto it : m_mapDll) {
        if (it.second->m_pConsummer != nullptr) {
            it.second->m_pConsummer->OnServerConnect(nServerID);
        }
    }
}

// 服务器断开连接
void CModuleMgr::OnServerDisConnect(uint16 nServerID) {
    if (nServerID == 0)
        return;
    for (auto it : m_mapDll) {
        if (it.second->m_pConsummer != nullptr) {
            it.second->m_pConsummer->OnServerDisConnect(nServerID);
        }
    }
}

void CModuleMgr::OnModuleConfigChanged() {
    toml::table file_table = toml::parse_file(m_strTomlFile);
    if (!file_table.contains(m_strSession)) {
        Log::Error("CModuleMgr::OnModuleConfigChanged toml file not exist {}", m_strSession);
        return;
    }
    toml::table &settings = *file_table[m_strSession].as_table();
    std::map<uint32, CDllInfoPtr> newDllMap;
    for (auto &[key, value] : settings) {
        auto value_array = value.as_array();
        int nModuleId = value_array[0].as_integer()->get();
        int nModuleVer = value_array[1].as_integer()->get();
        auto iterDllInfo = m_mapDll.find(nModuleId);

        // 已经加载，如果版本变化需要重新加载
        if (iterDllInfo != m_mapDll.end()) {
            auto pOldDllInfo = iterDllInfo->second;
            if (nModuleVer == pOldDllInfo->m_nModuleVersion) {
                // 版本未变化，跳过
                continue;
            }
            // 版本变化，要卸载旧的
            UnLoadDll(nModuleId);
            if (IS_MAIN_DLL(nModuleId)) {
                Log::Error("CModuleMgr::OnModuleConfigChanged main module {} cannot be unloaded",
                           key.str());
                continue;
            } else {
                Log::Info("CModuleMgr::OnModuleConfigChanged module {} unloaded", key.str());
            }
        }

        if (nModuleVer != 0) {
            auto &pDllInfo = iterDllInfo->second;
            pDllInfo->m_nModuleVersion = nModuleVer;
            pDllInfo->m_strModuleName = key.str();
            try {
                if (!LoadDll(pDllInfo)) {
                    Log::Error("CModuleMgr::OnModuleConfigChanged load dll {} failed", key.str());
                    continue;
                }
            } catch (CCException & /*e*/) {
                Log::Error("CModuleMgr::OnModuleConfigChanged load dll {} catch ccexception",
                           key.str());
                continue;
            } catch (std::exception & /*e*/) {
                Log::Error("CModuleMgr::OnModuleConfigChanged load dll {} catch exception",
                           key.str());
                continue;
            } catch (...) {
                Log::Error("CModuleMgr::OnModuleConfigChanged load dll {} catch ...", key.str());
                continue;
            }
        }
    }
}

std::string CModuleMgr::GetDynamicFileName(const std::string &strName) {
#ifdef _WIN32
    return std::format("{}/{}.dll", m_strBinPath, strName);
#else
    return std::format("{}/lib{}.so", m_strBinPath, strName);
#endif // _WIN32
}
