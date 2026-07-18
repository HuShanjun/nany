# nany-cpp GoogleTest 单元/集成测试框架设计

> 日期：2026-07-18  
> 状态：待实现（设计已评审通过）  
> 范围：测试基础设施 + 目录/CMake 约定 + 各模块用例分层规划

## 1. 背景与目标

nany-cpp 是跨 Windows（MSVC）/ Linux 的 C++ 游戏引擎基础库。当前：

- `test/` 目录存在但为空
- 无 GoogleTest / Catch2 / doctest 等依赖
- 无 `enable_testing()` / `ctest` 接入
- `sample/*` 仅为手工演示程序，不作为自动化测试

**目标：** 引入 GoogleTest（经 vcpkg），按模块建立可执行测试目标，用 label 区分 unit / integration，支持全面覆盖（含网络、数据库等集成场景）。

**非目标（YAGNI）：**

- 不自研测试框架
- 不把 `sample/*` 改造成测试
- 首期不上 CI 矩阵、不上 testcontainers
- 不为测试而大规模改生产接口；需要替身时优先对已有 `I*` 接口手写 fake

## 2. 决策摘要

| 项 | 选择 |
|----|------|
| 框架 | GoogleTest（含 gtest_main；gmock 按需后续启用） |
| 依赖获取 | vcpkg（`vcpkg/vcpkg.json` 增加 `gtest`） |
| 组织方式 | 每模块一个测试可执行文件（方案 1） |
| 覆盖范围 | 全模块：unit + integration |
| 开关 | `NANY_BUILD_TESTS`（默认 ON） |

## 3. 目录结构

```
test/
├── CMakeLists.txt
├── README.md
├── cmake/
│   └── GammaTest.cmake          # gamma_add_gtest(...)
├── common/                      # 共享辅助（头文件为主）
│   ├── TestEnv.h                # 读 NANY_TEST_* 环境变量
│   ├── TempDir.h                # 临时目录 RAII
│   └── SkipIf.h                 # 集成依赖探测 + GTEST_SKIP 辅助
├── config/
│   └── test.example.toml        # 集成配置模板（真实 test.toml 不入库）
├── GammaCommon/
│   ├── CMakeLists.txt
│   └── test_*.cpp
├── GammaNetwork/
├── GammaDatabase/
├── GammaMTDbs/
├── GammaConnects/
├── GammaScript/
├── GammaShm/
├── GammaApp/                    # 可选：纯逻辑
└── GameApp/                     # 可选：配置/命令行等可测片段
```

**命名约定：**

- 可执行文件：`test_<Module>`（如 `test_GammaCommon`）
- 源文件：`test_<Topic>.cpp`（如 `test_GammaMath.cpp`）
- 用例标签：`unit`、`integration`；可选 `slow`
- `test/common` 不链接业务模块，只提供辅助

## 4. 构建与运行

### 4.1 根 CMake

```cmake
option(NANY_BUILD_TESTS "Build unit/integration tests" ON)
if(NANY_BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()
```

### 4.2 vcpkg

在 `vcpkg/vcpkg.json` 的 `dependencies` 中增加 `"gtest"`。配置前需按现有流程重新 install / 使用已有 preset 的 vcpkg 工具链。

### 4.3 公共宏 `gamma_add_gtest`

定义于 `test/cmake/GammaTest.cmake`，典型用法：

```cmake
gamma_add_gtest(test_GammaCommon
  SOURCES test_GammaMath.cpp test_Hash.cpp
  LIBS GammaCommon
  LABELS unit
)
```

宏职责：

1. `add_executable`
2. 链接 `GTest::gtest_main` 与业务库（及平台所需传递依赖）
3. `gtest_discover_tests`，并将 `LABELS` 写入测试属性
4. 输出目录与现有 `bin/${PLATFORM_NAME}/...` 约定一致
5. 对共享同一 DB/端口的 integration 目标，支持设置 `RUN_SERIAL`

### 4.4 常用命令

```bash
cmake --preset linux-vcpkg-static   # 或 vcpkg-static
cmake --build build/linux           # 或 build/msvc

ctest --test-dir build/linux -L unit --output-on-failure
ctest --test-dir build/linux -L integration --output-on-failure
ctest --test-dir build/linux -R test_GammaCommon --output-on-failure
```

关闭测试构建：`-DNANY_BUILD_TESTS=OFF`。

## 5. 集成测试配置

**优先级：** 环境变量 > `test/config/test.toml`。

建议环境变量：

| 变量 | 含义 |
|------|------|
| `NANY_TEST_DB_HOST` | MariaDB 主机 |
| `NANY_TEST_DB_PORT` | 端口 |
| `NANY_TEST_DB_USER` | 用户 |
| `NANY_TEST_DB_PASSWORD` | 密码 |
| `NANY_TEST_DB_DATABASE` | 库名 |

`test/config/test.example.toml` 提供模板；本地复制为 `test/config/test.toml`，并在仓库 `.gitignore` 中忽略 `test/config/test.toml`（若不存在 `.gitignore` 条目则新增）。

**依赖不可用时：** 相关用例调用 `GTEST_SKIP()`（可带原因字符串），不记为失败。

## 6. 模块用例分层

| 模块 | unit（默认必过） | integration（可 SKIP） |
|------|------------------|------------------------|
| GammaCommon | 数学/向量、容器、字符串/格式化、MD5/SHA1 已知向量、压缩 round-trip | 临时目录文件 I/O、线程创建/join |
| GammaScript | C++ 类注册 ↔ Lua 调用往返 | （多数可 unit；依赖内嵌 lua） |
| GammaNetwork | 地址/URL 解析、缓冲区组帧 | loopback TCP 客户端↔服务端 |
| GammaConnects | 协议分发/字节缓冲、KCP 喂包 | `CreateConnMgr` loopback 会话 |
| GammaDatabase | 纯逻辑（若有参数/SQL 拼装） | 真实 MariaDB：连库、CRUD |
| GammaMTDbs | 队列/分片等可测逻辑 | 多线程查询 + 真实 DB |
| GammaShm | 块/状态机在堆缓冲上推进 | 临时文件 mmap 生命周期 |
| GammaApp / GameApp | toml 解析、日志级别映射等 | DLL 热加载等仍以 sample/手工为主 |

### 6.1 落地顺序

1. 脚手架：vcpkg gtest + `gamma_add_gtest` + `test_GammaCommon` 冒烟
2. GammaCommon 扩面 → GammaScript → Network/Connects unit → Shm unit
3. Database / MTDbs / Network integration
4. GameApp 配置类 unit

### 6.2 用例与生命周期约定

- 工厂返回的 `IGammaUnknown*`：结束前必须 `Release()`，禁止裸 `delete`
- Fixture `TearDown` 负责释放；推荐 RAII 包装引用计数
- 单例（如 `CModuleMgr`）：避免同进程用例互相污染；必要时串行或独立可执行文件
- 共用 DB/端口的 integration 用例：CMake 属性 `RUN_SERIAL`
- 默认允许 `ctest -j`；仅串行标记的用例例外

### 6.3 失败语义

| 情况 | 行为 |
|------|------|
| 断言失败 | FAIL，`ctest` 非 0 |
| 集成依赖不可用 | `GTEST_SKIP`，不失败 |
| 超时/慢用例 | 用例内超时；可选 label `slow` |
| 崩溃 | 视为失败；ASan 为可选项，非首期必做 |

## 7. 验收标准

框架脚手架完成即视为本设计落地成功：

1. `NANY_BUILD_TESTS=ON` 时可构建至少一个 `test_GammaCommon`
2. 无 MariaDB 环境下，`ctest -L unit` 全部通过
3. DB 不可用时，integration 用例为 SKIP 而非 FAIL
4. `test/README.md` 说明：gtest 依赖、DB 配置、常用 `ctest` 命令

## 8. 与现有工程的关系

- 公共头路径、输出目录、平台宏继续遵循根 `CMakeLists.txt` / `common.cmake`
- 测试目标链接方式对齐 `sample/*`（消费已构建的 Gamma 库，不定义各模块 `_EXPORTS`）
- 文档语言：与仓库一致，以中文为主

## 9. 后续实现

设计批准后，另写实现计划：  
`docs/superpowers/plans/YYYY-MM-DD-gtest-unit-test-framework.md`，按任务拆分脚手架与各模块首批用例。
