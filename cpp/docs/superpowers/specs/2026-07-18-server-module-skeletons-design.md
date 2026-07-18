# 五类游戏服模块骨架设计（Login / Game / Gate / Center / DBMain）

> 日期：2026-07-18  
> 状态：设计已评审通过，待写实现计划  
> 范围：骨架 DLL + GameApp 加载打通 + LocalPort 监听修正；不含业务协议

## 1. 背景与目标

项目已有统一进程壳 `GameApp`（`--devops` + `--name`）、`machine.toml` 服配置与 `NetTopology`、以及按服类型加载 DLL 的 `CModuleMgr` + `process_module.toml`。`source/module/` 为空；`GameApp::OnInit` 中 `LoadDll` 被注释；`CNetComp::OnStarted` 仅在存在 `PublicPort` 时 `StartService`，导致仅有 `LocalPort` 的 Center / Login / DBMain 无法被拓扑对端连接。

**目标（本期 = 选项 C + 独立 DLL 方案 2）：**

1. 实现五个独立主模块 DLL：`ModuleLogin` / `ModuleGame` / `ModuleCenter` / `ModuleGate` / `ModuleDBMain`
2. 每个 DLL 完整实现 `IConsummer` 与导出符号 `InitDllServant` / `GetConsummer`（模块间不共享业务代码）
3. 恢复 `GameApp` 按 `m_strServerType` 加载对应段配置；补齐 `DBMain` 配置与 `m_strBinPath`
4. 修正 `CNetComp`：始终监听 `LocalPort`；有 `PublicPort` 时额外监听公网端口
5. 主动连接路径上将 `CGameConnServer` 连接/断开回调接到 `CModuleMgr::OnServerConnect/DisConnect`

**非目标（YAGNI）：**

- 账号鉴权、角色、场景、持久化等业务逻辑
- 服间身份握手协议（被动接入侧识别对端 ServerID）
- Gate ↔ Client 完整转发链路
- 模块间公共静态库（本期刻意选择五个完全独立 DLL）

## 2. 决策摘要

| 项 | 选择 |
|----|------|
| 完成度 | 骨架 + 生命周期钩子 + LocalPort 监听（选项 C） |
| 组织方式 | 统一 `GameApp` + 每服类型一个主 DLL（沿用现有约定） |
| 代码复用 | 五个完全独立 DLL，不抽 ModuleCommon（方案 2） |
| 组网 | 继续由 `CNetComp` + `machine.toml` `[NetTopology]` 驱动 |
| 模块回调 | 日志版 `OnInit` / `OnTimer` / `OnServerConnect` / `OnServerDisConnect` / `OnServerStop` |

## 3. 整体架构

```
GameApp (--name Center|Login|Game01|Gate01|DBMain ...)
  ├─ LoadConfig(machine.toml)
  ├─ LoadServerInfo → ServerType / ServerID
  ├─ RegisterComp(CNetComp)     # 解析拓扑、准备主动连接表
  ├─ CModuleMgr::LoadDll(ServerType, process_module.toml)
  │     └─ libModuleXxx.so  → IConsummer::OnInit
  └─ CNetComp::OnStarted
        ├─ StartService(LocalHost, LocalPort)
        ├─ [可选] StartService(PublicHost, PublicPort)
        └─ Register reconnect tick → Connect 拓扑目标
```

目录与产物：

```
source/module/
  ModuleLogin/     → libModuleLogin.so / ModuleLogin.dll     (id 10000)
  ModuleGame/      → libModuleGame.so                        (id 20000)
  ModuleCenter/    → libModuleCenter.so                      (id 30000)
  ModuleGate/      → libModuleGate.so                        (id 40000)
  ModuleDBMain/    → libModuleDBMain.so                      (id 50000)
```

DLL 输出到与 `GameApp` 相同的 `LIBRARY_OUTPUT_PATH`（`bin/<platform>/<Debug|Release>`），以便 `CModuleMgr::GetDynamicFileName` 按 bin 路径加载。

## 4. 模块骨架

每个模块目录结构统一（以 Login 为例）：

```
source/module/ModuleLogin/
  CMakeLists.txt
  ModuleLogin.h
  ModuleLogin.cpp
  ModuleExport.cpp
```

| 文件 | 职责 |
|------|------|
| `ModuleLogin.h/.cpp` | `CModuleLogin : IConsummer`，`DEFINE_MODULE(10000)`，生命周期打日志 |
| `ModuleExport.cpp` | 导出 `InitDllServant(IGameApp*)`、`GetConsummer(IConsummerPtr&)` |
| `CMakeLists.txt` | `add_library(... SHARED)`，链接 `GammaCommon`（及加载所需最小依赖） |

钩子行为：

| 钩子 | 行为 |
|------|------|
| `OnInit` | 日志：模块名、ModuleID、当前进程 ServerID/Type（经 `IGameApp` / `GetGameApp`） |
| `UnInit` | 日志 |
| `OnTimer` | 返回 0，无业务 |
| `OnServerConnect` / `OnServerDisConnect` | 日志对端 ServerID |
| `OnServerStop` | 日志，返回 `true` |

模块 ID 与 `process_module.toml`：

| 段 | 模块名 | ID |
|----|--------|-----|
| `[Login]` | ModuleLogin | 10000 |
| `[Game]` | ModuleGame | 20000 |
| `[Center]` | ModuleCenter | 30000 |
| `[Gate]` | ModuleGate | 40000 |
| `[DBMain]` | ModuleDBMain | 50000（新增段） |

主模块 ID 满足 `IS_MAIN_DLL`（`id % 10000 == 0`）。

### GameApp / ModuleMgr 改动

1. `GameApp::OnInit`：在 `LoadServerInfo` 成功后调用  
   `CModuleMgr::Instance()->LoadDll(m_strServerType.c_str(), process_module.toml路径)`
2. `CModuleMgr`：在加载前设置 `m_strBinPath` 为可执行文件所在目录（当前成员未赋值会导致找不到 so/dll）
3. 根 `CMakeLists.txt`：`add_subdirectory` 五个模块目标

## 5. 网络修正

### 5.1 监听

`CNetComp::OnStarted`：

1. 若 `m_pCurServerInfo` 有效且 `LocalPort > 0`：`StartService(LocalHost, LocalPort, …)`
2. 若 `PublicPort > 0`：再 `StartService(PublicHost, PublicPort, …)`
3. 骨架阶段两个端口共用现有连接工厂（`CGameConnServer` / `CGameConnFromClient`）；后续再拆客户端连接类型

### 5.2 回调

- 主动 `Connect` 成功且已 `SetServerID` 时：调用 `CModuleMgr::OnServerConnect(serverID)`
- 断开时：调用 `CModuleMgr::OnServerDisConnect(serverID)`（若 ServerID 已知）
- **被动接入**且尚未交换身份：只打连接日志，不伪造 ServerID；身份握手留到下一期

### 5.3 拓扑（沿用 `machine.toml`，不改语义）

| 本机类型 | 主动连接目标类型 |
|----------|------------------|
| Login | Center, DBMain |
| Game | Center, DBMain, ServerID 更大的 Game |
| Gate | Login, Center, Game |
| Center | DBMain |
| DBMain | （无） |

同类型多实例规则保持现有逻辑：仅当目标 `ServerID >` 本机时主动连（避免双连）。

## 6. 错误处理

- `LoadDll` 失败：沿用 `CModuleMgr` 现有异常路径，进程初始化失败退出
- `StartService` 失败：打 Error 日志；不静默忽略（实现时至少记录端口与地址）
- 重连 tick：目标已在 `m_treeServer` 中则跳过；`Connect` 失败打 Error 并下轮重试

## 7. 验收标准

1. 五个 SHARED 模块与 `GameApp` 同输出目录构建成功；`process_module.toml` 含 `[DBMain]`
2. `GameApp --devops <path> --name Center`（及 Login / Game01 / Gate01 / DBMain）启动后出现对应模块 `OnInit` 日志
3. 先起 `Center`（及 `DBMain`），再起 `Login`：Login 侧出现对拓扑目标的 Connect 尝试；Center/DBMain 侧能 `StartService` LocalPort
4. 有 `PublicPort` 的 Game/Gate 同时监听 Local 与 Public

## 8. 测试策略（本期）

- 以手工启动多进程联调为主（见验收标准）
- 不强制新增 GoogleTest 用例；若实现中抽出纯函数（如 bin 路径解析），可顺带加单测，非必须

## 9. 实现影响面（文件级）

| 区域 | 动作 |
|------|------|
| `source/module/Module{Login,Game,Center,Gate,DBMain}/` | 新增 |
| `CMakeLists.txt` | 注册五个子目录 |
| `data/devops/etc/process_module.toml` | 增加 `[DBMain]` |
| `source/GameApp/GameApp.cpp` | 启用 `LoadDll` |
| `source/GameApp/ModuleMgr.cpp` | 设置 `m_strBinPath` |
| `source/GameApp/NetComp/CNetComp.cpp` | Local + Public 双监听 |
| `source/GameApp/NetComp/CGameConnServer.cpp` | 接通 ModuleMgr 连接回调 |
