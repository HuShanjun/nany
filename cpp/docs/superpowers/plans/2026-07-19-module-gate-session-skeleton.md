# ModuleGate Session Skeleton Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add Gate client/server session tables, wire client connect events from `CGameConnFromClient` through `CNetComp`/`CModuleMgr` into `ModuleGate`, and provide bind/forward stubs that log without sending.

**Architecture:** Extract pure `CGateSessionStore` for session maps (unit-tested). Extend `IConsummer` with default-empty client hooks; `CModuleMgr` broadcasts like server hooks. `ModuleGate` owns a store and implements hooks + stub forwards. `CGameConnFromClient` notifies NetComp + ModuleMgr.

**Tech Stack:** C++17/23, GoogleTest, existing GameApp/ModuleGate SHARED module, toml/config unchanged.

**Spec:** `docs/superpowers/specs/2026-07-19-module-gate-session-skeleton-design.md`

## Global Constraints

- Scope: session tables + event path + forward stubs only — **no** real `SendShellMsg`, **no** CGNetwork::Check fix, **no** auth/protocol
- `OnClientConnect` / `OnClientDisConnect` on `IConsummer` with **default empty** bodies
- `nClientID == 0` ignored by ModuleMgr (same pattern as `nServerID == 0`)
- Forward stub: missing target → log Error + `false`; present → log Info + `true`, no network send
- Runtime TCP smoke **not required**
- Paths relative to CMake project root `cpp/`
- Prefer Chinese or English logs consistent with nearby `Log::Info` style

## File Structure

| Path | Responsibility |
|------|----------------|
| `source/module/ModuleGate/CGateSessionStore.h` | Session store API + `SGateClientSession` |
| `source/module/ModuleGate/CGateSessionStore.cpp` | Map/set logic (no Log dependency preferred) |
| `source/module/ModuleGate/ModuleGate.h/.cpp` | Consume store; hooks; stub Forward* with logs |
| `include/Interface/IConsummer.h` | Client hooks |
| `include/GameApp/ModuleMgr.h` + `ModuleMgr.cpp` | Broadcast OnClient* |
| `source/GameApp/NetComp/CGameConnFromClient.cpp` | Wire Add/Del + ModuleMgr |
| `source/GameApp/NetComp/CNetComp.cpp` | Dedup on AddClientConnect |
| `test/ModuleGate/CMakeLists.txt` + `test_GateSessionStore.cpp` | Unit tests |
| `test/CMakeLists.txt` | `add_subdirectory(ModuleGate)` |

---

### Task 1: CGateSessionStore + unit tests (TDD)

**Files:**
- Create: `source/module/ModuleGate/CGateSessionStore.h`
- Create: `source/module/ModuleGate/CGateSessionStore.cpp`
- Create: `test/ModuleGate/CMakeLists.txt`
- Create: `test/ModuleGate/test_GateSessionStore.cpp`
- Modify: `test/CMakeLists.txt` — add `add_subdirectory(ModuleGate)`
- Modify: `source/module/ModuleGate/CMakeLists.txt` — ensure store `.cpp` is picked up by existing GLOB (it should be automatic)

**Interfaces:**
- Consumes: `<cstdint>`, `<map>`, `<set>`, project `uint32` / `int64` via `GammaCommon/GammaCommonType.h` **or** use `uint32_t`/`int64_t` in the store to keep the test free of Gamma — prefer `uint32_t`/`int64_t` in the store for easy testing
- Produces:
  - `struct SGateClientSession { uint32_t nClientID; uint32_t nBoundServerID; int64_t nConnectTime; }`
  - `class CGateSessionStore` with:
    - `void AddClient(uint32_t nClientID, int64_t nConnectTime);`
    - `void RemoveClient(uint32_t nClientID);`
    - `bool HasClient(uint32_t nClientID) const;`
    - `SGateClientSession* FindClient(uint32_t nClientID);` / `const` overload
    - `void AddServer(uint32_t nServerID);`
    - `void RemoveServer(uint32_t nServerID);` // clears bindings to that server
    - `bool HasServer(uint32_t nServerID) const;`
    - `bool BindClientToServer(uint32_t nClientID, uint32_t nServerID);`
    - `bool CanForwardToServer(uint32_t nClientID, uint32_t nServerID) const;`
    - `bool CanForwardToClient(uint32_t nClientID) const;`

- [ ] **Step 1: Write failing tests**

Create `test/ModuleGate/test_GateSessionStore.cpp`:

```cpp
#include <gtest/gtest.h>
#include "CGateSessionStore.h"

TEST(GateSessionStore_Unit, AddRemoveClient) {
    CGateSessionStore store;
    store.AddClient(1, 100);
    ASSERT_TRUE(store.HasClient(1));
    auto* s = store.FindClient(1);
    ASSERT_NE(nullptr, s);
    EXPECT_EQ(1u, s->nClientID);
    EXPECT_EQ(0u, s->nBoundServerID);
    EXPECT_EQ(100, s->nConnectTime);
    store.RemoveClient(1);
    EXPECT_FALSE(store.HasClient(1));
}

TEST(GateSessionStore_Unit, BindRequiresClientAndServer) {
    CGateSessionStore store;
    EXPECT_FALSE(store.BindClientToServer(1, 200));
    store.AddClient(1, 0);
    EXPECT_FALSE(store.BindClientToServer(1, 200));
    store.AddServer(200);
    EXPECT_TRUE(store.BindClientToServer(1, 200));
    EXPECT_EQ(200u, store.FindClient(1)->nBoundServerID);
}

TEST(GateSessionStore_Unit, RemoveServerClearsBindings) {
    CGateSessionStore store;
    store.AddClient(1, 0);
    store.AddClient(2, 0);
    store.AddServer(200);
    store.BindClientToServer(1, 200);
    store.BindClientToServer(2, 200);
    store.RemoveServer(200);
    EXPECT_FALSE(store.HasServer(200));
    EXPECT_EQ(0u, store.FindClient(1)->nBoundServerID);
    EXPECT_EQ(0u, store.FindClient(2)->nBoundServerID);
}

TEST(GateSessionStore_Unit, CanForwardChecks) {
    CGateSessionStore store;
    EXPECT_FALSE(store.CanForwardToClient(1));
    EXPECT_FALSE(store.CanForwardToServer(1, 200));
    store.AddClient(1, 0);
    store.AddServer(200);
    EXPECT_TRUE(store.CanForwardToClient(1));
    EXPECT_TRUE(store.CanForwardToServer(1, 200));
}
```

`test/ModuleGate/CMakeLists.txt`:

```cmake
gamma_add_gtest(test_ModuleGate
  SOURCES test_GateSessionStore.cpp
          ${CMAKE_SOURCE_DIR}/source/module/ModuleGate/CGateSessionStore.cpp
)
target_include_directories(test_ModuleGate PRIVATE
  ${CMAKE_SOURCE_DIR}/source/module/ModuleGate
)
```

In `test/CMakeLists.txt` add: `add_subdirectory(ModuleGate)`

- [ ] **Step 2: Run tests — expect fail (missing header/link)**

```bash
cmake --build build/linux --target test_ModuleGate
```

Expected: compile fail — `CGateSessionStore.h` missing.

- [ ] **Step 3: Implement CGateSessionStore**

`CGateSessionStore.h`:

```cpp
#pragma once
#include <cstdint>
#include <map>
#include <set>

struct SGateClientSession {
    uint32_t nClientID = 0;
    uint32_t nBoundServerID = 0;
    int64_t nConnectTime = 0;
};

class CGateSessionStore {
public:
    void AddClient(uint32_t nClientID, int64_t nConnectTime);
    void RemoveClient(uint32_t nClientID);
    bool HasClient(uint32_t nClientID) const;
    SGateClientSession* FindClient(uint32_t nClientID);
    const SGateClientSession* FindClient(uint32_t nClientID) const;

    void AddServer(uint32_t nServerID);
    void RemoveServer(uint32_t nServerID);
    bool HasServer(uint32_t nServerID) const;

    bool BindClientToServer(uint32_t nClientID, uint32_t nServerID);
    bool CanForwardToServer(uint32_t nClientID, uint32_t nServerID) const;
    bool CanForwardToClient(uint32_t nClientID) const;

private:
    std::map<uint32_t, SGateClientSession> m_mapClients;
    std::set<uint32_t> m_setServers;
};
```

`CGateSessionStore.cpp` — implement as specified:
- `AddClient`: insert/overwrite session with given id/time, `nBoundServerID=0` on new; on overwrite keep or reset bind — **spec: 已存在则覆盖** → reset session fields including `nBoundServerID=0` and new time
- `RemoveServer`: erase server; foreach client if `nBoundServerID==nServerID` set to 0
- `BindClientToServer`: require `HasClient` && `HasServer`; set bind; return true/false
- `CanForwardToServer`: `HasClient && HasServer`
- `CanForwardToClient`: `HasClient`

Ignore `nClientID==0` / `nServerID==0` in Add* (no-op) for safety.

- [ ] **Step 4: Run tests — expect pass**

```bash
cmake --build build/linux --target test_ModuleGate
ctest --test-dir build/linux -R GateSessionStore_Unit --output-on-failure
```

Expected: all PASS.

- [ ] **Step 5: Commit**

```bash
git add source/module/ModuleGate/CGateSessionStore.h source/module/ModuleGate/CGateSessionStore.cpp \
  test/ModuleGate test/CMakeLists.txt
git commit -m "$(cat <<'EOF'
feat(ModuleGate): add CGateSessionStore with unit tests

Pure session map/set logic for Gate client binding and forward checks.
EOF
)"
```

---

### Task 2: IConsummer client hooks + ModuleMgr broadcast

**Files:**
- Modify: `include/Interface/IConsummer.h`
- Modify: `include/GameApp/ModuleMgr.h`
- Modify: `source/GameApp/ModuleMgr.cpp`

**Interfaces:**
- Consumes: existing `OnServerConnect` loop pattern
- Produces:
  - `virtual void OnClientConnect(uint32 nClientID) {}`
  - `virtual void OnClientDisConnect(uint32 nClientID) {}`
  - `CModuleMgr::OnClientConnect(uint32)` / `OnClientDisConnect(uint32)`

- [ ] **Step 1: Add hooks to IConsummer.h**

After `OnServerDisConnect`:

```cpp
virtual void OnClientConnect(uint32 nClientID) {}
virtual void OnClientDisConnect(uint32 nClientID) {}
```

- [ ] **Step 2: Declare and implement ModuleMgr methods**

In `ModuleMgr.h` public section next to server methods:

```cpp
void OnClientConnect(uint32 nClientID);
void OnClientDisConnect(uint32 nClientID);
```

In `ModuleMgr.cpp` (mirror `OnServerConnect`, use `uint32`):

```cpp
void CModuleMgr::OnClientConnect(uint32 nClientID) {
    if (nClientID == 0)
        return;
    for (auto it : m_mapDll) {
        if (it.second->m_pConsummer != nullptr) {
            it.second->m_pConsummer->OnClientConnect(nClientID);
        }
    }
}

void CModuleMgr::OnClientDisConnect(uint32 nClientID) {
    if (nClientID == 0)
        return;
    for (auto it : m_mapDll) {
        if (it.second->m_pConsummer != nullptr) {
            it.second->m_pConsummer->OnClientDisConnect(nClientID);
        }
    }
}
```

- [ ] **Step 3: Build GameApp to verify compile**

```bash
cmake --build build/linux --target GameApp
```

Expected: success (default empty hooks — no module changes required yet).

- [ ] **Step 4: Commit**

```bash
git add include/Interface/IConsummer.h include/GameApp/ModuleMgr.h source/GameApp/ModuleMgr.cpp
git commit -m "$(cat <<'EOF'
feat(ModuleMgr): broadcast client connect/disconnect to modules

Extend IConsummer with default-empty OnClient* hooks.
EOF
)"
```

---

### Task 3: ModuleGate uses store + stubs

**Files:**
- Modify: `source/module/ModuleGate/ModuleGate.h`
- Modify: `source/module/ModuleGate/ModuleGate.cpp`

**Interfaces:**
- Consumes: `CGateSessionStore`, `GetGameApp()->GetCurTime()` (optional; if GetCurTime unavailable on IGameApp, use `0` or add call via existing `IGameApp::GetCurTime` — **it exists**)
- Produces: overrides `OnClient*` / `OnServer*`; methods `BindClientToServer`, `ForwardToServer`, `ForwardToClient`

- [ ] **Step 1: Update ModuleGate.h**

```cpp
#pragma once
#include "Interface/IConsummer.h"
#include "CGateSessionStore.h"

class CModuleGate : public IConsummer {
public:
    DEFINE_MODULE(40000);
    CModuleGate() = default;
    ~CModuleGate() override = default;

    void OnInit() override;
    void UnInit() override;
    int OnTimer(uint32 nInterval) override;
    void OnServerConnect(uint16 nServerID) override;
    void OnServerDisConnect(uint16 nServerID) override;
    void OnClientConnect(uint32 nClientID) override;
    void OnClientDisConnect(uint32 nClientID) override;
    bool OnServerStop() override;

    bool BindClientToServer(uint32 nClientID, uint32 nServerID);
    bool ForwardToServer(uint32 nClientID, uint32 nServerID, const void* pData, size_t nSize);
    bool ForwardToClient(uint32 nClientID, const void* pData, size_t nSize);

    CGateSessionStore& GetSessionStore() { return m_store; }

private:
    CGateSessionStore m_store;
};
```

- [ ] **Step 2: Implement ModuleGate.cpp hooks and stubs**

```cpp
void CModuleGate::OnClientConnect(uint32 nClientID) {
    int64_t t = 0;
    if (auto pApp = GetGameApp()) {
        t = pApp->GetCurTime();
    }
    m_store.AddClient(nClientID, t);
    Log::Info("ModuleGate::OnClientConnect clientId={}", nClientID);
}

void CModuleGate::OnClientDisConnect(uint32 nClientID) {
    m_store.RemoveClient(nClientID);
    Log::Info("ModuleGate::OnClientDisConnect clientId={}", nClientID);
}

void CModuleGate::OnServerConnect(uint16 nServerID) {
    m_store.AddServer(nServerID);
    Log::Info("ModuleGate::OnServerConnect serverId={}", nServerID);
}

void CModuleGate::OnServerDisConnect(uint16 nServerID) {
    m_store.RemoveServer(nServerID);
    Log::Info("ModuleGate::OnServerDisConnect serverId={}", nServerID);
}

bool CModuleGate::BindClientToServer(uint32 nClientID, uint32 nServerID) {
    bool ok = m_store.BindClientToServer(nClientID, nServerID);
    if (!ok) {
        Log::Error("ModuleGate::BindClientToServer failed clientId={} serverId={}", nClientID, nServerID);
        return false;
    }
    Log::Info("ModuleGate::BindClientToServer clientId={} serverId={}", nClientID, nServerID);
    return true;
}

bool CModuleGate::ForwardToServer(uint32 nClientID, uint32 nServerID, const void* /*pData*/, size_t nSize) {
    if (!m_store.CanForwardToServer(nClientID, nServerID)) {
        Log::Error("ModuleGate::ForwardToServer missing client/server clientId={} serverId={}", nClientID, nServerID);
        return false;
    }
    Log::Info("ModuleGate::ForwardToServer stub clientId={} serverId={} size={}", nClientID, nServerID, nSize);
    return true;
}

bool CModuleGate::ForwardToClient(uint32 nClientID, const void* /*pData*/, size_t nSize) {
    if (!m_store.CanForwardToClient(nClientID)) {
        Log::Error("ModuleGate::ForwardToClient missing clientId={}", nClientID);
        return false;
    }
    Log::Info("ModuleGate::ForwardToClient stub clientId={} size={}", nClientID, nSize);
    return true;
}
```

Keep existing `OnInit`/`UnInit`/`OnTimer`/`OnServerStop` log behavior.

- [ ] **Step 3: Build ModuleGate**

```bash
cmake --build build/linux --target ModuleGate
```

Expected: success.

- [ ] **Step 4: Commit**

```bash
git add source/module/ModuleGate/ModuleGate.h source/module/ModuleGate/ModuleGate.cpp
git commit -m "$(cat <<'EOF'
feat(ModuleGate): wire session store and forward stubs

Client/server hooks update CGateSessionStore; forwards log only.
EOF
)"
```

---

### Task 4: Wire CGameConnFromClient + AddClientConnect dedup

**Files:**
- Modify: `source/GameApp/NetComp/CGameConnFromClient.cpp`
- Modify: `source/GameApp/NetComp/CNetComp.cpp` (`AddClientConnect`)

**Interfaces:**
- Consumes: `CModuleMgr::OnClientConnect/DisConnect`, `CNetComp::Add/DelClientConnect`, `GetNetComp` pattern from `CGameConnServer.cpp`
- Produces: live event path ClientConn → NetComp → ModuleMgr → ModuleGate

- [ ] **Step 1: Harden AddClientConnect**

```cpp
void CNetComp::AddClientConnect(CGameConnFromClient *pFromClientConn) {
    if (!pFromClientConn) {
        return;
    }
    uint32 nID = pFromClientConn->GetConnectID();
    if (m_treeFromClient.Find(nID)) {
        return;
    }
    m_treeFromClient.Insert(static_cast<CClientConnectNode &>(*pFromClientConn));
    Log::Info("CNetComp::AddClientConnect clientId={}", nID);
}
```

Confirm `Find` accepts `uint32` key for `CClientConnectNode` tree (operator compares `m_Data`). If Find API differs, use equivalent lookup.

- [ ] **Step 2: Wire CGameConnFromClient**

Reuse static `GetNetComp()` (copy the small helper from `CGameConnServer.cpp` into this file, or move helper to a shared `.cpp`/`.h` under NetComp — **prefer copy small helper** to avoid extra file unless already shared).

```cpp
#include "GameApp/ModuleMgr.h"
#include "GammaApp/CBaseApp.h"

static CNetComp *GetNetComp() {
    CBaseApp *pApp = CBaseApp::Inst();
    if (!pApp) {
        return nullptr;
    }
    return static_cast<CNetComp *>(pApp->GetComp(CNetComp::GetID()));
}

void CGameConnFromClient::OnConnected() {
    uint32 nID = GetConnectID();
    GammaLog << "ClientConn OnConnected( " << this << "," << nID << " ) : "
        << GetRemoteAddress().GetAddress() << ":" << GetRemoteAddress().GetPort();
    GammaLog << "<==>" << GetLocalAddress().GetAddress() << ":" << GetLocalAddress().GetPort() << endl;

    if (auto *pNet = GetNetComp()) {
        pNet->AddClientConnect(this);
    }
    CModuleMgr::Instance()->OnClientConnect(nID);
}

void CGameConnFromClient::OnDisConnect() {
    uint32 nID = GetConnectID();
    GammaLog << "ClientConn OnDisConnect( " << this << "," << nID << " ) : "
        << GetRemoteAddress().GetAddress() << ":" << GetRemoteAddress().GetPort();
    GammaLog << "<==>" << GetLocalAddress().GetAddress() << ":" << GetLocalAddress().GetPort() << endl;

    CModuleMgr::Instance()->OnClientDisConnect(nID);
    if (auto *pNet = GetNetComp()) {
        pNet->DelClientConnect(this);
    }
}
```

- [ ] **Step 3: Build GameApp + ModuleGate**

```bash
cmake --build build/linux --target GameApp ModuleGate test_ModuleGate
ctest --test-dir build/linux -R GateSessionStore_Unit --output-on-failure
```

Expected: build OK, store tests PASS.

- [ ] **Step 4: Commit**

```bash
git add source/GameApp/NetComp/CGameConnFromClient.cpp source/GameApp/NetComp/CNetComp.cpp
git commit -m "$(cat <<'EOF'
feat(NetComp): notify modules on client connect/disconnect

Wire CGameConnFromClient through CNetComp and CModuleMgr.
EOF
)"
```

---

### Task 5: Acceptance checklist

**Files:** none required unless fixes.

- [ ] **Step 1: Rebuild and test**

```bash
cmake --build build/linux --target GameApp ModuleGate test_ModuleGate
ctest --test-dir build/linux -R 'GateSessionStore_Unit|ProcessModuleToml_Unit' --output-on-failure
```

Expected: PASS.

- [ ] **Step 2: Confirm non-goals**

Document in report (no commit if clean): TCP PublicPort smoke skipped due to CGNetwork::Check; no SendShellMsg in Forward stubs.

- [ ] **Step 3: Commit only if fixes landed**

---

## Spec Coverage

| Spec item | Task |
|-----------|------|
| CGateSessionStore + unit tests | 1 |
| IConsummer OnClient* | 2 |
| ModuleMgr broadcast | 2 |
| ModuleGate tables + Forward stubs | 3 |
| CGameConnFromClient wiring | 4 |
| AddClientConnect dedup | 4 |
| No TCP smoke / no real send | 5 (verify) |

## Self-Review Notes

- Store uses `uint32_t`/`int64_t` so tests need not link GammaCommon.
- `AddClient` overwrite resets bind (spec 覆盖).
- Disconnect order: ModuleMgr first, then DelClientConnect.
