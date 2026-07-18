# GoogleTest Unit/Integration Test Framework Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Wire GoogleTest via vcpkg into nany-cpp with per-module test executables, shared helpers, ctest labels (`unit`/`integration`), and first-pass tests covering the acceptance criteria in the design spec.

**Architecture:** Root CMake optionally builds `test/` when `NANY_BUILD_TESTS=ON`. A shared `gamma_add_gtest` macro creates one executable per module, links `GTest::gtest_main` + Gamma libs (same pattern as `sample/*`), and registers tests with `gtest_discover_tests` twice (unit filter + integration filter) so `ctest -L unit|integration` works. Integration tests read `NANY_TEST_DB_*` / `test/config/test.toml` and `GTEST_SKIP` when dependencies are unavailable.

**Tech Stack:** CMake ≥ 3.19, GoogleTest (vcpkg `gtest`), CTest, existing Gamma modules, toml++ (already vendored for GameApp config tests).

**Spec:** `docs/superpowers/specs/2026-07-18-gtest-unit-test-framework-design.md`

## Global Constraints

- Framework: GoogleTest + `gtest_main` (gmock optional later; do not add in this plan)
- Dependency: add `"gtest"` to `vcpkg/vcpkg.json` only (no FetchContent, no vendoring)
- Layout: one executable per module under `test/<Module>/` named `test_<Module>`
- Source naming: `test_<Topic>.cpp`
- Labels: suite names end with `_Unit` or `_Integration` (e.g. `GammaMath_Unit`, `Database_Integration`); CMake maps these to ctest LABELS `unit` / `integration`
- Switch: `option(NANY_BUILD_TESTS ... ON)`; off path must not require gtest
- Docs language: Chinese for `test/README.md`
- Do not convert `sample/*` into tests
- `IGammaUnknown*` from factories: always `Release()` in TearDown / RAII; never raw `delete`
- Paths below are relative to the CMake project root: `cpp/` (repo path `cpp/...` under `/mnt/e/Practice/nany`)

## File Structure

| Path | Responsibility |
|------|----------------|
| `vcpkg/vcpkg.json` | Add `gtest` dependency |
| `CMakeLists.txt` | `NANY_BUILD_TESTS` + `add_subdirectory(test)` |
| `test/CMakeLists.txt` | find GTest, include macro, add module subdirs |
| `test/cmake/GammaTest.cmake` | `gamma_add_gtest` macro |
| `test/common/TestEnv.h` | Read `NANY_TEST_*` env vars |
| `test/common/TempDir.h` | Temporary directory RAII |
| `test/common/SkipIf.h` | Helpers that call `GTEST_SKIP` |
| `test/common/RefCountPtr.h` | RAII for `IGammaUnknown*` |
| `test/config/test.example.toml` | DB config template |
| `test/README.md` | How to build/run/configure |
| `test/GammaCommon/...` | First module tests (scaffold + expand) |
| `test/GammaNetwork/...` | Address unit + loopback integration |
| `test/GammaScript/...` | Lua smoke unit |
| `test/GammaConnects/...` | ConnMgr smoke + loopback integration |
| `test/GammaDatabase/...` | MariaDB integration |
| `test/GammaMTDbs/...` | MT query integration |
| `test/GammaShm/...` | Shm create/lifecycle integration-style |
| `test/GameApp/...` | toml log-level / parse unit helpers |
| `../.gitignore` (repo root) | Ignore `cpp/test/config/test.toml` |

---

### Task 1: vcpkg gtest + CMake scaffold + GammaCommon smoke

**Files:**
- Modify: `vcpkg/vcpkg.json`
- Modify: `CMakeLists.txt`
- Create: `test/CMakeLists.txt`
- Create: `test/cmake/GammaTest.cmake`
- Create: `test/GammaCommon/CMakeLists.txt`
- Create: `test/GammaCommon/test_GammaMath.cpp`
- Create: `test/README.md` (minimal stub; expanded in Task 2)
- Modify: `/mnt/e/Practice/nany/.gitignore` (add `cpp/test/config/test.toml`)

**Interfaces:**
- Consumes: existing target `GammaCommon`, vcpkg toolchain presets
- Produces: `gamma_add_gtest(...)` macro; executable `test_GammaCommon`; ctest entries labeled `unit`

- [ ] **Step 1: Add gtest to vcpkg manifest**

In `vcpkg/vcpkg.json`, add `"gtest"` to `dependencies` (alongside `curl`, `openssl`, etc.):

```json
{
    "name": "my-project",
    "version": "1.0.0",
    "builtin-baseline": "16ee2ecb31788c336ace8bb14c21801efb6836e4",
    "dependencies": [
      "curl",
      "openssl",
      "libmariadb",
      "zlib",
      "gtest",
      {"name": "libuuid", "platform": "linux"}
    ]
}
```

- [ ] **Step 2: Reconfigure so vcpkg installs gtest**

Run (Linux example; require `VCPKG_ROOT`):

```bash
cd /mnt/e/Practice/nany/cpp
cmake --preset linux-vcpkg-static
```

Expected: configure succeeds and gtest appears under `3rd-libs/linux/...` (or vcpkg install log shows `gtest`).

If configure fails with “Could not find a package configuration file provided by GTest”, fix manifest/install before continuing.

- [ ] **Step 3: Write `test/cmake/GammaTest.cmake`**

```cmake
include(GoogleTest)

# gamma_add_gtest(<target>
#   SOURCES <src>...
#   LIBS <lib>...
#   [RUN_SERIAL]          # set on all discovered tests
# )
function(gamma_add_gtest target_name)
  set(options RUN_SERIAL)
  set(oneValueArgs)
  set(multiValueArgs SOURCES LIBS)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT ARG_SOURCES)
    message(FATAL_ERROR "gamma_add_gtest(${target_name}): SOURCES required")
  endif()

  add_executable(${target_name} ${ARG_SOURCES})
  target_include_directories(${target_name} PRIVATE
    ${CMAKE_SOURCE_DIR}/test/common
    ${CMAKE_SOURCE_DIR}/test
  )
  target_link_libraries(${target_name} PRIVATE
    GTest::gtest_main
    ${ARG_LIBS}
  )
  set_target_properties(${target_name} PROPERTIES FOLDER "test")

  set(_serial_props "")
  if(ARG_RUN_SERIAL)
    set(_serial_props "RUN_SERIAL;TRUE")
  endif()

  # Suite naming convention: *_Unit / *_Integration
  gtest_discover_tests(${target_name}
    TEST_FILTER "*_Unit.*"
    TEST_PREFIX "${target_name}."
    PROPERTIES LABELS "unit" ${_serial_props}
    DISCOVERY_TIMEOUT 60
  )
  gtest_discover_tests(${target_name}
    TEST_FILTER "*_Integration.*"
    TEST_PREFIX "${target_name}."
    PROPERTIES LABELS "integration" ${_serial_props}
    DISCOVERY_TIMEOUT 60
  )
endfunction()
```

- [ ] **Step 4: Write `test/CMakeLists.txt`**

```cmake
find_package(GTest CONFIG REQUIRED)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/GammaTest.cmake)

add_subdirectory(GammaCommon)
# Later tasks append: GammaNetwork, GammaScript, ...
```

- [ ] **Step 5: Wire root `CMakeLists.txt`**

Append after `add_subdirectory(source/GameApp)`:

```cmake
option(NANY_BUILD_TESTS "Build unit/integration tests" ON)
if(NANY_BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()
```

- [ ] **Step 6: Write failing smoke test (assert wrong on purpose first)**

Create `test/GammaCommon/test_GammaMath.cpp`:

```cpp
#include <gtest/gtest.h>
#include "GammaCommon/TVector3.h"

using Gamma::TVector3;

TEST(GammaMath_Unit, DistSqr_3_4_5)
{
    TVector3<float> a(3.f, 4.f, 0.f);
    TVector3<float> origin(0.f, 0.f, 0.f);
    // Intentionally wrong expectation to verify discovery/failure path once;
    // next step corrects to 25.f
    EXPECT_FLOAT_EQ(a.DistSqr(origin), 0.f);
}
```

Create `test/GammaCommon/CMakeLists.txt`:

```cmake
gamma_add_gtest(test_GammaCommon
  SOURCES test_GammaMath.cpp
  LIBS GammaCommon
)
```

- [ ] **Step 7: Build and confirm test is discovered and fails**

```bash
cmake --build build/linux --target test_GammaCommon
ctest --test-dir build/linux -R test_GammaCommon --output-on-failure
```

Expected: FAIL with DistSqr assertion (not link/discovery errors).

- [ ] **Step 8: Fix assertion to correct value**

Replace expectation with `25.f`:

```cpp
EXPECT_FLOAT_EQ(a.DistSqr(origin), 25.f);
```

Rebuild and re-run:

```bash
cmake --build build/linux --target test_GammaCommon
ctest --test-dir build/linux -L unit --output-on-failure
```

Expected: PASS.

- [ ] **Step 9: Minimal README stub + gitignore**

`test/README.md`:

```markdown
# nany-cpp 测试

依赖：vcpkg `gtest`。构建开关：`NANY_BUILD_TESTS`（默认 ON）。

```bash
cmake --preset linux-vcpkg-static
cmake --build build/linux
ctest --test-dir build/linux -L unit --output-on-failure
```

详见后续完整说明（集成 DB 配置等）。
```

Append to repo `.gitignore` (`/mnt/e/Practice/nany/.gitignore`):

```
/cpp/test/config/test.toml
```

- [ ] **Step 10: Commit**

```bash
cd /mnt/e/Practice/nany
git add cpp/vcpkg/vcpkg.json cpp/CMakeLists.txt cpp/test .gitignore
git commit -m "$(cat <<'EOF'
feat(test): scaffold GoogleTest with GammaCommon smoke test

Add vcpkg gtest, gamma_add_gtest helper, and ctest unit label wiring.
EOF
)"
```

---

### Task 2: Shared test helpers + config template + README

**Files:**
- Create: `test/common/TestEnv.h`
- Create: `test/common/TempDir.h`
- Create: `test/common/SkipIf.h`
- Create: `test/common/RefCountPtr.h`
- Create: `test/config/test.example.toml`
- Modify: `test/README.md` (full content)
- Create: `test/GammaCommon/test_TempDir.cpp` (exercises TempDir; proves helpers compile)

**Interfaces:**
- Consumes: `<cstdlib>`, `<filesystem>`, `<string>`, gtest
- Produces: `GammaTest::Env`, `GammaTest::TempDir`, `GammaTest::SkipIfNoDb()`, `GammaTest::RefCountPtr<T>`

- [ ] **Step 1: Write `TestEnv.h`**

```cpp
#pragma once
#include <cstdint>
#include <cstdlib>
#include <string>

namespace GammaTest {

inline std::string GetEnvOr(const char* key, const char* fallback = "")
{
    const char* v = std::getenv(key);
    return v ? std::string(v) : std::string(fallback);
}

struct DbConfig {
    std::string host = "127.0.0.1";
    uint16_t port = 3306;
    std::string user = "root";
    std::string password;
    std::string database = "nany_test";
};

inline DbConfig LoadDbConfigFromEnv()
{
    DbConfig c;
    c.host = GetEnvOr("NANY_TEST_DB_HOST", "127.0.0.1");
    const std::string portStr = GetEnvOr("NANY_TEST_DB_PORT", "3306");
    c.port = static_cast<uint16_t>(std::stoi(portStr.empty() ? "3306" : portStr));
    c.user = GetEnvOr("NANY_TEST_DB_USER", "root");
    c.password = GetEnvOr("NANY_TEST_DB_PASSWORD", "");
    c.database = GetEnvOr("NANY_TEST_DB_DATABASE", "nany_test");
    return c;
}

} // namespace GammaTest
```

- [ ] **Step 2: Write `TempDir.h`**

```cpp
#pragma once
#include <filesystem>
#include <random>
#include <string>

namespace GammaTest {

class TempDir {
public:
    TempDir()
    {
        auto base = std::filesystem::temp_directory_path() / "nany_gtest";
        std::filesystem::create_directories(base);
        path_ = base / ("t_" + std::to_string(std::random_device{}()));
        std::filesystem::create_directories(path_);
    }
    ~TempDir()
    {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }
    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;

    const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
};

} // namespace GammaTest
```

- [ ] **Step 3: Write `SkipIf.h`**

```cpp
#pragma once
#include <gtest/gtest.h>
#include <string>
#include "TestEnv.h"
#include "GammaDatabase/IDatabase.h"

namespace GammaTest {

// Returns true if skipped (caller should return immediately after).
inline bool SkipIfNoDb()
{
    using namespace Gamma;
    DbConfig cfg = LoadDbConfigFromEnv();
    IDatabase* db = GetDatabase();
    if (!db) {
        GTEST_SKIP() << "GetDatabase() returned null";
        return true;
    }
    try {
        IDbConnection* conn = db->CreateConnection(
            cfg.host.c_str(), cfg.port, cfg.user.c_str(), cfg.password.c_str(),
            cfg.database.c_str(), 1, true, false, true);
        if (!conn) {
            GTEST_SKIP() << "CreateConnection returned null";
            return true;
        }
        conn->Release();
        return false;
    } catch (const std::string& err) {
        GTEST_SKIP() << "MariaDB unavailable: " << err;
        return true;
    } catch (...) {
        GTEST_SKIP() << "MariaDB unavailable (unknown error)";
        return true;
    }
}

} // namespace GammaTest
```

Note: `SkipIf.h` pulls Database; only include it from Database/MTDbs tests. Keep `TestEnv.h` / `TempDir.h` header-only with no Gamma module deps.

- [ ] **Step 4: Write `RefCountPtr.h`**

```cpp
#pragma once

namespace GammaTest {

template <typename T>
class RefCountPtr {
public:
    explicit RefCountPtr(T* p = nullptr) : p_(p) {}
    ~RefCountPtr() { reset(); }
    RefCountPtr(const RefCountPtr&) = delete;
    RefCountPtr& operator=(const RefCountPtr&) = delete;
    RefCountPtr(RefCountPtr&& o) noexcept : p_(o.p_) { o.p_ = nullptr; }
    RefCountPtr& operator=(RefCountPtr&& o) noexcept
    {
        if (this != &o) {
            reset();
            p_ = o.p_;
            o.p_ = nullptr;
        }
        return *this;
    }

    T* get() const { return p_; }
    T* operator->() const { return p_; }
    explicit operator bool() const { return p_ != nullptr; }

    void reset(T* p = nullptr)
    {
        if (p_) {
            p_->Release();
        }
        p_ = p;
    }

private:
    T* p_;
};

} // namespace GammaTest
```

- [ ] **Step 5: Write `test/config/test.example.toml`**

```toml
[database]
host = "127.0.0.1"
port = 3306
user = "root"
password = "change-me"
database = "nany_test"
```

- [ ] **Step 6: Add TempDir unit test**

`test/GammaCommon/test_TempDir.cpp`:

```cpp
#include <gtest/gtest.h>
#include <fstream>
#include "TempDir.h"

TEST(TempDir_Unit, CreateAndCleanup)
{
    namespace fs = std::filesystem;
    fs::path kept;
    {
        GammaTest::TempDir dir;
        kept = dir.path();
        ASSERT_TRUE(fs::exists(kept));
        std::ofstream(kept / "marker.txt") << "ok";
        ASSERT_TRUE(fs::exists(kept / "marker.txt"));
    }
    EXPECT_FALSE(fs::exists(kept));
}
```

Update `test/GammaCommon/CMakeLists.txt` SOURCES to include `test_TempDir.cpp`.

- [ ] **Step 7: Expand `test/README.md`**

Document: presets, `-DNANY_BUILD_TESTS=OFF`, `ctest -L unit|integration`, `NANY_TEST_DB_*`, copying `test.example.toml` → `test.toml`, suite naming (`*_Unit` / `*_Integration`), Release/`Release()` rule.

- [ ] **Step 8: Build and run unit tests**

```bash
cmake --build build/linux --target test_GammaCommon
ctest --test-dir build/linux -L unit --output-on-failure
```

Expected: all unit tests PASS (no DB required).

- [ ] **Step 9: Commit**

```bash
git add cpp/test/common cpp/test/config cpp/test/README.md cpp/test/GammaCommon
git commit -m "$(cat <<'EOF'
feat(test): add shared helpers, config template, and README

Provide TempDir/TestEnv/SkipIf/RefCountPtr for module tests.
EOF
)"
```

---

### Task 3: Expand GammaCommon unit tests (MD5)

**Files:**
- Create: `test/GammaCommon/test_Md5.cpp`
- Modify: `test/GammaCommon/CMakeLists.txt`

**Interfaces:**
- Consumes: `GammaCommon/GammaMd5.h` (`Gamma::MD5Ex`)
- Produces: additional `*_Unit` cases in `test_GammaCommon`

- [ ] **Step 1: Write MD5 known-vector test**

```cpp
#include <gtest/gtest.h>
#include <cstring>
#include "GammaCommon/GammaMd5.h"

TEST(Md5_Unit, EmptyString_MD5Ex)
{
    uint8_t out[32] = {};
    Gamma::MD5Ex(out, "", 0);
    // MD5("") = d41d8cd98f00b204e9800998ecf8427e (lowercase hex from MD5Ex)
    EXPECT_EQ(0, std::memcmp(out, "d41d8cd98f00b204e9800998ecf8427e", 32));
}

TEST(Md5_Unit, Abc_MD5Ex)
{
    uint8_t out[32] = {};
    const char* msg = "abc";
    Gamma::MD5Ex(out, msg, 3);
    EXPECT_EQ(0, std::memcmp(out, "900150983cd24fb0d6963f7d28e17f72", 32));
}
```

- [ ] **Step 2: Build and run**

```bash
cmake --build build/linux --target test_GammaCommon
ctest --test-dir build/linux -R test_GammaCommon --output-on-failure
```

Expected: PASS.

- [ ] **Step 3: Commit**

```bash
git add cpp/test/GammaCommon
git commit -m "$(cat <<'EOF'
test(GammaCommon): add MD5 known-vector unit tests
EOF
)"
```

---

### Task 4: GammaNetwork unit tests (CAddress)

**Files:**
- Create: `test/GammaNetwork/CMakeLists.txt`
- Create: `test/GammaNetwork/test_Address.cpp`
- Modify: `test/CMakeLists.txt` — `add_subdirectory(GammaNetwork)`

**Interfaces:**
- Consumes: `GammaNetwork/CAddress.h`, libs `GammaCommon` + `GammaNetwork`
- Produces: `test_GammaNetwork`

- [ ] **Step 1: Write address unit tests**

```cpp
#include <gtest/gtest.h>
#include "GammaNetwork/CAddress.h"

using Gamma::CAddress;
using Gamma::IsIP;
using Gamma::IsPort;

TEST(Address_Unit, ConstructAndGetters)
{
    CAddress addr("127.0.0.1", 8080);
    EXPECT_STREQ("127.0.0.1", addr.GetAddress());
    EXPECT_EQ(8080, addr.GetPort());
}

TEST(Address_Unit, IsIP_IsPort)
{
    EXPECT_TRUE(IsIP("192.168.0.1"));
    EXPECT_FALSE(IsIP("not-an-ip"));
    EXPECT_TRUE(IsPort("443"));
    EXPECT_FALSE(IsPort("70000"));
}
```

`test/GammaNetwork/CMakeLists.txt`:

```cmake
gamma_add_gtest(test_GammaNetwork
  SOURCES test_Address.cpp
  LIBS GammaCommon GammaNetwork
)
```

- [ ] **Step 2: Register subdirectory and build**

In `test/CMakeLists.txt` add `add_subdirectory(GammaNetwork)`.

```bash
cmake --build build/linux --target test_GammaNetwork
ctest --test-dir build/linux -R test_GammaNetwork -L unit --output-on-failure
```

Expected: PASS. If `IsPort("70000")` behavior differs from assumption, adjust assertion to match actual API (read `CAddress.cpp` / helpers) — do not change production code in this task unless fixing a clear bug.

- [ ] **Step 3: Commit**

```bash
git add cpp/test/GammaNetwork cpp/test/CMakeLists.txt
git commit -m "$(cat <<'EOF'
test(GammaNetwork): add CAddress unit tests
EOF
)"
```

---

### Task 5: GammaScript smoke unit test

**Files:**
- Create: `test/GammaScript/CMakeLists.txt`
- Create: `test/GammaScript/test_ScriptLuaSmoke.cpp`
- Modify: `test/CMakeLists.txt`

**Interfaces:**
- Consumes: `GammaScript/CScriptLua.h` (constructor + destructor; if RunString/equivalent exists use it)
- Produces: `test_GammaScript`

- [ ] **Step 1: Inspect public Lua API before writing**

```bash
rg -n "RunString|RunFile|DoString|Execute" include/gamma/GammaScript source/gamma/GammaScript --glob '*.h'
```

Use the simplest public API that evaluates a Lua snippet returning a number. If only construction is safe without more macros, keep a construct/destruct smoke test.

- [ ] **Step 2: Write smoke test (adjust API names to match Step 1)**

Preferred shape if a string runner exists:

```cpp
#include <gtest/gtest.h>
#include "GammaScript/CScriptLua.h"

TEST(ScriptLua_Unit, ConstructAndRunArithmetic)
{
    Gamma::CScriptLua script(nullptr, 0, false);
    // Replace RunString with actual API discovered in Step 1
    // EXPECT_EQ(expected, script.RunString("return 1+2"));
    SUCCEED();
}
```

If full script execution needs class registration macros, leave `SUCCEED()` smoke and add a follow-up comment in the test file documenting the next API to cover — still counts as a linking/smoke unit test for the module target.

- [ ] **Step 3: CMake + build**

```cmake
gamma_add_gtest(test_GammaScript
  SOURCES test_ScriptLuaSmoke.cpp
  LIBS GammaCommon GammaScript
)
```

```bash
cmake --build build/linux --target test_GammaScript
ctest --test-dir build/linux -R test_GammaScript -L unit --output-on-failure
```

Expected: PASS (or SKIP only if intentionally skipped — prefer PASS).

- [ ] **Step 4: Commit**

```bash
git add cpp/test/GammaScript cpp/test/CMakeLists.txt
git commit -m "$(cat <<'EOF'
test(GammaScript): add Lua smoke unit test target
EOF
)"
```

---

### Task 6: GammaConnects unit smoke (CreateConnMgr lifecycle)

**Files:**
- Create: `test/GammaConnects/CMakeLists.txt`
- Create: `test/GammaConnects/test_ConnMgrSmoke.cpp`
- Modify: `test/CMakeLists.txt`

**Interfaces:**
- Consumes: `CreateConnMgr`, `IConnectionMgr`, `GammaTest::RefCountPtr`
- Produces: `test_GammaConnects`

- [ ] **Step 1: Write lifecycle unit test**

```cpp
#include <gtest/gtest.h>
#include "GammaConnects/IConnectionMgr.h"
#include "RefCountPtr.h"

TEST(ConnMgr_Unit, CreateAndRelease)
{
    GammaTest::RefCountPtr<Gamma::IConnectionMgr> mgr(
        Gamma::CreateConnMgr(/*nAutoDisconnectTime*/ 30000, /*bStrictMode*/ false));
    ASSERT_TRUE(mgr);
}
```

```cmake
gamma_add_gtest(test_GammaConnects
  SOURCES test_ConnMgrSmoke.cpp
  LIBS GammaCommon GammaNetwork GammaConnects
)
```

- [ ] **Step 2: Build and run**

```bash
cmake --build build/linux --target test_GammaConnects
ctest --test-dir build/linux -R test_GammaConnects -L unit --output-on-failure
```

Expected: PASS; no leak of `IConnectionMgr` (Release via RAII).

- [ ] **Step 3: Commit**

```bash
git add cpp/test/GammaConnects cpp/test/CMakeLists.txt
git commit -m "$(cat <<'EOF'
test(GammaConnects): add CreateConnMgr lifecycle unit smoke
EOF
)"
```

---

### Task 7: GammaDatabase integration (SKIP when DB down)

**Files:**
- Create: `test/GammaDatabase/CMakeLists.txt`
- Create: `test/GammaDatabase/test_DbConnect.cpp`
- Modify: `test/CMakeLists.txt`

**Interfaces:**
- Consumes: `GetDatabase()`, `GammaTest::SkipIfNoDb`, `GammaTest::LoadDbConfigFromEnv`
- Produces: `test_GammaDatabase` with `RUN_SERIAL`

- [ ] **Step 1: Write integration test**

```cpp
#include <gtest/gtest.h>
#include "GammaDatabase/IDatabase.h"
#include "SkipIf.h"
#include "TestEnv.h"

TEST(Database_Integration, ConnectAndSimpleQuery)
{
    if (GammaTest::SkipIfNoDb()) {
        return;
    }
    auto cfg = GammaTest::LoadDbConfigFromEnv();
    Gamma::IDatabase* db = Gamma::GetDatabase();
    Gamma::IDbConnection* conn = db->CreateConnection(
        cfg.host.c_str(), cfg.port, cfg.user.c_str(), cfg.password.c_str(),
        cfg.database.c_str(), 1, true, false, true);
    ASSERT_NE(conn, nullptr);

    Gamma::IDbTextResult* result = conn->Execute("SELECT 1 AS one");
    ASSERT_NE(result, nullptr);
    ASSERT_GE(result->GetRowNum(), 1u);
    result->Locate(0);
    const char* val = result->GetData(0);
    ASSERT_NE(val, nullptr);
    EXPECT_STREQ("1", val);
    result->Release();
    conn->Release();
}
```

```cmake
gamma_add_gtest(test_GammaDatabase
  SOURCES test_DbConnect.cpp
  LIBS GammaCommon GammaDatabase
  RUN_SERIAL
)
```

On Linux, if link errors for mariadb/openssl appear, mirror `sample/GammaDatabase/CMakeLists.txt` / `include(common.cmake)` patterns for the test target (add the same `target_link_libraries` extras the sample needs). Prefer extending `gamma_add_gtest` with an optional `EXTRA_LIBS` rather than duplicating ad-hoc link lines in every module.

- [ ] **Step 2: Verify SKIP without DB**

Unset DB password / point to closed port:

```bash
NANY_TEST_DB_HOST=127.0.0.1 NANY_TEST_DB_PORT=1 \
  ctest --test-dir build/linux -R test_GammaDatabase -L integration --output-on-failure
```

Expected: test result **Not Run / Skipped** (not Failed). Exit code of ctest should still be 0 when only skips occur.

- [ ] **Step 3: Verify PASS with real DB (manual when available)**

```bash
export NANY_TEST_DB_HOST=127.0.0.1
export NANY_TEST_DB_PORT=3306
export NANY_TEST_DB_USER=root
export NANY_TEST_DB_PASSWORD=...
export NANY_TEST_DB_DATABASE=nany_test
ctest --test-dir build/linux -R test_GammaDatabase -L integration --output-on-failure
```

Expected: PASS.

- [ ] **Step 4: Commit**

```bash
git add cpp/test/GammaDatabase cpp/test/CMakeLists.txt cpp/test/cmake/GammaTest.cmake
git commit -m "$(cat <<'EOF'
test(GammaDatabase): add MariaDB connect integration with GTEST_SKIP
EOF
)"
```

---

### Task 8: GammaNetwork loopback integration

**Files:**
- Create: `test/GammaNetwork/test_LoopbackTcp.cpp`
- Modify: `test/GammaNetwork/CMakeLists.txt`

**Interfaces:**
- Consumes: `CreateNetWork`, `INetwork`, `IListener`, `IConnecter`, handler interfaces from `INetHandler.h`
- Produces: `Network_Integration.LoopbackSendRecv` (or SKIP if bind fails)

- [ ] **Step 1: Study sample handlers**

Read `sample/GammaNetwork/server.cpp` and `client.cpp` for minimal listen/connect/send/recv pattern. Copy the smallest handler stubs needed into the test file (do not modify samples).

- [ ] **Step 2: Implement loopback test**

Outline (fill handler methods to match interfaces exactly):

```cpp
TEST(Network_Integration, LoopbackSendRecv)
{
    Gamma::INetwork* net = Gamma::CreateNetWork();
    ASSERT_NE(net, nullptr);

    // StartListener on 127.0.0.1 port 0 or a high free port (e.g. 19090)
    // On bind failure: GTEST_SKIP() << "cannot bind";
    // Connect from same INetwork (or second instance if required by design)
    // Send a few bytes; pump Check() until received or timeout (~2s)
    // Release listener/connecter/network

    net->Release();
}
```

Implementation must:
- Use `GTEST_SKIP` on bind/permission failures
- Always `Release()` resources on all paths (prefer RAII wrappers around `Release()`-based network objects if they are not `IGammaUnknown`)

- [ ] **Step 3: Build and run**

```bash
cmake --build build/linux --target test_GammaNetwork
ctest --test-dir build/linux -R test_GammaNetwork --output-on-failure
```

Expected: unit PASS; integration PASS or SKIP (not FAIL) without special privileges.

- [ ] **Step 4: Commit**

```bash
git add cpp/test/GammaNetwork
git commit -m "$(cat <<'EOF'
test(GammaNetwork): add loopback TCP integration test
EOF
)"
```

---

### Task 9: GammaMTDbs + GammaShm integration scaffolds

**Files:**
- Create: `test/GammaMTDbs/CMakeLists.txt`, `test/GammaMTDbs/test_DbsThreadMgr.cpp`
- Create: `test/GammaShm/CMakeLists.txt`, `test/GammaShm/test_ShareMemoryMgr.cpp`
- Modify: `test/CMakeLists.txt`

**Interfaces:**
- Consumes: `CreateDbsThreadMgr`, `CreateShareMemoryMgr`, `TempDir`, `SkipIfNoDb`, `RefCountPtr`
- Produces: `test_GammaMTDbs`, `test_GammaShm`

- [ ] **Step 1: MTDbs — create mgr then SKIP/query**

Mirror `sample/GammaMTDbs/main.cpp` for the shortest create + one query path. If DB unavailable, `SkipIfNoDb()` then return. Always `Release()` the thread mgr.

Use `RUN_SERIAL` in `gamma_add_gtest`.

- [ ] **Step 2: Shm — create against TempDir file**

```cpp
TEST(ShareMemory_Integration, CreateAgainstTempFile)
{
    GammaTest::TempDir dir;
    auto path = (dir.path() / "shm_test.dat").string();
    // Prepare SShareCommonHead* as required by CreateShareMemoryMgr
    // (read IShareMemoryMgr.h / sample/GammaShm for required header layout).
    // On failure to map: GTEST_SKIP or FAIL with clear message.
    // Start/Check briefly, then Release().
}
```

If `SShareCommonHead` setup is too heavy for a short test, keep a compile-linked smoke that documents required fields and `GTEST_SKIP() << "shm fixture not fully wired"` only as last resort — prefer a real create/release against temp file.

- [ ] **Step 3: Build all new targets + full unit suite**

```bash
cmake --build build/linux
ctest --test-dir build/linux -L unit --output-on-failure
ctest --test-dir build/linux -L integration --output-on-failure
```

Expected: unit all PASS; integration PASS or SKIP only.

- [ ] **Step 4: Commit**

```bash
git add cpp/test/GammaMTDbs cpp/test/GammaShm cpp/test/CMakeLists.txt
git commit -m "$(cat <<'EOF'
test: add GammaMTDbs and GammaShm integration scaffolds
EOF
)"
```

---

### Task 10: GameApp toml unit + acceptance checklist

**Files:**
- Create: `test/GameApp/CMakeLists.txt`
- Create: `test/GameApp/test_TomlConfig.cpp`
- Modify: `test/CMakeLists.txt`
- Modify: `test/README.md` if any command drifted

**Interfaces:**
- Consumes: vendored `toml++/toml.hpp` (already used by GameApp); do **not** require linking full `GameApp` executable if parsing can be tested in isolation
- Produces: `test_GameApp` unit cases for config shape used by `LoadConfig` / log level

- [ ] **Step 1: Extract expected toml keys from GameApp**

Read `source/GameApp/GameApp.cpp` `LoadConfig` / `GetLogLevel` / `IsShowConsole` and mirror the key paths in a pure toml parse test (construct `toml::parse` from a string literal — no devops directory required).

Example shape (adjust keys to match real code):

```cpp
#include <gtest/gtest.h>
#include "toml++/toml.hpp"

TEST(GameAppConfig_Unit, ParseLogLevelFromToml)
{
    auto tbl = toml::parse(R"(
        [log]
        level = 2
        show_console = true
    )");
    // Assert the same fields GameApp reads; if GameApp uses different table names,
    // match those exactly.
    ASSERT_TRUE(tbl.contains("log"));
}
```

If GameApp logic is not extractable without linking the exe, either:
- link object sources that are pure (prefer not), or
- keep a toml-shape contract test that locks the config schema GameApp expects.

Do not start the full server process in unit tests.

- [ ] **Step 2: Build and run full acceptance**

```bash
cmake --build build/linux
ctest --test-dir build/linux -L unit --output-on-failure
# Intentionally bad DB — must not fail the suite
NANY_TEST_DB_PORT=1 ctest --test-dir build/linux -L integration --output-on-failure
```

Acceptance checklist (must all be true):

1. `NANY_BUILD_TESTS=ON` builds `test_GammaCommon` (and other module targets added)
2. `ctest -L unit` passes with no MariaDB
3. Bad DB yields SKIP for integration DB tests, not FAIL
4. `test/README.md` documents gtest, DB env vars, and ctest commands

- [ ] **Step 3: Commit**

```bash
git add cpp/test/GameApp cpp/test/CMakeLists.txt cpp/test/README.md
git commit -m "$(cat <<'EOF'
test(GameApp): add toml config unit tests; complete test framework acceptance
EOF
)"
```

---

## Self-Review (plan vs spec)

| Spec requirement | Task |
|------------------|------|
| vcpkg `gtest` | Task 1 |
| `NANY_BUILD_TESTS` + `enable_testing` | Task 1 |
| Per-module executables + `gamma_add_gtest` | Task 1, 4–10 |
| Labels `unit` / `integration` via suite naming | Task 1 (`GammaTest.cmake`) |
| `test/common` helpers + config example | Task 2 |
| GammaCommon unit (math/MD5/temp) | Tasks 1–3 |
| GammaScript / Network / Connects / DB / MTDbs / Shm / GameApp | Tasks 4–10 |
| SKIP when DB unavailable | Tasks 2, 7 |
| `test/README.md` + gitignore `test.toml` | Tasks 1–2, 10 |
| Acceptance criteria §7 | Task 10 checklist |

**Placeholder scan:** No TBD left as blockers; Script/Shm steps include API inspection with concrete smoke fallbacks.

**Type consistency:** Executable names `test_<Module>`; suites `*_Unit` / `*_Integration`; helpers under `GammaTest::`; factory cleanup via `RefCountPtr` / `Release()`.
