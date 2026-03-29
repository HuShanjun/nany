# GammaConnects 模块文档

## 概述

GammaConnects 是 **nany-cpp** 中的**连接与会话层**模块：在 **GammaNetwork**（套接字、监听、域名解析等）之上，提供连接管理、协议分派（Dispatch）、Shell 层消息收发、心跳与 KCP/WebSocket 等能力。游戏或业务进程通过 `IConnectionMgr` 统一启动监听、发起连接，并派生 `CBaseConn` 处理业务消息。

## 模块定位与依赖

| 项目 | 说明 |
|------|------|
| 依赖 | **GammaCommon**（对象系统、时间、容器等）、**GammaNetwork**（`INetwork`、`IConnecter`、地址等） |
| 构建 | Windows 下为**静态库**；Linux 下为**动态库**（`GAMMA_DLL`），并链接 `GammaCommon`、`GammaNetwork` |
| 命名空间 | `Gamma` |
| 导出宏 | `GAMMA_CONNECTS_API`（Windows DLL 导入/导出） |

头文件路径（相对工程 `include`）：`GammaConnects/*.h`（源码树中亦存在 `include/gamma/GammaConnects` 镜像，以 CMake `include_directories` 为准）。

## 架构分层

```
应用层（派生 CBaseConn，实现 OnShellMsg / OnConnected / OnDisConnect）
        │
        ▼
CConnectionMgr（IConnectionMgr）── CreateConnMgr()
        │  Check(nWaitTimes) 驱动网络与连接更新
        ├── 监听：StartService → CListenHandler → OnAccept → 按 EConnType 创建 CConnection / CPrtConnection / CWebConnection
        └── 主动连接：Connect → 同类连接对象
        │
        ▼
CConnection / CPrtConnection / CWebConnection（收包、分派、心跳、可选 KCP）
        │
        ▼
GammaNetwork（INetwork::Check、套接字发送等）
```

- **驱动模型**：主循环或定时器需周期性调用 `IConnectionMgr::Check(uint32_t nWaitTimes)`，内部会调用 `INetwork::Check`，并处理连接超时、更新队列（KCP、延迟发送等）。
- **连接与 Handler**：每个逻辑连接对应一个 `CBaseConn` 子类实例；通过 **动态类 ID**（`CDynamicObject` / `DECLARE_DYNAMIC_CLASS`）与 `nConnectClassID` 绑定，便于同一进程内多种连接类型共存。

## 核心概念

### 连接类型 `EConnType`（`ConnectDef.h`）

| 枚举 | 含义 |
|------|------|
| `eConnType_UDP_Raw` | UDP 原始，无可靠性保证 |
| `eConnType_UDP_Prt` | UDP + KCP 协议封装（可靠） |
| `eConnType_TCP_Raw` | TCP 原始，无协议封装 |
| `eConnType_TCP_Prt` | TCP + Shell 协议（最常用） |
| `eConnType_TCP_Web` | WebSocket（明文） |
| `eConnType_TCP_Webs` | WebSocket + TLS |
| `eConnType_TCP_Prts` | TCP + 协议 + TLS |

监听与连接时需为同一业务选用一致的 `EConnType`。TLS 监听可在 `StartService` 中传入证书与私钥路径。

### 连接生命周期与状态

```
Connecting ──► Connected ──► Disconnecting ──► Disconnected
```

**关闭类型 `ECloseType`：**

| 枚举 | 含义 |
|------|------|
| `eCT_None` | 未关闭 |
| `eCT_Grace` | 优雅关闭（`ShellCmdClose`） |
| `eCT_Force` | 强制关闭（`ForceClose`） |
| `eCE_Unknown` | 异常/未知关闭 |

**CConnectionMgr 生命周期管理：**
- 每秒调用 `OnCheckConnecting()`，检查超时与心跳
- 已断开连接在 5 秒后从列表移除
- `nAutoDisconnectTime`：握手未完成连接的超时时间

### Shell 层消息

在 **Prt（协议）** 模式下，TCP 载荷之上有一层 **Shell** 分包与消息投递：

- 底层连接对象将解析后的**业务载荷**交给用户派生的 `CBaseConn::OnShellMsg(const void* pCmd, size_t nSize, bool bUnreliable)`。
- 发送使用 `SendShellMsg`（单缓冲或多段 `SSendBuf`），可区分可靠/不可靠（UDP/KCP 语义下有意义）。

框架内置 **GammaPrt**（`source/gamma/GammaConnects/GammaPrt.h`）定义了 Shell 层命令 ID：

| 命令 ID | 含义 |
|---------|------|
| `CGC_ShellMsg8`（ID=0） | Shell 消息，载荷 ≤ 65535 字节 |
| `CGC_ShellMsg32`（ID=253） | Shell 消息，载荷 > 65535 字节 |
| `CGC_HeartbeatMsg`（ID=254） | 心跳请求 |
| `CGC_HeartbeatReply`（ID=255） | 心跳回复 |

业务协议通常在 Shell 载荷内再定义自己的消息头（如示例中的 `SMsgHeader` + 枚举 `ESampleMsg`）。

### 协议分派 `TDispatch`

`CPrtConnection`、`CWebConnection` 等使用 **模板类 `TDispatch<...>`**（`TDispatch.h` + `TDispatch.inl`）：

```cpp
TDispatch<DispatchClass, IdType_t, SubClass, MsgBaseType>
```

- 消息基类为 **`TBasePrtlMsg<IdType>`**（`TBasePrtlMsg.h`）：首部含消息 ID，可重载 `GetExtraSize` 支持变长包。
- 通过 **`RegistProcessFun`** 静态注册「校验函数 + 成员处理函数」，按 ID 解析并回调。
- 返回值：`eCR_Again`（数据不足）、`eCR_Error`（解析错误）或已消耗字节数。
- 解析失败触发日志/断言，并强制关闭连接。

应用侧若仅在 `OnShellMsg` 里自行 `switch` 解析，可不直接使用 `TDispatch`；框架层用其处理内置 Prt/WebSocket 帧。

### KCP 配置 `SKcpConfig`（`ConnectDef.h`）

| 字段 | 默认值 | 说明 |
|------|--------|------|
| `KCPCONFIG_CONV` | `0xd14d4926` | KCP 会话 ID |
| `KCPCONFIG_UPDATE` | 33 ms | KCP Update 调用间隔 |
| `KCPCONFIG_TICK` | 10 ms | KCP Tick |
| `KCPCONFIG_RESEND` | 2 | 快速重传阈值（0=禁用） |
| `KCPCONFIG_SENDWND` | 128 | 发送窗口大小 |
| `KCPCONFIG_RECVWND` | 128 | 接收窗口大小 |
| `KCPCONFIG_RTO` | 10 ms | 重传超时 |

管理器提供 `GetKcpConfig()` / `SetKcpConfig()` 读写全局 KCP 配置。

### WebSocket 协议（`CWebConnection`）

帧类型：

| 帧类型 | 说明 |
|--------|------|
| `CWS_Empty` | 空帧 |
| `CWS_Text` | 文本帧 |
| `CWS_Binary` | 二进制帧（业务数据） |
| `CWS_Close` | 关闭帧 |
| `CWS_Ping` / `CWS_Pong` | 心跳帧 |

客户端连接时自动完成 WebSocket 握手（`CheckShakeHand`），支持分片重组。

### 严格模式 `bStrictMode`

`CreateConnMgr(nAutoDisconnectTime, bStrictMode)` 中：

- **严格模式**：若连接从未回调过 `OnConnected`，则不会回调 `OnDisConnect`。用于过滤未成功建立的"伪断开"通知。

## 线程模型

**单线程事件循环：**
- 所有回调（`OnConnected`、`OnDisConnect`、`OnShellMsg`）均在调用 `Check()` 的线程中执行。
- 模块内部无额外线程（GammaNetwork 层可能使用线程处理 I/O）。
- 用户必须从同一线程周期性调用 `Check()`。

## 公共接口摘要

### 工厂函数

```cpp
GAMMA_CONNECTS_API IConnectionMgr* CreateConnMgr(uint32_t nAutoDisconnectTime, bool bStrictMode = false);
```

### `IConnectionMgr`（`IConnectionMgr.h`）

继承自 `IGammaUnknown`，使用完毕后需调用 `Release()`。

| 方法 | 签名 | 说明 |
|------|------|------|
| `GetNetwork` | `INetwork*()` | 获取底层网络层 |
| `PreResolveDomain` | `void(const char*)` | 预解析域名，减少首次连接延迟 |
| `Check` | `void(uint32_t nWaitTimes)` | **必须周期性调用**，驱动所有事件处理 |
| `StartService` | `bool(szAddress, nPort, nConnectClassID, eType, certPath, keyPath)` | 启动监听服务 |
| `StopService` | `void(szAddress, nPort)` | 停止指定监听 |
| `StopAllService` | `void()` | 停止全部监听 |
| `Connect` | `CBaseConn*(szAddress, nPort, nConnectClassID, eType)` | 发起连接 |
| `StopConnect` | `void(pConn)` | 停止指定连接 |
| `StopAllConnect` | `void()` | 停止全部外向连接 |
| `GetAllConn` | `uint32_t(nConnectClassID, aryConn[], nCount)` | 枚举指定类型连接 |
| `GetKcpConfig` | `const SKcpConfig&()` | 读取 KCP 配置 |
| `SetKcpConfig` | `void(const SKcpConfig&)` | 写入 KCP 配置 |

### `CBaseConn`（`CBaseConn.h`）

#### 必须实现的纯虚方法

| 方法 | 说明 |
|------|------|
| `OnConnected()` | 连接建立完成时回调 |
| `OnDisConnect()` | 连接断开时回调（严格模式下仅在 OnConnected 后触发） |
| `OnShellMsg(pCmd, nSize, bUnreliable)` | 收到业务消息，返回已消耗字节数 |

#### 可选重写

| 方法 | 说明 |
|------|------|
| `OnHeartBeatStop()` | 心跳超时时回调 |

#### 连接控制

| 方法 | 说明 |
|------|------|
| `ShellCmdClose(szLogContext)` | 优雅关闭 |
| `ForceClose(szLogContext)` | 强制关闭 |
| `IsConnected()` | 是否已连接 |
| `IsGraceClose()` | 是否优雅关闭中 |
| `IsForceClose()` | 是否强制关闭中 |
| `GetConnection()` | 获取底层 `CConnection*` |
| `BindConnection(pConnection)` | 重新绑定连接对象 |

#### 数据发送

| 方法 | 说明 |
|------|------|
| `SendShellMsg(pData, nSize, bUnreliable)` | 发送单段消息 |
| `SendShellMsg(aryBuffer[], nBufferCount, bUnreliable)` | 发送多段 `SSendBuf` |
| `EnableMsgDispatch(bool)` | 暂停/恢复消息处理 |

#### 连接信息

| 方法 | 说明 |
|------|------|
| `GetRemoteAddress()` | 远端地址（`CAddress`） |
| `GetLocalAddress()` | 本端地址（`CAddress`） |
| `GetPingDelay()` | RTT（毫秒） |

#### 心跳与延迟

| 方法 | 说明 |
|------|------|
| `SetHeartBeatInterval(nSeconds)` | 设置心跳间隔（秒） |
| `SetNetDelay(nMinDelay, nMaxDelay)` | 模拟网络延迟（毫秒范围） |

#### 流量统计

| 方法 | 说明 |
|------|------|
| `EnableProfile(nSendIDBits, nRecvIDBits)` | 启用按消息 ID 的流量统计 |
| `GetSendBufferSize(nShellID)` | 指定消息 ID 的发送字节数 |
| `GetRecvBufferSize(nShellID)` | 指定消息 ID 的接收字节数 |
| `GetTotalSendSize()` | 总发送字节数 |
| `GetTotalRecvSize()` | 总接收字节数 |
| `PrintMsgSize()` | 打印消息流量直方图 |

## 网络延迟模拟

`SetNetDelay(nMinDelay, nMaxDelay)` 启用延迟模拟：
- 发送/接收数据先缓冲到 `m_szSendBuf`/`m_szRecvBuf`，附带时间戳。
- `OnUpdate()` 在时间到期后才实际处理。
- 用于测试高延迟场景下的业务逻辑正确性。

## 错误处理模式

- `OnShellMsg()` 返回 0 表示数据不足（等待更多数据），框架会在下次收包后重试。
- Dispatch 解析错误触发强制关闭并记录日志上下文。
- 回调中的异常会被捕获并记录日志。
- 无效操作通过 `IsValid()` 检查后静默忽略。

## 使用流程

### 1. 定义协议

```cpp
#pragma pack(push, 1)
enum ESampleMsg : uint16_t { eSM_Echo = 1, eSM_EchoReply = 2 };
struct SMsgHeader { ESampleMsg eMsgId; };
struct SMsgEcho : SMsgHeader { char szText[256]; };
#pragma pack(pop)
```

### 2. 派生 CBaseConn

```cpp
class CServerConn : public CBaseConn {
    DECLARE_DYNAMIC_CLASS(CServerConn)  // 注册动态类
public:
    void OnConnected() override { /* 记录连接 */ }
    void OnDisConnect() override { /* 清理状态 */ }
    size_t OnShellMsg(const void* pCmd, size_t nSize, bool bUnreliable) override {
        if (nSize < sizeof(SMsgHeader)) return 0;  // 数据不足
        auto* hdr = (const SMsgHeader*)pCmd;
        switch (hdr->eMsgId) {
            case eSM_Echo: {
                if (nSize < sizeof(SMsgEcho)) return 0;
                auto* msg = (const SMsgEcho*)pCmd;
                SMsgEchoReply reply;
                reply.eMsgId = eSM_EchoReply;
                strncpy(reply.szText, msg->szText, sizeof(reply.szText));
                SendShellMsg(&reply, sizeof(reply));
                return sizeof(SMsgEcho);
            }
        }
        return nSize;
    }
};
DEFINE_DYNAMIC_CLASS(CServerConn)  // 实现动态类
```

### 3. 服务端启动

```cpp
auto* pMgr = CreateConnMgr(30);  // 30 秒自动断开超时
pMgr->StartService("0.0.0.0", 19860, CServerConn::GetID(), eConnType_TCP_Prt);
while (running) {
    pMgr->Check(100);  // 每 100ms 驱动一次
}
pMgr->StopAllService();
pMgr->Release();
```

### 4. 客户端连接

```cpp
auto* pMgr = CreateConnMgr(30);
auto* pConn = pMgr->Connect("127.0.0.1", 19860, CClientConn::GetID(), eConnType_TCP_Prt);
while (running) {
    pMgr->Check(10);
}
pMgr->StopAllConnect();
pMgr->Release();
```

## 示例工程

| 路径 | 说明 |
|------|------|
| `sample/GammaConnects/server.cpp` | TCP Prt 服务端监听，`CServerConn` 收 Echo 并回复 |
| `sample/GammaConnects/client.cpp` | 连接服务器，发送 Echo 消息 |
| `sample/GammaConnects/SampleProtocol.h` | 示例业务消息 `eSM_Echo` / `eSM_EchoReply` |

构建目标：`GammaConnectsServerSample`、`GammaConnectsClientSample`（见 `sample/GammaConnects/CMakeLists.txt`）。

## 与 GammaNetwork 的边界

| 层次 | 职责 |
|------|------|
| **GammaNetwork** | 地址、监听、套接字 I/O、DNS 解析、平台差异封装 |
| **GammaConnects** | 连接生命周期管理、多连接列表、Shell 分包、心跳、Prt/WebSocket/KCP 协议、业务 `CBaseConn` 绑定 |

扩展新传输行为时，优先在 Network 层扩展 `IConnecter`/`INetwork`；Connects 层通过 `CConnection` 派生或新 `EConnType` 接入。

## 相关文件索引

| 类别 | 路径 |
|------|------|
| 对外接口 | `include/.../GammaConnects/IConnectionMgr.h` |
| | `include/.../GammaConnects/CBaseConn.h` |
| | `include/.../GammaConnects/ConnectDef.h` |
| 管理器实现 | `source/gamma/GammaConnects/CConnectionMgr.cpp/.h` |
| | `source/gamma/GammaConnects/CListenHandler.cpp/.h` |
| 连接基类 | `source/gamma/GammaConnects/CConnection.cpp/.h` |
| Prt 连接 | `source/gamma/GammaConnects/CPrtConnection.cpp/.h` |
| | `source/gamma/GammaConnects/GammaPrt.h` |
| WebSocket 连接 | `source/gamma/GammaConnects/CWebConnection.cpp/.h` |
| | `source/gamma/GammaConnects/WebSocketPrt.h` |
| 分派框架 | `source/gamma/GammaConnects/TDispatch.h` |
| | `source/gamma/GammaConnects/TDispatch.inl` |
| | `source/gamma/GammaConnects/TBasePrtlMsg.h` |
| KCP 实现 | `source/gamma/GammaConnects/ikcp.h` |
| | `source/gamma/GammaConnects/ikcp.cpp` |

---

*文档对应仓库内 GammaConnects 模块源码结构；若 CMake 或头文件路径与本地不一致，以当前工程 `include_directories` 为准。*
