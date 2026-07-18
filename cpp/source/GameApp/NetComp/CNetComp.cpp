#include "CNetComp.h"
#include "CGameConnServer.h"
#include "GammaApp/CBaseApp.h"
#include "Interface/IComp.h"
#include "Interface/IGameApp.h"
#include "GammaCommon/CThread.h"
#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/ILog.h"
#include "GammaNetwork/CAddress.h"
#include "Interface/INetComp.h"
#include "GameApp/GameApp.h"
#include "toml++/toml.hpp"
#include <array>
#include <string>
#include <sys/types.h>

namespace IDAllocator {
static uint32_t s_nID = 0;
static std::vector<uint32_t> s_vecFreeIDs;

uint32_t Alloc() {
    if (s_vecFreeIDs.empty()) {
        return s_nID++;
    }
    uint32_t nID = s_vecFreeIDs.back();
    s_vecFreeIDs.pop_back();
    return nID;
}

void Free(uint32_t nID) {
    s_vecFreeIDs.push_back(nID);
}

void Reset() {
    s_nID = 0;
    s_vecFreeIDs.clear();
}
}; // namespace IDAllocator

const std::array<std::string, EServerType::Count> arrServerTypeName = {
    "Center", "Login", "Game", "Gate", "DBMain",
};

template <>
struct std::formatter<SServerInfo> {
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

    auto format(const SServerInfo &serverInfo, std::format_context &ctx) const {
        return std::format_to(
            ctx.out(),
            "{{ServerName={}, ServerType={}, ServerID={}, TypeID={}, LocalHost={}, LocalPort={}, PublicHost={}, PublicPort={}}}",
            serverInfo.strName, serverInfo.strType, serverInfo.nServerID, serverInfo.nTypeID,
            serverInfo.strLocalHost, serverInfo.nLocalPort, serverInfo.strPublicHost,
            serverInfo.nPublicPort);
    }
};

std::string GetServerTypeName(EServerType eServerType) {
    return arrServerTypeName[eServerType];
}

EServerType GetServerType(const std::string &strServerType) {
    for (size_t i = 0; i < arrServerTypeName.size(); i++) {
        if (arrServerTypeName[i] == strServerType) {
            return static_cast<EServerType>(i);
        }
    }
    return EServerType::Count;
}

CNetComp::CNetComp(toml::table serverConfig)
    : m_ServerConfig(serverConfig)
    , m_ReconnectTick(this, &CNetComp::OnReconnectTick) {
}

CNetComp::~CNetComp() {
}

void CNetComp::OnInit() {
    ParserNetTopology();
    auto pGameApp = GetGameApp<CGameApp>();
    const std::string &strCurServerType = pGameApp->GetServerType();
    uint32 nCurServerID = pGameApp->GetServerID();
    uint8 nCurServerType = nCurServerID / 100;
    const auto &setNetTopology = m_vecNetTopology[nCurServerType];

    for (auto &[key, value] : m_ServerConfig) {
        auto strServerName = key.str();
        toml::table pServerInfo = *value.as_table();
        uint32 nServerID = pServerInfo.at("ServerID").value_or(0);
        uint32 nServerType = nServerID / 100;
        std::string strServerType = pServerInfo.at("type").value_or("");
        std::string strLocalHost = pServerInfo.at("LocalHost").value_or("");
        uint16 nLocalPort = pServerInfo.at("LocalPort").value_or(0);
        std::string strPublicHost =
            pServerInfo.contains("PublicHost") ? pServerInfo.at("PublicHost").value_or("") : "";
        uint16 nPublicPort =
            pServerInfo.contains("PublicPort") ? pServerInfo.at("PublicPort").value_or(0) : 0;

        SServerInfo &serverInfo = m_mapServerType[nServerType][nServerID];
        serverInfo.strName = strServerName;
        serverInfo.strType = strServerType;
        serverInfo.nServerID = nServerID;
        serverInfo.nTypeID = nServerType;
        serverInfo.strLocalHost = strLocalHost;
        serverInfo.nLocalPort = nLocalPort;
        serverInfo.strPublicHost = strPublicHost;
        serverInfo.nPublicPort = nPublicPort;

        if (nCurServerID == nServerID) {
            m_pCurServerInfo = &serverInfo;
        }

        Log::Info("CNetComp::OnInit ServerInfo = {}", serverInfo);
        if (setNetTopology.contains((EServerType)nServerType)) {
            if (nServerType != nCurServerType
                || nServerType == nCurServerType && nServerID > nCurServerID) {
                CAddress &addr = m_mapServers[nServerID];
                addr.SetAddress(strLocalHost.c_str());
                addr.SetPort(nLocalPort);
            }
        }
    }
}

void CNetComp::OnStarted() {
    auto pConnMgr = GetGameApp()->GetConnMgr();
    if (m_pCurServerInfo && m_pCurServerInfo->nPublicPort > 0) {
        pConnMgr->StartService(m_pCurServerInfo->strPublicHost.c_str(),
                               m_pCurServerInfo->nPublicPort, CGameConnServer::GetID());
        Log::Info("CNetComp::OnStarted StartService {}:{}", m_pCurServerInfo->strPublicHost,
                  m_pCurServerInfo->nPublicPort);
    }
    GetGameApp()->Register(&m_ReconnectTick, 1000, 0);
}

void CNetComp::OnQuit() {
    GetGameApp()->UnRegister(&m_ReconnectTick);
}

void CNetComp::OnReconnectTick() {
    auto pConnMgr = GetGameApp()->GetConnMgr();
    for (auto &[nServerID, addr] : m_mapServers) {
        auto pServerConn = m_treeServer.Find(nServerID);
        if (pServerConn) {
            continue;
        }
        const char *szAddress = addr.GetAddress();
        uint nPort = addr.GetPort();
        CBaseConn *pBase =
            pConnMgr->Connect(addr.GetAddress(), addr.GetPort(), CGameConnServer::GetID());
        if (!pBase) {
            Log::Error("CNetComp::OnReconnectTick {}:{}", szAddress, nPort);
            continue;
        }
        pServerConn = static_cast<CGameConnServer *>(pBase);
        pServerConn->SetServerID(nServerID);
    }
}

void CNetComp::ParserNetTopology() {
    auto pGameApp = GetGameApp<CGameApp>();
    toml::table tblNetTopology = pGameApp->GetAppSettingTable("NetTopology");
    for (int i = 0; i < EServerType::Count; i++) {
        std::string strServerTypeName = GetServerTypeName((EServerType)i);
        auto &setNetTopology = m_vecNetTopology[i];
        if (!tblNetTopology.contains(strServerTypeName)) {
            continue;
        }
        auto &tblServerTypes = *tblNetTopology.at(strServerTypeName).as_array();
        for (auto &serverType : tblServerTypes) {
            std::string strServerType = serverType.value_or("");
            if (strServerType.empty()) {
                continue;
            }
            auto eServerType = GetServerType(strServerType);
            if (eServerType != EServerType::Count) {
                setNetTopology.insert(eServerType);
            }
        }
    }
}

SServerInfo *CNetComp::GetServerInfo(uint32 nServerID) {
    uint32 nTypeID = nServerID / 100;
    auto it = m_mapServerType.find(nTypeID);
    if (it == m_mapServerType.end()) {
        return nullptr;
    }
    auto it2 = it->second.find(nServerID);
    if (it2 == it->second.end()) {
        return nullptr;
    }
    return &it2->second;
}
