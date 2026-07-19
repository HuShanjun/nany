# ModuleGate 网关骨架增强设计

> 日期：2026-07-19  
> 状态：设计已评审通过，待写实现计划  
> 范围：Gate 会话表 + 客户端事件路径 + 转发 stub；不含真实发包与 CGNetwork 修复

## 1. 背景与目标

`ModuleGate` 目前仅为 `IConsummer` 骨架（日志版生命周期）。拓扑上 Gate 对外 `PublicPort` 接客户端、对内连 Login/Center/Game（`machine.toml` `[NetTopology] Gate = ["Login","Center","Game"]`）。`CNetComp` 已有 `AddClientConnect`/`DelClientConnect`，但 `CGameConnFromClient` 未调用；`IConsummer` 仅有服连接钩子。

**目标（选项 A + 接线 B + 方案 1）：**

1. 扩展 `IConsummer`：`OnClientConnect` / `OnClientDisConnect`（默认空实现）
2. `CModuleMgr` 广播客户端连接事件（对齐 `OnServerConnect`）
3. `CGameConnFromClient` → `CNetComp` → `CModuleMgr` → `ModuleGate`
4. `ModuleGate` 维护客户端会话表与后端在线表；提供绑定与转发 stub（日志，不真实发包）
5. 单元测试覆盖会话增删 / 绑定 / 转发失败路径

**非目标：**

- 修复 `CGNetwork::Check` SIGSEGV / 真实 TCP 联调
- 鉴权、协议解析、Game 负载分配、会话迁移
- 实现 `INetComp::SendMsgTo*` 真实发送

## 2. 决策摘要

| 项 | 选择 |
|----|------|
| 完成度 | 网关骨架增强（选项 A） |
| 事件路径 | ModuleGate 建表 + NetComp 回调打通（接线 B） |
| 接口形态 | 扩展 `IConsummer` 默认空钩子（方案 1） |
| 转发 | Gate 内部 stub，成功仅打日志 |
| 运行时 TCP 验收 | 不要求（引擎已知崩溃） |

## 3. 整体架构

```
客户端 TCP (PublicPort)
  → CGameConnFromClient::OnConnected / OnDisConnect
  → CNetComp::AddClientConnect / DelClientConnect
  → CModuleMgr::OnClientConnect / OnClientDisConnect
  → IConsummer::OnClient* （默认空）
  → CModuleGate 更新 m_mapClients

服间连接（已有）
  → CGameConnServer → ModuleMgr::OnServer*
  → CModuleGate 更新 m_mapServers
```

## 4. 会话与接口

### 4.1 IConsummer

```cpp
virtual void OnClientConnect(uint32 nClientID) {}
virtual void OnClientDisConnect(uint32 nClientID) {}
```

其它模块无需改动。

### 4.2 CModuleMgr

新增：

```cpp
void OnClientConnect(uint32 nClientID);
void OnClientDisConnect(uint32 nClientID);
```

实现：`nClientID == 0` 则返回；否则遍历 `m_mapDll` 调用对应钩子（与 `OnServerConnect` 相同模式）。

### 4.3 ModuleGate 数据结构

```cpp
struct SGateClientSession {
    uint32 nClientID = 0;
    uint32 nBoundServerID = 0;  // 0 = 未绑定
    int64  nConnectTime = 0;
};

std::map<uint32, SGateClientSession> m_mapClients;
std::set<uint32> m_mapServers;  // 在线后端 ServerID
```

为便于单测，将表操作抽为 `CGateSessionStore`（头文件可放在 `source/module/ModuleGate/` 或 `include/GameApp/`，无外部依赖），`CModuleGate` 持有并委托。

### 4.4 钩子行为

| 事件 | 行为 |
|------|------|
| `OnClientConnect` | 插入会话（已存在则覆盖/忽略并打日志），记录时间 |
| `OnClientDisConnect` | 擦除会话 |
| `OnServerConnect` | `m_mapServers.insert` |
| `OnServerDisConnect` | erase；所有 `nBoundServerID == 该服` 的客户端绑定置 0 |

### 4.5 转发 / 绑定 stub

```cpp
bool BindClientToServer(uint32 nClientID, uint32 nServerID);
bool ForwardToServer(uint32 nClientID, uint32 nServerID, const void* pData, size_t nSize);
bool ForwardToClient(uint32 nClientID, const void* pData, size_t nSize);
```

- `BindClientToServer`：客户端与服均须在表中；成功则写 `nBoundServerID`
- `ForwardTo*`：目标缺失 → Error + `false`；存在 → Info（含 id/size）+ `true`，**不**调用 `SendShellMsg`

## 5. NetComp 接线

1. `CGameConnFromClient::OnConnected`：`GetNetComp()` → `AddClientConnect(this)` → `CModuleMgr::OnClientConnect(GetConnectID())`
2. `OnDisConnect`：先 `OnClientDisConnect(id)`，再 `DelClientConnect(this)`
3. `AddClientConnect`：按 clientId 查重后 Insert；`DelClientConnect` 保持现有 Remove 逻辑

`GetNetComp` 复用 `CGameConnServer` 已有模式：`CBaseApp::Inst()->GetComp(CNetComp::GetID())`。

## 6. 错误处理

- 空指针连接对象：直接返回
- `nClientID == 0`：ModuleMgr 忽略
- Bind/Forward 缺会话或缺后端：日志 + `false`，不抛异常

## 7. 测试与验收

1. `CGateSessionStore`（或等价）GoogleTest：connect/disconnect、bind 成功/失败、server disconnect 清绑定、forward 缺目标失败
2. `ModuleGate` + `GameApp` 构建成功
3. **不要求** PublicPort 真实接入烟测

## 8. 影响面

| 区域 | 动作 |
|------|------|
| `include/Interface/IConsummer.h` | 新增客户端钩子 |
| `include/GameApp/ModuleMgr.h` + `ModuleMgr.cpp` | OnClient* 广播 |
| `CGameConnFromClient.cpp` | 接通 NetComp + ModuleMgr |
| `CNetComp` Add/DelClient | 查重加固（如需要） |
| `ModuleGate/*` + `CGateSessionStore` | 会话与 stub |
| `test/GameApp/` 或 `test/ModuleGate/` | 会话单测 |
