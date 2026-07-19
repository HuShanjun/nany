# Center/Login/Gate/Game/DBMain 基础骨架增强设计

> 日期：2026-07-19  
> 状态：设计已评审通过，待写实现计划  
> 范围：公共 PeerOnline + 四服业务 Store + Gate 迁 PeerOnline；不含真实协议/DB/发包/CGNetwork 修复

## 1. 背景与目标

五类服 DLL 已可加载。Gate 已具备 `CGateSessionStore`、客户端事件与转发 stub。Center / Login / Game / DBMain 仍仅有生命周期日志。

**目标（选项 A + 共享 PeerOnline 方案 B + 落地 方案 1）：**

1. 引入 header-only `CPeerOnlineStore`（`include/GameApp/`）
2. Login / Center / Game / DBMain 各增业务 Store + Module 钩子/stub API
3. Gate 的 `CGateSessionStore` 用 `CPeerOnlineStore` 替换自管 `m_setServers`（单测行为不变）
4. 各 Store 配 GoogleTest；不要求 TCP 烟测

**非目标：** 鉴权协议、真实 SQL、`SendShellMsg`、修 `CGNetwork::Check`。

## 2. 决策摘要

| 项 | 选择 |
|----|------|
| 深度 | 对齐 Gate 的增强骨架 |
| 在线表 | 公共 `CPeerOnlineStore` + 各模块私有业务 Store |
| Gate | 一并迁移到 PeerOnline |
| PeerOnline 形态 | header-only，避免额外静态库 |

## 3. 架构

```
CPeerOnlineStore (include/GameApp/CPeerOnlineStore.h)
  ↑ 组合
CLoginAuthStore | CCenterRegistryStore | CGamePlayerStore | CDBMainJobStore | CGateSessionStore
  ↑
CModuleLogin | CModuleCenter | CModuleGame | CModuleDBMain | CModuleGate
```

## 4. Store API

### 4.1 CPeerOnlineStore

```cpp
void Add(uint32_t nServerID);    // 0 忽略
void Remove(uint32_t nServerID);
bool Has(uint32_t nServerID) const;
size_t Size() const;
```

内部 `std::set<uint32_t>`。

### 4.2 CGateSessionStore（改造）

- 删除 `m_setServers`，成员改为 `CPeerOnlineStore m_peers`
- `AddServer`/`RemoveServer`/`HasServer`/`CanForwardToServer` 委托 `m_peers`
- `RemoveServer` 在 peer Remove 后仍清除绑定该服的客户端
- 客户端 map 与 Bind/CanForward 语义不变；现有 `GateSessionStore_Unit` 保持通过

### 4.3 CLoginAuthStore

```cpp
// nState: 0=pending, 1=ok, 2=fail
struct SLoginTicket { uint32_t nTicketID; std::string strAccount; uint8_t nState; };
uint32_t SubmitAuth(const std::string& account); // 空账号失败返回 0
bool CompleteAuth(uint32_t ticket, bool ok);
bool HasTicket(uint32_t ticket) const;
```

Module：`OnServerConnect/DisConnect` → peers；`SubmitAuth`/`CompleteAuth` 打日志后调 Store。

### 4.4 CCenterRegistryStore

内嵌 `CPeerOnlineStore`。

```cpp
struct SRegisteredServer { uint32_t nServerID; uint32_t nTypeID; };
bool RegisterServer(uint32_t id, uint32_t typeId); // 要求 peers.Has(id)
void UnregisterServer(uint32_t id);
bool IsRegistered(uint32_t id) const;
```

Module：`OnServerConnect` → `peers.Add` + `RegisterServer(id, id/100)`；disconnect → Unregister + Remove。

### 4.5 CGamePlayerStore

内嵌 `CPeerOnlineStore`。

```cpp
struct SGamePlayer { uint64_t nPlayerID; uint32_t nGateServerID; };
bool PlayerEnter(uint64_t playerId, uint32_t gateServerId); // 要求 peers.Has(gate)
void PlayerLeave(uint64_t playerId);
bool HasPlayer(uint64_t playerId) const;
```

`Remove` peer 时：清除 `nGateServerID == 该服` 的玩家。

### 4.6 CDBMainJobStore

内嵌 `CPeerOnlineStore`。

```cpp
// nState: 0=queued, 1=done
struct SDBJob { uint32_t nJobID; uint32_t nFromServerID; uint8_t nState; };
uint32_t Enqueue(uint32_t fromServerId); // 要求 peers.Has(from)；失败 0
bool Complete(uint32_t jobId);
bool HasJob(uint32_t jobId) const;
```

## 5. 文件布局

| 路径 | 说明 |
|------|------|
| `include/GameApp/CPeerOnlineStore.h` | header-only |
| `source/module/ModuleGate/CGateSessionStore.*` | 迁 PeerOnline |
| `source/module/ModuleLogin/CLoginAuthStore.*` | 新增 |
| `source/module/ModuleCenter/CCenterRegistryStore.*` | 新增 |
| `source/module/ModuleGame/CGamePlayerStore.*` | 新增 |
| `source/module/ModuleDBMain/CDBMainJobStore.*` | 新增 |
| 对应 `ModuleXxx.h/.cpp` | 钩子 + stub API |
| `test/ModuleLogin` 等 | Store 单测；`test_PeerOnlineStore` 可放 `test/GameApp` |

## 6. 测试与验收

1. PeerOnline：Add/Remove/Has/忽略 0  
2. 各业务 Store：成功路径 + 缺 peer 失败 + disconnect 清理（按上表）  
3. Gate 既有单测回归  
4. 五 Module + GameApp 构建成功  
5. 无 TCP / 真实 DB 验收要求  

## 7. 非目标

鉴权协议、MariaDB 读写、真实转发、修复网络崩溃。
