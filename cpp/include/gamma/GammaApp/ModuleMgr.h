#pragma once
#include "GammaCommon/CDynamicFile.h"
#include "GammaCommon/CIniFile.h"
#include "GammaCommon/CTinyTimer.h"
#include "Interface/IConsummer.h"
#include "GammaCommon/TSingleton.h"
#include "GammaApp/IGameApp.h"
#include <string>
using namespace Gamma;

// dll的基本信息类
class CDllInfo : public CRefShare
{
public:
    CDynamicFile *m_pFile;       // dll文件指针
    IConsummerPtr m_pConsummer;  // dll提供的接口
    int32 m_nModuleId;           // 模块ID
    int32 m_nModuleVersion;      // 模块版本
    std::string m_strModuleName; // 模块名称

    CDllInfo();
    ~CDllInfo();
};

typedef CRefObject<CDllInfo> CDllInfoPtr;

// dll管理器
class CModuleMgr : public CSingleton<CModuleMgr>
{
public:
    CModuleMgr();
    virtual ~CModuleMgr();

    bool LoadDll(const char *strSettings, const std::string &strTomlFile);

    bool LoadDll(CDllInfoPtr &pDllInfo);

    void UnLoadDll(uint32 nMoudle);

    IConsummerPtr GetConsummer(uint32 nModuleID);

    virtual void OnProfileUpdate(const char *strFile);

    void Execute(void);

    bool StopServer();

    void OnServerConnect(uint16 nServerID);

    void OnServerDisConnect(uint16 nServerID);

    int64 GetCurFrame() const { return m_nFrame; }

    int64 GetServerTime() const { return m_nStartTime + m_nFrame * FRAME_INTERVAL; }

    void ResponseHttpMsg(int nSerialID, const char *pMsg);

    void OnModuleConfigChanged();

private:
    std::string GetDynamicFileName(const std::string &strName);

private:
    // 当前加载的dll
    std::map<uint32, CDllInfoPtr> m_mapDll;

    std::string m_strSession;      // 配置文件节点
    std::string m_strProfile;      // 配置文件路径
    CIniFile m_Profile;            // dll配置
    time_t m_nProfileModifiedTime; // 配置文件上次修改时间
    CTinyTimer m_tCheckProfile;    // 检查配置文件周期

    // 路径配置
    std::string m_strBinPath;
    std::string m_strEtcPath;
    std::string m_strTomlFile;

    Gamma::TTickFun<CModuleMgr> m_Tick;
    int64 m_nFrame = 0;
    int64 m_nStartTime = 0;
};