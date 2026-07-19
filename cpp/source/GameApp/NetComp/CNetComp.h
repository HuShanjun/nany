#pragma once

#include "GameApp/GameAppHelp.h"
#include "CGameConnFromClient.h"
#include "GammaCommon/GammaTime.h"
#include "Interface/INetComp.h"
#include "GammaCommon/TGammaRBTree.h"
#include "toml++/toml.hpp"
#include "GammaNetwork/CAddress.h"

#include <map>
#include <set>
#include <cstdint>
#include <string>


using namespace Gamma;

namespace IDAllocator {
    uint32_t Alloc();
    void Free(uint32_t nID);
    void Reset();
}

struct SServerInfo {
    std::string strType;
    std::string strName;
    uint32_t nServerID;
    uint32_t nTypeID;
    std::string strLocalHost;
    uint16_t nLocalPort;
    std::string strPublicHost;
    uint16_t nPublicPort;
};

class CGameConnServer;

typedef Gamma::TGammaRBTree<CGameConnServer> CGameConnServerTree;
typedef Gamma::TGammaRBTree<CClientConnectNode> CGameConnFromClientTree;
typedef CGameConnServerTree::CGammaRBTreeNode CGameConnServerNode;
typedef CGameConnFromClientTree::CGammaRBTreeNode CGameConnFromClientNode;

using CServerInfoMap = std::map<uint32, SServerInfo>;
using CServerTypeMap = std::map<uint32, CServerInfoMap>;

enum EServerType {
    Center = 0,
    Login,
    Game,
    Gate,
    DBMain,
    Count,
};

std::string GetServerTypeName(EServerType eServerType);
EServerType GetServerType(const std::string& strServerType);


class CNetComp : public INetComp
{
	typedef TTickFun<CNetComp> CNetCompTick;
    using CConnectionMap = std::map<uint32_t, CGameConnection*>;

public:
    DEFINE_COMP_ID(NetComp, EGameAppCompID::NetComp);
    CNetComp(toml::table serverConfig);
    ~CNetComp();

    virtual void OnInit() override;
    virtual void OnCleanup() override {}
    virtual void OnStarted() override;
    virtual void OnQuit() override;

    void SendMsgToClient(uint32_t nClientID, void* pData, size_t nSize) override {}
    void SendMsgToCenter(void* pData, size_t nSize) override {}
    void SendMsgToServer(uint32_t nServerID, void* pData, size_t nSize) override {}
    void SendMsgToLogin(void* pData, size_t nSize) override {}

    void AddServerConnect(CGameConnServer* pServerConn);
    void DelServerConnect(CGameConnServer* pServerConn);
    void AddClientConnect(CGameConnFromClient* pFromClientConn);
    void DelClientConnect(CGameConnFromClient* pFromClientConn);

private:
    void OnReconnectTick();

    void ParserNetTopology();
    SServerInfo *GetServerInfo(uint32 nServerID);

private:
    std::set<EServerType> m_vecNetTopology[EServerType::Count];
    CServerTypeMap m_mapServerType;
    CNetCompTick m_ReconnectTick;
    std::map<uint16, CAddress> m_mapServers;
    std::map<uint32, CGameConnServer*> m_mapServerConn;
    CGameConnServerTree m_treeServer;
    CGameConnFromClientTree m_treeFromClient;
    const toml::table m_ServerConfig;
    SServerInfo* m_pCurServerInfo;
};