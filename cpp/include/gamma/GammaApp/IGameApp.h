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

    /* 具体实现类需要实现的接口 */
    virtual void OnReady() = 0;
    virtual void InitCmdLine(int argc, const char **argv) = 0;
    virtual void LoadConfig() = 0;
    virtual void InitPathConfig() = 0;
    virtual uint32 GetLogLevel() = 0;
    virtual uint8 GetDumpLevel() = 0;
    virtual bool IsShowConsole() = 0;
    virtual bool IsOpenLogFile() = 0;
    /* 具体实现类需要实现的接口 */

    /* CBaseApp 实现的接口, 外部业务需要访问 */
    virtual int64 GetCurTime() const = 0;
    virtual uint32 GetCurFrame() const = 0;
    virtual uint32 GetBaseCyc() const = 0;
    virtual void Register(CTick *pTick, uint32 uCyc, uint16 nTickID) = 0;
    virtual void Register(CTick *pTick, uint32 nStart, uint32 uCyc, uint16 nTickID) = 0;
    virtual void UnRegister(CTick *pTick) = 0;
    virtual int64 GetCurTickTime() const = 0;
    virtual IConnectionMgr *GetConnMgr() const = 0;

    /* 获取组件 */
    virtual IComp *GetComp(uint8_t nCompID) = 0;
    virtual IComp *AddComp(uint8_t nCompID, IComp *pComp) = 0;
    virtual void DelComp(uint8_t nCompID) = 0;

    template <typename T>
    T *GetComp();

    template <typename T, typename... Args>
    T *AddComp(Args &&...args);

    template <typename T>
    void DelComp();

    /* CBaseApp 实现的接口, 外部业务需要访问 */
};

template <typename T>
inline void IGameApp::DelComp() {
    DelComp(T::GetID());
}

template <typename T, typename... Args>
inline T *IGameApp::AddComp(Args &&...args) {
    T *pComp = new T(std::forward<Args>(args)...);
    AddComp(T::GetID(), pComp);
    return static_cast<T *>(pComp);
}

template <typename T>
inline T *IGameApp::GetComp() {
    return dynamic_cast<T *>(GetComp(T::GetID()));
}

IGameAppPtr GetGameApp();

template <typename T>
CRefObject<T> GetGameApp() {
    return dynamic_cast<T *>(GetGameApp().get());
}

template <typename T>
T* GetComp(){
    return GetGameApp()->GetComp<T>();
}