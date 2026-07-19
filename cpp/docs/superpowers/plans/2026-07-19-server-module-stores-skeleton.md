# Server Module Stores Skeleton Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add shared `CPeerOnlineStore` and per-server business stores (Login/Center/Game/DBMain), migrate Gate peers, with unit tests; no real network/DB I/O.

**Architecture:** Header-only peer set in `include/GameApp/`. Each module owns a business store (Center/Game/DB embed peers; Login keeps peers on the module and auth tickets in `CLoginAuthStore`). Modules update stores from `OnServerConnect/DisConnect` and expose logged stub APIs.

**Tech Stack:** C++17/23, GoogleTest (`gamma_add_gtest`), existing Module* SHARED targets.

**Spec:** `docs/superpowers/specs/2026-07-19-server-module-stores-skeleton-design.md`

## Global Constraints

- No real SQL, protocol, `SendShellMsg`, or CGNetwork::Check fix
- `CPeerOnlineStore` is **header-only** under `include/GameApp/`
- `serverId == 0` ignored on peer Add
- Empty auth account → `SubmitAuth` returns `0`
- Gate existing `GateSessionStore_Unit` tests must keep passing after peer migration
- Paths relative to CMake project root `cpp/`
- Work in-place on `main` only if user already consented (this repo: user previously consented)

## File Structure

| Path | Responsibility |
|------|----------------|
| `include/GameApp/CPeerOnlineStore.h` | Peer online set |
| `source/module/ModuleGate/CGateSessionStore.*` | Use `CPeerOnlineStore` |
| `source/module/ModuleLogin/CLoginAuthStore.*` + ModuleLogin | Auth tickets + peers on module |
| `source/module/ModuleCenter/CCenterRegistryStore.*` + ModuleCenter | Registry embeds peers |
| `source/module/ModuleGame/CGamePlayerStore.*` + ModuleGame | Players + peer cleanup |
| `source/module/ModuleDBMain/CDBMainJobStore.*` + ModuleDBMain | Job queue embeds peers |
| `test/GameApp/test_PeerOnlineStore.cpp` | Peer unit tests |
| `test/ModuleLogin|Center|Game|DBMain/` | Business store tests |

---

### Task 1: CPeerOnlineStore + unit tests (TDD)

**Files:**
- Create: `include/GameApp/CPeerOnlineStore.h`
- Create: `test/GameApp/test_PeerOnlineStore.cpp`
- Modify: `test/GameApp/CMakeLists.txt` — add source to `test_GameApp` SOURCES

**Interfaces:**
- Produces: `class CPeerOnlineStore { void Add(uint32_t); void Remove(uint32_t); bool Has(uint32_t) const; size_t Size() const; }`

- [ ] **Step 1: Write failing test**

```cpp
#include <gtest/gtest.h>
#include "GameApp/CPeerOnlineStore.h"

TEST(PeerOnlineStore_Unit, AddRemoveHasIgnoresZero) {
    CPeerOnlineStore peers;
    peers.Add(0);
    EXPECT_EQ(0u, peers.Size());
    peers.Add(200);
    EXPECT_TRUE(peers.Has(200));
    EXPECT_EQ(1u, peers.Size());
    peers.Remove(200);
    EXPECT_FALSE(peers.Has(200));
    EXPECT_EQ(0u, peers.Size());
}
```

Update `test/GameApp/CMakeLists.txt`:

```cmake
gamma_add_gtest(test_GameApp
  SOURCES test_TomlConfig.cpp test_ProcessModuleToml.cpp test_PeerOnlineStore.cpp
)
target_compile_definitions(test_GameApp PRIVATE NANY_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
```

- [ ] **Step 2: Build — expect fail (missing header)**

```bash
cmake --build build/linux --target test_GameApp 2>&1 | head -40
```

- [ ] **Step 3: Implement header-only store**

`include/GameApp/CPeerOnlineStore.h`:

```cpp
#pragma once
#include <cstdint>
#include <set>

class CPeerOnlineStore {
public:
    void Add(uint32_t nServerID) {
        if (nServerID == 0) {
            return;
        }
        m_set.insert(nServerID);
    }
    void Remove(uint32_t nServerID) { m_set.erase(nServerID); }
    bool Has(uint32_t nServerID) const { return m_set.find(nServerID) != m_set.end(); }
    size_t Size() const { return m_set.size(); }

private:
    std::set<uint32_t> m_set;
};
```

- [ ] **Step 4: Reconfigure if needed, run test**

```bash
cmake --build build/linux --target test_GameApp
ctest --test-dir build/linux -R PeerOnlineStore_Unit --output-on-failure
```

Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add include/GameApp/CPeerOnlineStore.h test/GameApp/test_PeerOnlineStore.cpp test/GameApp/CMakeLists.txt
git commit -m "$(cat <<'EOF'
feat(GameApp): add header-only CPeerOnlineStore

Shared online server-id set for module skeleton stores.
EOF
)"
```

---

### Task 2: Migrate CGateSessionStore to PeerOnline

**Files:**
- Modify: `source/module/ModuleGate/CGateSessionStore.h`
- Modify: `source/module/ModuleGate/CGateSessionStore.cpp`
- Test: existing `test/ModuleGate/test_GateSessionStore.cpp` (no API change)

**Interfaces:**
- Consumes: `GameApp/CPeerOnlineStore.h`
- Produces: same public Gate store API; internal `CPeerOnlineStore m_peers`

- [ ] **Step 1: Run Gate tests (baseline green)**

```bash
ctest --test-dir build/linux -R GateSessionStore_Unit --output-on-failure
```

- [ ] **Step 2: Replace `m_setServers` with `m_peers`**

In header: `#include "GameApp/CPeerOnlineStore.h"`, remove `<set>`, member `CPeerOnlineStore m_peers`.

In cpp:

```cpp
void CGateSessionStore::AddServer(uint32_t nServerID) {
    m_peers.Add(nServerID);
}
void CGateSessionStore::RemoveServer(uint32_t nServerID) {
    m_peers.Remove(nServerID);
    for (auto &[id, session] : m_mapClients) {
        (void)id;
        if (session.nBoundServerID == nServerID) {
            session.nBoundServerID = 0;
        }
    }
}
bool CGateSessionStore::HasServer(uint32_t nServerID) const {
    return m_peers.Has(nServerID);
}
```

Ensure `test_ModuleGate` include path sees `include/` (gamma_add_gtest / source include — ModuleGate cmake already has `${CMAKE_SOURCE_DIR}/include` via target? test only includes ModuleGate dir — **add**:

```cmake
target_include_directories(test_ModuleGate PRIVATE
  ${CMAKE_SOURCE_DIR}/source/module/ModuleGate
  ${CMAKE_SOURCE_DIR}/include
)
```

- [ ] **Step 3: Rebuild and run Gate tests**

```bash
cmake --build build/linux --target test_ModuleGate ModuleGate
ctest --test-dir build/linux -R GateSessionStore_Unit --output-on-failure
```

Expected: PASS.

- [ ] **Step 4: Commit**

```bash
git add source/module/ModuleGate/CGateSessionStore.h source/module/ModuleGate/CGateSessionStore.cpp test/ModuleGate/CMakeLists.txt
git commit -m "$(cat <<'EOF'
refactor(ModuleGate): back CGateSessionStore peers with CPeerOnlineStore

Keep Gate session APIs; reuse shared peer online set.
EOF
)"
```

---

### Task 3: ModuleLogin — CLoginAuthStore + peers

**Files:**
- Create: `source/module/ModuleLogin/CLoginAuthStore.h`
- Create: `source/module/ModuleLogin/CLoginAuthStore.cpp`
- Modify: `source/module/ModuleLogin/ModuleLogin.h`
- Modify: `source/module/ModuleLogin/ModuleLogin.cpp`
- Create: `test/ModuleLogin/CMakeLists.txt`
- Create: `test/ModuleLogin/test_LoginAuthStore.cpp`
- Modify: `test/CMakeLists.txt` — `add_subdirectory(ModuleLogin)`

**Interfaces:**
- `CLoginAuthStore`: `SubmitAuth`, `CompleteAuth`, `HasTicket` (+ optional `FindTicket`)
- `CModuleLogin`: `CPeerOnlineStore m_peers` + `CLoginAuthStore m_auth`; hooks + `SubmitAuth`/`CompleteAuth` wrappers with logs

- [ ] **Step 1: Write failing Login store tests**

```cpp
#include <gtest/gtest.h>
#include "CLoginAuthStore.h"

TEST(LoginAuthStore_Unit, SubmitCompleteFlow) {
    CLoginAuthStore store;
    EXPECT_EQ(0u, store.SubmitAuth(""));
    uint32_t id = store.SubmitAuth("alice");
    ASSERT_NE(0u, id);
    EXPECT_TRUE(store.HasTicket(id));
    EXPECT_TRUE(store.CompleteAuth(id, true));
    EXPECT_FALSE(store.CompleteAuth(999, true));
}
```

CMake like ModuleGate test, sources include `CLoginAuthStore.cpp`.

- [ ] **Step 2: Implement store**

- `m_nNextTicket` starting at 1
- `SubmitAuth`: empty → 0; else create pending ticket
- `CompleteAuth`: missing → false; set state 1 or 2

- [ ] **Step 3: Wire ModuleLogin**

```cpp
// OnServerConnect: m_peers.Add(nServerID);
// OnServerDisConnect: m_peers.Remove(nServerID);
// SubmitAuth/CompleteAuth: Log + m_auth.*
```

- [ ] **Step 4: Build ModuleLogin + tests; ctest LoginAuthStore_Unit**

- [ ] **Step 5: Commit**

```bash
git commit -m "$(cat <<'EOF'
feat(ModuleLogin): add auth ticket store and peer tracking

Skeleton SubmitAuth/CompleteAuth with unit tests.
EOF
)"
```

---

### Task 4: ModuleCenter — CCenterRegistryStore

**Files:**
- Create: `CCenterRegistryStore.h/.cpp` under ModuleCenter
- Modify: ModuleCenter.h/.cpp
- Create: `test/ModuleCenter/` + register in `test/CMakeLists.txt`

**Interfaces:**
- Embedded `CPeerOnlineStore`
- Public: `Peers()` accessor or `AddPeer`/`RemovePeer` methods forwarding; `RegisterServer`/`UnregisterServer`/`IsRegistered`
- Spec: Register requires `peers.Has(id)`. Module connect: `store.AddPeer(id); store.RegisterServer(id, id/100);`

Recommended store API:

```cpp
void AddPeer(uint32_t id) { m_peers.Add(id); }
void RemovePeer(uint32_t id); // UnregisterServer + m_peers.Remove
bool RegisterServer(uint32_t id, uint32_t typeId);
void UnregisterServer(uint32_t id);
bool IsRegistered(uint32_t id) const;
```

- [ ] **Step 1: Tests** — register fails without peer; succeeds after AddPeer; RemovePeer clears registration

- [ ] **Step 2: Implement store + Module hooks**

- [ ] **Step 3: Build + ctest**

- [ ] **Step 4: Commit**

```bash
git commit -m "$(cat <<'EOF'
feat(ModuleCenter): add server registry store skeleton

Register peers on connect; unit-test peer-gated registration.
EOF
)"
```

---

### Task 5: ModuleGame — CGamePlayerStore

**Files:** analogous under ModuleGame + `test/ModuleGame`

**Interfaces:**

```cpp
void AddPeer(uint32_t id);
void RemovePeer(uint32_t id); // also erase players with nGateServerID==id
bool PlayerEnter(uint64_t playerId, uint32_t gateServerId);
void PlayerLeave(uint64_t playerId);
bool HasPlayer(uint64_t playerId) const;
```

- [ ] **Step 1: Tests** — enter fails without peer; enter ok; RemovePeer drops players on that gate

- [ ] **Step 2: Implement + Module `PlayerEnter`/`PlayerLeave` logged wrappers; `OnServer*` → AddPeer/RemovePeer

- [ ] **Step 3: Build + ctest**

- [ ] **Step 4: Commit**

```bash
git commit -m "$(cat <<'EOF'
feat(ModuleGame): add player session store skeleton

Peer-gated PlayerEnter and cleanup on gate disconnect.
EOF
)"
```

---

### Task 6: ModuleDBMain — CDBMainJobStore

**Files:** analogous under ModuleDBMain + `test/ModuleDBMain`

**Interfaces:**

```cpp
void AddPeer(uint32_t id);
void RemovePeer(uint32_t id);
uint32_t Enqueue(uint32_t fromServerId); // 0 if !Has peer
bool Complete(uint32_t jobId);
bool HasJob(uint32_t jobId) const;
```

- [ ] **Step 1: Tests** — enqueue fails without peer; enqueue+complete ok

- [ ] **Step 2: Implement + Module `EnqueueQuery`/`CompleteQuery` wrappers; server hooks → peers

- [ ] **Step 3: Build + ctest**

- [ ] **Step 4: Commit**

```bash
git commit -m "$(cat <<'EOF'
feat(ModuleDBMain): add query job store skeleton

Peer-gated enqueue/complete stubs with unit tests.
EOF
)"
```

---

### Task 7: Full acceptance

**Files:** none unless fixes

- [ ] **Step 1: Build all modules + GameApp + all new tests**

```bash
cmake --build build/linux --target GameApp ModuleLogin ModuleGame ModuleCenter ModuleGate ModuleDBMain \
  test_GameApp test_ModuleGate test_ModuleLogin test_ModuleCenter test_ModuleGame test_ModuleDBMain
ctest --test-dir build/linux -R 'PeerOnlineStore_Unit|GateSessionStore_Unit|LoginAuthStore_Unit|CenterRegistry|GamePlayer|DBMainJob' --output-on-failure
```

Expected: all PASS. Adjust `-R` regex to match actual suite names used in tests (`CenterRegistryStore_Unit`, `GamePlayerStore_Unit`, `DBMainJobStore_Unit`).

- [ ] **Step 2: Commit only if fixes were needed**

---

## Spec Coverage

| Spec | Task |
|------|------|
| CPeerOnlineStore | 1 |
| Gate migrate peers | 2 |
| Login auth store | 3 |
| Center registry | 4 |
| Game players | 5 |
| DBMain jobs | 6 |
| Build + all unit tests | 7 |

## Self-Review Notes

- Login peers live on `CModuleLogin`, not inside `CLoginAuthStore` (auth independent of peer gate for tickets).
- Center/Game/DB embed peers and peer-gate mutating APIs.
- Header-only PeerOnline needs `include/` on every test that pulls Gate/Center/etc. after Gate migration.
