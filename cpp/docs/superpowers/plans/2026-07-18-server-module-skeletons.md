# Server Module Skeletons Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Ship five independent main-module DLLs (Login/Game/Center/Gate/DBMain), wire `GameApp` to load them, and fix `CNetComp` so LocalPort listens and reconnect tracking works.

**Architecture:** One `GameApp` process shell selects role via `--name`; `CModuleMgr` loads the matching SHARED module from `process_module.toml`. Each module is a self-contained `IConsummer` with exported `InitDllServant` / `GetConsummer`. Networking stays in `CNetComp` (Local + optional Public listen; active Connect per `[NetTopology]`); connection events notify modules via `CModuleMgr`.

**Tech Stack:** CMake SHARED libs, `IConsummer` / `CModuleMgr`, toml++ config, GammaCommon/GammaApp/GammaConnects, existing `CGameConnServer` / `CGameConnFromClient`.

**Spec:** `docs/superpowers/specs/2026-07-18-server-module-skeletons-design.md`

## Global Constraints

- Five **independent** DLLs — do **not** introduce a shared ModuleCommon static lib or shared C++ base class for consummers
- Module IDs: Login=10000, Game=20000, Center=30000, Gate=40000, DBMain=50000 (all `IS_MAIN_DLL`)
- Export symbols must be exactly `InitDllServant` and `GetConsummer` (see `ModuleMgr.cpp`)
- DLL output must land next to `GameApp` (`LIBRARY_OUTPUT_PATH` / `bin/<platform>/<Debug|Release>`)
- Linux so name: `libModuleXxx.so`; Windows: `ModuleXxx.dll`
- No business protocols, auth, DB I/O, or server-identity handshake this plan
- Paths relative to CMake project root `cpp/` (repo: `cpp/...` under `/mnt/e/Practice/nany`)
- Prefer Chinese log messages only where existing code already uses Chinese; new logs may be English or Chinese consistently with nearby `Log::Info` style

## File Structure

| Path | Responsibility |
|------|----------------|
| `include/Interface/IGameApp.h` | Add `GetServerID` / `GetServerType` / `GetServerName` / `GetBinPath` for modules |
| `include/GameApp/GameApp.h` + `source/GameApp/GameApp.cpp` | Implement new `IGameApp` methods; call `LoadDll` in `OnInit` |
| `include/GameApp/ModuleMgr.h` + `source/GameApp/ModuleMgr.cpp` | Set `m_strBinPath` from `GetBinPath()` before open |
| `data/devops/etc/process_module.toml` | Add `[DBMain]` ModuleDBMain = [50000, 1] |
| `source/module/cmake/NanyServerModule.cmake` | CMake helper `nany_add_server_module` (build only; no C++ share) |
| `source/module/ModuleLogin/*` | Login skeleton DLL |
| `source/module/ModuleGame/*` | Game skeleton DLL |
| `source/module/ModuleCenter/*` | Center skeleton DLL |
| `source/module/ModuleGate/*` | Gate skeleton DLL |
| `source/module/ModuleDBMain/*` | DBMain skeleton DLL |
| `CMakeLists.txt` | `add_subdirectory` for five modules |
| `source/GameApp/NetComp/CNetComp.cpp` | Local+Public `StartService`; implement Add/DelServerConnect |
| `source/GameApp/NetComp/CGameConnServer.cpp` | Notify ModuleMgr + NetComp on connect/disconnect |
| `test/GameApp/test_ProcessModuleToml.cpp` | Lock process_module.toml shape for all five types |

---

### Task 1: IGameApp accessors + process_module.toml + unit test

**Files:**
- Modify: `include/Interface/IGameApp.h`
- Modify: `include/GameApp/GameApp.h`
- Modify: `source/GameApp/GameApp.cpp`
- Modify: `data/devops/etc/process_module.toml`
- Create: `test/GameApp/test_ProcessModuleToml.cpp`
- Modify: `test/GameApp/CMakeLists.txt` — add `test_ProcessModuleToml.cpp` to `SOURCES` and define `NANY_SOURCE_DIR`

**Interfaces:**
- Consumes: existing `CGameApp` fields `m_nServerID`, `m_strServerType`, `m_strServerName`; `CBaseApp::GetExePath()`
- Produces:
  - `virtual uint32 GetServerID() const = 0`
  - `virtual const std::string& GetServerType() const = 0`
  - `virtual const std::string& GetServerName() const = 0`
  - `virtual const char* GetBinPath() = 0` (non-const OK; may call `GetExePath()` to fill cache)
  - `process_module.toml` section `[DBMain]` with `ModuleDBMain = [50000, 1]`

- [ ] **Step 1: Write the failing test**

Create `test/GameApp/test_ProcessModuleToml.cpp`:

```cpp
#include <gtest/gtest.h>
#include "toml++/toml.hpp"
#include <filesystem>

namespace fs = std::filesystem;

static fs::path ProcessModuleTomlPath() {
    // Prefer source-tree config relative to binary or CMAKE define; fall back to known repo path.
    const char* szEnv = std::getenv("NANY_PROCESS_MODULE_TOML");
    if (szEnv && *szEnv) {
        return fs::path(szEnv);
    }
    return fs::path(NANY_SOURCE_DIR) / "data/devops/etc/process_module.toml";
}

TEST(ProcessModuleToml_Unit, AllServerTypesHaveMainModule) {
    auto path = ProcessModuleTomlPath();
    ASSERT_TRUE(fs::exists(path)) << path;
    toml::table tbl = toml::parse_file(path.string());

    struct Expect {
        const char* section;
        const char* moduleName;
        int64_t moduleId;
    };
    const Expect expects[] = {
        {"Login", "ModuleLogin", 10000},
        {"Game", "ModuleGame", 20000},
        {"Center", "ModuleCenter", 30000},
        {"Gate", "ModuleGate", 40000},
        {"DBMain", "ModuleDBMain", 50000},
    };

    for (const auto& e : expects) {
        ASSERT_TRUE(tbl.contains(e.section)) << e.section;
        auto* section = tbl[e.section].as_table();
        ASSERT_NE(nullptr, section);
        ASSERT_TRUE(section->contains(e.moduleName)) << e.moduleName;
        auto* arr = section->at(e.moduleName).as_array();
        ASSERT_NE(nullptr, arr);
        ASSERT_GE(arr->size(), 2u);
        EXPECT_EQ(e.moduleId, arr->get(0)->value_or(int64_t{0}));
        EXPECT_EQ(1, arr->get(1)->value_or(int64_t{0}));
        EXPECT_EQ(0, e.moduleId % 10000); // IS_MAIN_DLL
    }
}
```

If `NANY_SOURCE_DIR` is not already defined for `test_GameApp`, update `test/GameApp/CMakeLists.txt` to:

```cmake
gamma_add_gtest(test_GameApp
  SOURCES test_TomlConfig.cpp test_ProcessModuleToml.cpp
)
target_compile_definitions(test_GameApp PRIVATE NANY_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
```

(If `gamma_add_gtest` already creates the target, the `target_compile_definitions` line must come after the macro call.)

- [ ] **Step 2: Run test to verify it fails**

```bash
cmake --build build/linux --target test_GameApp
ctest --test-dir build/linux -R ProcessModuleToml_Unit -V
```

Expected: FAIL — `[DBMain]` missing (or file parse assertion).

- [ ] **Step 3: Extend IGameApp + CGameApp; update process_module.toml**

In `include/Interface/IGameApp.h`, add pure virtuals inside `IGameApp`:

```cpp
virtual uint32 GetServerID() const = 0;
virtual const std::string& GetServerType() const = 0;
virtual const std::string& GetServerName() const = 0;
virtual const char* GetBinPath() = 0;
```

Add `#include <string>` if not already present.

In `include/GameApp/GameApp.h`, change existing getters to `override` and add `GetBinPath`:

```cpp
uint32 GetServerID() const override { return m_nServerID; }
const std::string& GetServerName() const override { return m_strServerName; }
const std::string& GetServerType() const override { return m_strServerType; }
const char* GetBinPath() override;
```

In `source/GameApp/GameApp.cpp`:

```cpp
const char* CGameApp::GetBinPath() {
    // GetExePath caches into CBaseApp::m_strBinPath
    static_cast<void>(GetExePath());
    return CBaseApp::GetBinPath();
}
```

Update `data/devops/etc/process_module.toml` to:

```toml
# module_name = [module_id, module_version]
[Login]
ModuleLogin = [10000, 1]

[Game]
ModuleGame = [20000, 1]

[Center]
ModuleCenter = [30000, 1]

[Gate]
ModuleGate = [40000, 1]

[DBMain]
ModuleDBMain = [50000, 1]
```

- [ ] **Step 4: Run test to verify it passes**

```bash
cmake --build build/linux --target test_GameApp
ctest --test-dir build/linux -R ProcessModuleToml_Unit -V
```

Expected: PASS. Also rebuild `GameApp` to ensure `IGameApp` pure virtuals compile.

- [ ] **Step 5: Commit**

```bash
git add include/Interface/IGameApp.h include/GameApp/GameApp.h source/GameApp/GameApp.cpp \
  data/devops/etc/process_module.toml test/GameApp/test_ProcessModuleToml.cpp test/GameApp/CMakeLists.txt
git commit -m "$(cat <<'EOF'
feat(GameApp): expose server identity on IGameApp; add DBMain module config

Modules need ServerID/Type/BinPath via IGameApp; process_module.toml gains DBMain.
EOF
)"
```

---

### Task 2: CMake helper + ModuleLogin SHARED library

**Files:**
- Create: `source/module/cmake/NanyServerModule.cmake`
- Create: `source/module/ModuleLogin/CMakeLists.txt`
- Create: `source/module/ModuleLogin/ModuleLogin.h`
- Create: `source/module/ModuleLogin/ModuleLogin.cpp`
- Create: `source/module/ModuleLogin/ModuleExport.cpp`
- Modify: `CMakeLists.txt` (root) — add `add_subdirectory(source/module/ModuleLogin)`

**Interfaces:**
- Consumes: `IConsummer`, `DEFINE_MODULE`, `IGameApp` / `GetGameApp`, `Log::Info`
- Produces: target `ModuleLogin` → `libModuleLogin.so` / `ModuleLogin.dll` exporting:
  - `void InitDllServant(IGameApp* pApp)`
  - `void GetConsummer(IConsummerPtr& out)`

- [ ] **Step 1: Add CMake helper**

Create `source/module/cmake/NanyServerModule.cmake`:

```cmake
# nany_add_server_module(<TargetName>)
# Expects sources in CMAKE_CURRENT_SOURCE_DIR; produces SHARED lib next to GameApp.
function(nany_add_server_module TargetName)
    file(GLOB _srcs CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
    add_library(${TargetName} SHARED ${_srcs})
    target_include_directories(${TargetName} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include/gamma
        ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${TargetName} PRIVATE GammaCommon GammaApp)
    set_target_properties(${TargetName} PROPERTIES
        FOLDER "module"
        OUTPUT_NAME "${TargetName}"
        PREFIX "lib" # Linux: libModuleLogin.so; on MSVC PREFIX is often ignored for SHARED — verify
    )
    if(WIN32)
        set_target_properties(${TargetName} PROPERTIES PREFIX "")
    endif()
    # Ensure same output dirs as root project already set via LIBRARY_OUTPUT_PATH
endfunction()
```

Note: On Linux CMake default PREFIX is already `lib`. Prefer:

```cmake
if(WIN32)
  set_target_properties(${TargetName} PROPERTIES PREFIX "")
endif()
```

and do **not** force PREFIX `lib` globally if it doubles the prefix.

- [ ] **Step 2: Implement ModuleLogin sources**

`ModuleLogin.h`:

```cpp
#pragma once
#include "Interface/IConsummer.h"

class CModuleLogin : public IConsummer {
public:
    DEFINE_MODULE(10000);
    CModuleLogin() = default;
    ~CModuleLogin() override = default;

    void OnInit() override;
    void UnInit() override;
    int OnTimer(uint32 nInterval) override;
    void OnServerConnect(uint16 nServerID) override;
    void OnServerDisConnect(uint16 nServerID) override;
    bool OnServerStop() override;
};
```

`ModuleLogin.cpp`:

```cpp
#include "ModuleLogin.h"
#include "Interface/IGameApp.h"
#include "GammaCommon/ILog.h"

void CModuleLogin::OnInit() {
    auto pApp = GetGameApp();
    Log::Info("ModuleLogin::OnInit moduleId={} serverName={} serverType={} serverId={}",
              GetModuleID(),
              pApp ? pApp->GetServerName() : std::string{},
              pApp ? pApp->GetServerType() : std::string{},
              pApp ? pApp->GetServerID() : 0u);
}

void CModuleLogin::UnInit() {
    Log::Info("ModuleLogin::UnInit");
}

int CModuleLogin::OnTimer(uint32 /*nInterval*/) {
    return 0;
}

void CModuleLogin::OnServerConnect(uint16 nServerID) {
    Log::Info("ModuleLogin::OnServerConnect serverId={}", nServerID);
}

void CModuleLogin::OnServerDisConnect(uint16 nServerID) {
    Log::Info("ModuleLogin::OnServerDisConnect serverId={}", nServerID);
}

bool CModuleLogin::OnServerStop() {
    Log::Info("ModuleLogin::OnServerStop");
    return true;
}
```

`ModuleExport.cpp`:

```cpp
#include "ModuleLogin.h"
#include "Interface/IGameApp.h"

#if defined(_WIN32)
#define MODULE_EXPORT extern "C" __declspec(dllexport)
#else
#define MODULE_EXPORT extern "C" __attribute__((visibility("default")))
#endif

static IConsummerPtr g_pConsummer;

MODULE_EXPORT void InitDllServant(IGameApp* /*pApp*/) {
    // GameApp already SetGameApp; pApp retained for future use.
}

MODULE_EXPORT void GetConsummer(IConsummerPtr& out) {
    if (!g_pConsummer) {
        g_pConsummer = new CModuleLogin();
    }
    out = g_pConsummer;
}
```

`source/module/ModuleLogin/CMakeLists.txt`:

```cmake
include(${CMAKE_SOURCE_DIR}/source/module/cmake/NanyServerModule.cmake)
nany_add_server_module(ModuleLogin)
```

Root `CMakeLists.txt` — after `add_subdirectory(source/GameApp)` add:

```cmake
add_subdirectory(source/module/ModuleLogin)
```

- [ ] **Step 3: Build and verify export symbols**

```bash
cmake --build build/linux --target ModuleLogin
nm -D bin/linux/Debug/libModuleLogin.so | grep -E 'InitDllServant|GetConsummer'
```

Expected: both symbols present (T or T-like). Adjust build type path if Release.

- [ ] **Step 4: Commit**

```bash
git add source/module/cmake/NanyServerModule.cmake source/module/ModuleLogin CMakeLists.txt
git commit -m "$(cat <<'EOF'
feat(module): add ModuleLogin shared library skeleton

First independent server module DLL with IConsummer exports.
EOF
)"
```

---

### Task 3: ModuleCenter, ModuleGame, ModuleGate, ModuleDBMain

**Files:**
- Create: `source/module/ModuleCenter/{CMakeLists.txt,ModuleCenter.h,ModuleCenter.cpp,ModuleExport.cpp}`
- Create: `source/module/ModuleGame/{CMakeLists.txt,ModuleGame.h,ModuleGame.cpp,ModuleExport.cpp}`
- Create: `source/module/ModuleGate/{CMakeLists.txt,ModuleGate.h,ModuleGate.cpp,ModuleExport.cpp}`
- Create: `source/module/ModuleDBMain/{CMakeLists.txt,ModuleDBMain.h,ModuleDBMain.cpp,ModuleExport.cpp}`
- Modify: `CMakeLists.txt` — four more `add_subdirectory` lines

**Interfaces:**
- Consumes: same as Task 2 (`IConsummer`, exports)
- Produces: targets `ModuleCenter` (30000), `ModuleGame` (20000), `ModuleGate` (40000), `ModuleDBMain` (50000)

- [ ] **Step 1: ModuleCenter**

`ModuleCenter.h` — same shape as Login with `DEFINE_MODULE(30000)` and class `CModuleCenter`.

`ModuleCenter.cpp` — same hooks, log prefix `ModuleCenter::`.

`ModuleExport.cpp`:

```cpp
#include "ModuleCenter.h"
#include "Interface/IGameApp.h"

#if defined(_WIN32)
#define MODULE_EXPORT extern "C" __declspec(dllexport)
#else
#define MODULE_EXPORT extern "C" __attribute__((visibility("default")))
#endif

static IConsummerPtr g_pConsummer;

MODULE_EXPORT void InitDllServant(IGameApp* /*pApp*/) {}

MODULE_EXPORT void GetConsummer(IConsummerPtr& out) {
    if (!g_pConsummer) {
        g_pConsummer = new CModuleCenter();
    }
    out = g_pConsummer;
}
```

`CMakeLists.txt`:

```cmake
include(${CMAKE_SOURCE_DIR}/source/module/cmake/NanyServerModule.cmake)
nany_add_server_module(ModuleCenter)
```

- [ ] **Step 2: ModuleGame**

Identical pattern: class `CModuleGame`, `DEFINE_MODULE(20000)`, logs `ModuleGame::`, export creates `CModuleGame`, CMake `nany_add_server_module(ModuleGame)`.

- [ ] **Step 3: ModuleGate**

Identical pattern: class `CModuleGate`, `DEFINE_MODULE(40000)`, logs `ModuleGate::`, export creates `CModuleGate`, CMake `nany_add_server_module(ModuleGate)`.

- [ ] **Step 4: ModuleDBMain**

Identical pattern: class `CModuleDBMain`, `DEFINE_MODULE(50000)`, logs `ModuleDBMain::`, export creates `CModuleDBMain`, CMake `nany_add_server_module(ModuleDBMain)`.

- [ ] **Step 5: Register in root CMakeLists.txt**

```cmake
add_subdirectory(source/module/ModuleLogin)
add_subdirectory(source/module/ModuleGame)
add_subdirectory(source/module/ModuleCenter)
add_subdirectory(source/module/ModuleGate)
add_subdirectory(source/module/ModuleDBMain)
```

- [ ] **Step 6: Build all modules**

```bash
cmake --build build/linux --target ModuleLogin ModuleGame ModuleCenter ModuleGate ModuleDBMain
ls bin/linux/Debug/libModule*.so
```

Expected: five `.so` files present.

- [ ] **Step 7: Commit**

```bash
git add source/module/ModuleCenter source/module/ModuleGame source/module/ModuleGate \
  source/module/ModuleDBMain CMakeLists.txt
git commit -m "$(cat <<'EOF'
feat(module): add Center/Game/Gate/DBMain skeleton DLLs

Complete the five independent main-module shared libraries.
EOF
)"
```

---

### Task 4: ModuleMgr bin path + GameApp LoadDll

**Files:**
- Modify: `source/GameApp/ModuleMgr.cpp`
- Modify: `source/GameApp/GameApp.cpp` (`OnInit`)

**Interfaces:**
- Consumes: `IGameApp::GetBinPath()`, `m_strServerType`, `process_module.toml` path under `m_strEtcPath`
- Produces: successful `LoadDll(serverType, toml)` so `OnInit` of the matching module runs at process start

- [ ] **Step 1: Set m_strBinPath in LoadDll**

At the start of `CModuleMgr::LoadDll(const char *strSettings, const std::string &strTomlFile)`, after `GetGameApp()` null check:

```cpp
auto pGameApp = GetGameApp();
if (pGameApp == nullptr) {
    return false;
}
m_strBinPath = pGameApp->GetBinPath();
if (m_strBinPath.empty()) {
    Log::Error("CModuleMgr::LoadDll bin path is empty");
    return false;
}
```

Ensure trailing slash behavior matches `GetDynamicFileName` (`{bin}/{name}.dll` / `{bin}/lib{name}.so`). If `GetExePath()` already returns a path ending with `/`, keep as-is; if not, append `/`:

```cpp
if (!m_strBinPath.empty() && m_strBinPath.back() != '/') {
    m_strBinPath.push_back('/');
}
```

(Inspect `CBaseApp::GetExePath` — it keeps the trailing slash when `strrchr` finds `/`. Match that.)

- [ ] **Step 2: Enable LoadDll in CGameApp::OnInit**

Replace the commented load with:

```cpp
bool CGameApp::OnInit() {
    if (!LoadServerInfo()) {
        return false;
    }
    fs::path etcPath = m_strEtcPath;
    std::string strProcessModuleConfig = (etcPath / "process_module.toml").string();
    if (!CModuleMgr::Instance()->LoadDll(m_strServerType.c_str(), strProcessModuleConfig)) {
        Log::Error("CGameApp::OnInit LoadDll failed for type {}", m_strServerType);
        return false;
    }
    return true;
}
```

Include `GameApp/ModuleMgr.h` if not already included.

- [ ] **Step 3: Manual smoke — Center loads ModuleCenter**

```bash
cmake --build build/linux --target GameApp ModuleCenter
# from repo; adjust paths
./bin/linux/Debug/GameApp --devops /mnt/e/Practice/nany/cpp/data/devops --name Center
```

Expected log contains `ModuleCenter::OnInit` with `serverType=Center` and `serverId=0` (or configured ServerID). Ctrl+C to stop.

If LoadDll fails with "open dynamic file", print `m_strBinPath` and confirm `libModuleCenter.so` sits beside `GameApp`.

- [ ] **Step 4: Commit**

```bash
git add source/GameApp/ModuleMgr.cpp source/GameApp/GameApp.cpp
git commit -m "$(cat <<'EOF'
feat(GameApp): load process module DLL on init

Wire CModuleMgr bin path and enable LoadDll by server type.
EOF
)"
```

---

### Task 5: CNetComp listen + server connection tree + ModuleMgr callbacks

**Files:**
- Modify: `source/GameApp/NetComp/CNetComp.cpp`
- Modify: `source/GameApp/NetComp/CGameConnServer.cpp`
- Modify: `source/GameApp/NetComp/CGameConnServer.h` (if need NetComp friend / helper — prefer GetComp)

**Interfaces:**
- Consumes: `IConnectionMgr::StartService`, `CGameConnServer::GetID()`, `CGameConnFromClient::GetID()`, `CModuleMgr::OnServerConnect/DisConnect`, `m_treeServer.Insert` / `Remove`
- Produces:
  - LocalPort always listened when `> 0`
  - PublicPort listened when `> 0` with client conn class
  - Outbound connects registered in `m_treeServer` so reconnect tick does not spam
  - Module `OnServerConnect` / `OnServerDisConnect` called when ServerID known

- [ ] **Step 1: Implement AddServerConnect / DelServerConnect**

In `CNetComp.cpp`:

```cpp
void CNetComp::AddServerConnect(CGameConnServer* pServerConn) {
    if (!pServerConn) {
        return;
    }
    if (pServerConn->IsInTree()) {
        return;
    }
    m_treeServer.Insert(*pServerConn);
    Log::Info("CNetComp::AddServerConnect serverId={}", pServerConn->GetConnectID());
}

void CNetComp::DelServerConnect(CGameConnServer* pServerConn) {
    if (!pServerConn) {
        return;
    }
    if (!pServerConn->IsInTree()) {
        return;
    }
    pServerConn->Remove();
    Log::Info("CNetComp::DelServerConnect serverId={}", pServerConn->GetConnectID());
}
```

Confirm `IsInTree()` / `Remove()` exist on `CGammaRBTreeNode` (they do in `TGammaRBTree.h`). If `IsInTree` is protected, call `Remove()` only when Find returns the same pointer, or make a thin public wrapper on `CGameConnServer`.

Also stub client helpers if linker requires them (declared in header):

```cpp
void CNetComp::AddClientConnect(CGameConnFromClient* pFromClientConn) {
    if (!pFromClientConn || pFromClientConn->IsInTree()) {
        return;
    }
    m_treeFromClient.Insert(*pFromClientConn);
}

void CNetComp::DelClientConnect(CGameConnFromClient* pFromClientConn) {
    if (!pFromClientConn || !pFromClientConn->IsInTree()) {
        return;
    }
    pFromClientConn->Remove();
}
```

(`DelClientConnect` is declared in header — implement it.)

- [ ] **Step 2: Fix OnStarted dual listen**

Replace `CNetComp::OnStarted` body with:

```cpp
void CNetComp::OnStarted() {
    auto pConnMgr = GetGameApp()->GetConnMgr();
    if (!m_pCurServerInfo) {
        Log::Error("CNetComp::OnStarted current server info is null");
        return;
    }

    if (m_pCurServerInfo->nLocalPort > 0) {
        pConnMgr->StartService(m_pCurServerInfo->strLocalHost.c_str(),
                               m_pCurServerInfo->nLocalPort,
                               CGameConnServer::GetID());
        Log::Info("CNetComp::OnStarted Local StartService {}:{}",
                  m_pCurServerInfo->strLocalHost, m_pCurServerInfo->nLocalPort);
    }

    if (m_pCurServerInfo->nPublicPort > 0) {
        pConnMgr->StartService(m_pCurServerInfo->strPublicHost.c_str(),
                               m_pCurServerInfo->nPublicPort,
                               CGameConnFromClient::GetID());
        Log::Info("CNetComp::OnStarted Public StartService {}:{}",
                  m_pCurServerInfo->strPublicHost, m_pCurServerInfo->nPublicPort);
    }

    GetGameApp()->Register(&m_ReconnectTick, 1000, 0);
}
```

Include `CGameConnFromClient.h` in `CNetComp.cpp`.

- [ ] **Step 3: Register outbound connect into tree in OnReconnectTick**

After successful `Connect` + `SetServerID`:

```cpp
pServerConn = static_cast<CGameConnServer*>(pBase);
pServerConn->SetServerID(nServerID);
AddServerConnect(pServerConn);
```

- [ ] **Step 4: Wire CGameConnServer callbacks to ModuleMgr + NetComp**

In `CGameConnServer.cpp` (`IGameApp` has no `GetComp`; use `CBaseApp`):

```cpp
#include "GameApp/ModuleMgr.h"
#include "CNetComp.h"
#include "GammaApp/CBaseApp.h"

static CNetComp* GetNetComp() {
    CBaseApp* pApp = CBaseApp::Inst();
    if (!pApp) {
        return nullptr;
    }
    return static_cast<CNetComp*>(pApp->GetComp(CNetComp::GetID()));
}
```

`OnConnected`:

```cpp
void CGameConnServer::OnConnected() {
    // existing GammaLog lines...
    if (m_nServerID != 0) {
        if (auto* pNet = GetNetComp()) {
            pNet->AddServerConnect(this);
        }
        CModuleMgr::Instance()->OnServerConnect(static_cast<uint16>(m_nServerID));
    }
}
```

`OnDisConnect`:

```cpp
void CGameConnServer::OnDisConnect() {
    // existing GammaLog lines...
    if (m_nServerID != 0) {
        CModuleMgr::Instance()->OnServerDisConnect(static_cast<uint16>(m_nServerID));
        if (auto* pNet = GetNetComp()) {
            pNet->DelServerConnect(this);
        }
    }
}
```

**Avoid double-insert:** If `OnReconnectTick` already `AddServerConnect`, `OnConnected` must no-op when `IsInTree()`. The `AddServerConnect` guard handles this.

**Public API:** `AddServerConnect` / `DelServerConnect` are currently private. Either:
- make them public, or
- add `public:` methods `NotifyServerConnected(CGameConnServer*)` / `NotifyServerDisconnected(CGameConnServer*)`.

Plan choice: move `AddServerConnect` / `DelServerConnect` to `public:` in `CNetComp.h`.

- [ ] **Step 5: Build GameApp**

```bash
cmake --build build/linux --target GameApp
```

Expected: success.

- [ ] **Step 6: Two-process smoke (Center then Login)**

Terminal A:

```bash
./bin/linux/Debug/GameApp --devops /mnt/e/Practice/nany/cpp/data/devops --name Center
```

Expect: `Local StartService 127.0.0.1:8006`, `ModuleCenter::OnInit`.

Terminal B:

```bash
./bin/linux/Debug/GameApp --devops /mnt/e/Practice/nany/cpp/data/devops --name Login
```

Expect: `ModuleLogin::OnInit`, reconnect attempts toward Center (and DBMain if up); ideally `ModuleLogin::OnServerConnect` when TCP succeeds. If DBMain is down, Center connect may still succeed alone depending on topology timing — Center must be listening.

Optional Terminal C: start `DBMain` first if Login must reach both.

- [ ] **Step 7: Commit**

```bash
git add source/GameApp/NetComp/CNetComp.h source/GameApp/NetComp/CNetComp.cpp \
  source/GameApp/NetComp/CGameConnServer.cpp
git commit -m "$(cat <<'EOF'
fix(NetComp): listen LocalPort and track server connections

Enable topology listen/reconnect and forward connect events to modules.
EOF
)"
```

---

### Task 6: Full acceptance checklist

**Files:** none required (verification only); fix bugs found in Tasks 1–5 if any.

- [ ] **Step 1: Rebuild all targets**

```bash
cmake --build build/linux --target GameApp ModuleLogin ModuleGame ModuleCenter ModuleGate ModuleDBMain test_GameApp
ctest --test-dir build/linux -R 'ProcessModuleToml_Unit|GameAppTomlConfig_Unit' --output-on-failure
```

Expected: all PASS.

- [ ] **Step 2: Per-type OnInit smoke**

For each `--name` in `Center`, `Login`, `DBMain`, `Game01`, `Gate01`, start briefly and confirm matching `ModuleXxx::OnInit` in logs. Stop after ~2s each.

- [ ] **Step 3: Dual-port smoke for Game01**

Start `Game01`; expect both Local `8003` and Public `8002` `StartService` logs.

- [ ] **Step 4: Final commit only if fixes were needed**

If no code changes, skip commit. If fixes landed, commit with message describing the fix.

---

## Spec Coverage Checklist

| Spec requirement | Task |
|------------------|------|
| Five independent DLLs | 2, 3 |
| `InitDllServant` / `GetConsummer` | 2, 3 |
| Lifecycle log hooks | 2, 3 |
| `[DBMain]` in process_module.toml | 1 |
| `GameApp` LoadDll by type | 4 |
| `m_strBinPath` set | 4 |
| LocalPort StartService | 5 |
| PublicPort StartService | 5 |
| ModuleMgr connect callbacks | 5 |
| Acceptance / smoke | 6 |
| No business protocols | (out of scope — no task) |

## Self-Review Notes

- No shared ModuleCommon C++ library (CMake helper only).
- `AddServerConnect` was declared but unimplemented — required for reconnect tick; included in Task 5 (necessary for Spec §5.2 / reconnect behavior).
- PassiveHost factory: Local → `CGameConnServer`, Public → `CGameConnFromClient` (matches Spec §5.1).
- Passive handshake for passive peers deferred (Spec non-goal).
