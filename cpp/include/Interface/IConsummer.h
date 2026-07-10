#pragma once

#include "GammaCommon/GammaCommonType.h"
#include "GammaCommon/CRefshare.h"

// 每个进程主dll的低三个十进制位为0
#define IS_MAIN_DLL(T) ((((T) % 10000) == 0) ? true : false)
#define DEFINE_MODULE(ID)                                                                          \
    enum { eModuleID = ID };                                                                       \
    virtual uint32 GetModuleID() {                                                                 \
        return ID;                                                                                 \
    }

class IConsummer : public CRefShare {
protected:
    friend class CRefObject<IConsummer>;

public:
    virtual ~IConsummer() {};

    virtual uint32 GetModuleID() = 0;

    // 初始化
    virtual void OnInit() = 0;

    // 关闭
    virtual void UnInit() = 0;

    // 定时器触发
    virtual int OnTimer(uint32 nInterval) = 0;

    // 服务器连接成功
    virtual void OnServerConnect(uint16 nServerID) {}

    // 服务器断开连接
    virtual void OnServerDisConnect(uint16 nServerID) {}

    // 服务器关闭，如果暂时不允许关闭返回false
    virtual bool OnServerStop() { return true; }

    // 静态数据更新
    virtual void OnProfileUpdate(const char *strFile) {}

    // 获取当前进程名枚举（EPROCESS_NAME）
    virtual uint8 GetProcessName() { return 0;}
};

typedef CRefObject<IConsummer> IConsummerPtr;
