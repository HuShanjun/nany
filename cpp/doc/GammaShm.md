# GammaShm 模块文档

## 概述

GammaShm 是一个基于**文件内存映射（mmap）**的共享内存持久化模块，用于游戏服务器中将内存数据增量提交到数据库。它在后台线程中运行，通过 diff 算法检测数据变化，仅提交发生变更的数据块，实现高效的数据持久化。

## 核心概念

### 数据层次结构

```
SShareCommonHead（共享内存头）
  └── SChunckTypeInfo[]（大数据块类型，如角色、帮会等，由游戏定义）
        └── SChunckHead[]（大数据块头，每个代表一个具体实体）
              └── SBlockInfo[]（小数据块描述，如技能、道具等）
                    └── SBlockData + 实际数据（带版本号的数据块）
```

### 状态机

共享内存中的每个 Chunk 按以下状态流转：

```
状态                | 生成方 | Gas 可改变为         | Shm 可改变为
--------------------|--------|---------------------|-----------------
eSCT_Invalid        | Shm    | eSCT_UpdateBlock    |
eSCT_UpdateBlock    | Gas    | eSCT_UpdateTrunk    |
eSCT_UpdateTrunk    | Gas    |                     | eSCT_UpdateFlg
eSCT_UpdateFlg      | Shm    |                     | eSCT_WaitingFlg
eSCT_WaitingFlg     | Shm    |                     | eSCT_Invalid
```

- **Gas**：游戏应用服务器（主线程），负责写入数据
- **Shm**：共享内存后台线程，负责检测变更并提交

### 无锁并发控制

Gas 与 Shm 线程之间通过 `m_nCommitIDFirst` / `m_nCommitIDLast` 实现无锁并发检测。Shm 线程在读取数据前后校验两个 ID 是否一致，若不一致说明 Gas 正在写数据，跳过本次处理等待下次检查。

### 增量提交（Diff）

对于包含数组结构的数据块（`m_nArrayElemSize > 0`），模块维护一份旧数据副本，通过逐元素比较生成 diff，仅提交变更的数组元素。使用 `TBitSet` 标记已提交但未确认的元素，确保失败重试时不遗漏。

## 公共接口

### IShareMemoryHandler（回调接口）

使用方需实现此接口来处理 Shm 线程的提交请求：

```cpp
class IShareMemoryHandler
{
public:
    virtual void OnReboot() = 0;
    virtual void OnCommitBlocksInfo(const SCommitBlocksInfo* pInfo, size_t nSize) = 0;
    virtual void OnCommitFlag(const SCommitFlag* pInfo, size_t nSize) = 0;
    virtual void OnCommitAllBlocksOK(const SCommitFlag* pInfo, size_t nSize) = 0;
};
```

- `OnReboot()`：启动时回调，用于从共享内存恢复数据
- `OnCommitBlocksInfo()`：提交数据块变更（差量数据）
- `OnCommitFlag()`：提交标志位到数据库
- `OnCommitAllBlocksOK()`：所有数据块提交完成的通知

### IShareMemoryMgr（管理器接口）

```cpp
class IShareMemoryMgr : public IGammaUnknown
{
public:
    virtual SShareCommonHead* GetShareMemory() = 0;
    virtual void   SetCheckInterval(uint32_t nInterval) = 0;
    virtual void   Start() = 0;
    virtual bool   IsReady() = 0;
    virtual void   Check() = 0;
    virtual void   NotifyAppClose() = 0;

    // 回调完成通知（在处理完 Handler 回调后调用）
    virtual void   OnRebootFinish(bool bFailed) = 0;
    virtual void   OnCommitAllBlocksOKFinish(const SCommitFlag* pInfo, bool bFailed) = 0;
    virtual void   OnCommitBlocksFinish(const SCommitBlocksInfo* pInfo, bool bFailed) = 0;
    virtual void   OnCommitFlagFinish(const SCommitFlag* pInfo, bool bFailed) = 0;
};
```

### 工厂函数

```cpp
IShareMemoryMgr* CreateShareMemoryMgr(
    uint16_t nGasID,                // 游戏服务器 ID
    SShareCommonHead* pSrc,         // 源共享内存头（定义内存布局）
    const char* szFileName,         // 共享内存映射文件路径
    IShareMemoryHandler* pHandler,  // 回调处理器
    uint32_t nCommitInterval,       // 提交间隔（毫秒）
    bool bKeepShmFile,              // 关闭时是否保留 shm 文件
    uint32_t nFlushInterval         // Linux 下刷盘间隔（毫秒），INVALID_32BITID 表示不启用
);
```

## 使用流程

1. **构造共享内存布局**：按 `SShareCommonHead → SChunckTypeInfo[] → SChunckHead[] → SBlockInfo[] → 数据区` 的格式在堆上构造源数据
2. **创建管理器**：调用 `CreateShareMemoryMgr()` 创建实例，模块会自动创建或打开内存映射文件
3. **启动**：调用 `Start()`，模块启动后台检查线程和同步线程（Linux）
4. **主循环**：每帧调用 `Check()` 处理来自后台线程的回调
5. **处理回调**：在 `IShareMemoryHandler` 的回调中执行数据库操作，完成后调用对应的 `On*Finish()` 方法
6. **关闭**：调用 `NotifyAppClose()`，模块会等待所有提交完成后退出

## 关键数据结构

| 结构                 | 说明                                                       |
|---------------------|------------------------------------------------------------|
| `SShareCommonHead`  | 共享内存文件头，包含版本、文件大小、ChunkType 数量          |
| `SChunckTypeInfo`   | 大数据块类型描述，包含偏移、大小、数量及是否需要提交标志     |
| `SChunckHead`       | 大数据块头，包含 ID、状态、版本号、提交 ID 等               |
| `SBlockInfo`        | 小数据块描述，包含类型、偏移、大小及数组信息                |
| `SBlockData`        | 小数据块运行时数据，包含版本号、提交状态、UUID 等           |
| `CDataContainer`    | 数据容器基类，带观察者模式和 UUID                           |

## 平台差异

- **Windows**：`GammaShm` 编译为静态库；内存映射文件由 OS 自动同步
- **Linux**：`GammaShm` 编译为动态库；启动额外的 `ShmSync` 线程定期调用 `GammaMemoryMapSyn()` 刷盘

## 线程模型

```
主线程 (Gas)                    ShmCheck 线程                ShmSync 线程 (仅 Linux)
    |                               |                            |
    |--- 写入共享内存 ------------->|                            |
    |                               |--- 检测数据变更            |
    |                               |--- BuildDiff              |
    |                               |--- SendCmd (QueryBuffer)  |
    |<-- Check() 读取 QueryBuffer --|                            |
    |--- 执行 DB 操作               |                            |
    |--- On*Finish (FinishBuffer)-->|                            |
    |                               |--- CheckResult            |
    |                               |--- 更新状态               |
    |                               |                            |--- 定期刷盘
```

`CCircleBuffer` 作为无锁环形缓冲区，在主线程与后台线程之间传递命令和结果。
