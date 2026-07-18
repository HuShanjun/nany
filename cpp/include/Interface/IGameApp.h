// 游戏服务器的接口，主要有以下作用
// 1. 暴露接口给业务层使用
// 2. 定义派生类需要实现的接口

#pragma once
#include "GammaCommon/CRefshare.h"
#include "GammaCommon/GammaHelp.h"
#include "GammaCommon/GammaTime.h"
#include "IComp.h"
#include <cstdint>
#include <functional>
#include <string>

#define FRAME_INTERVAL 50
#define FLOAT_CHECK_ZERO 0.000001f
#define MICROSECOND_PER_SECOND 1000

namespace Gamma {
class CScriptLua;
class IConnectionMgr;
} // namespace Gamma

class CCurrent;
class IGameApp;
class CDBResultHandler;

typedef CRefObject<IGameApp> IGameAppPtr;
typedef std::function<uint32(const void *, uint32, CCurrent *)> fpMsg;
using namespace Gamma;

class IGameApp : public CRefShare {
public:
    virtual ~IGameApp() {};

    virtual void OnReady() = 0;
    /* CBaseApp 实现的接口, 外部业务需要访问 */
    virtual int64 GetCurTime() const = 0;
    virtual uint32 GetCurFrame() const = 0;
    virtual uint32 GetBaseCyc() const = 0;
    virtual void Register(CTick *pTick, uint32 uCyc, uint16 nTickID) = 0;
    virtual void Register(CTick *pTick, uint32 nStart, uint32 uCyc, uint16 nTickID) = 0;
    virtual void UnRegister(CTick *pTick) = 0;
    virtual int64 GetCurTickTime() const = 0;
    virtual IConnectionMgr *GetConnMgr() const = 0;
    virtual uint32 GetServerID() const = 0;
    virtual const std::string& GetServerType() const = 0;
    virtual const std::string& GetServerName() const = 0;
    virtual const char* GetBinPath() = 0;
    /* CBaseApp 实现的接口, 外部业务需要访问 */
};

IGameAppPtr GetGameApp();

template <typename T>
CRefObject<T> GetGameApp() {
    return dynamic_cast<T *>(GetGameApp().get());
}
