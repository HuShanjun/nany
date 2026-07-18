# nany-cpp 测试

基于 GoogleTest（vcpkg `gtest`）的单元/集成测试框架。

## 构建开关

- `NANY_BUILD_TESTS`（默认 `ON`）：是否构建 `test/` 下的所有测试目标。关闭：

```bash
cmake --preset linux-vcpkg-static -DNANY_BUILD_TESTS=OFF
```

## 构建与运行

```bash
cmake --preset linux-vcpkg-static
cmake --build build/linux
ctest --test-dir build/linux -L unit --output-on-failure
```

## 用例分类与标签

测试套件命名约定：

- `*_Unit`：单元测试，不依赖外部资源（数据库、网络等），始终运行，通过 `gtest_discover_tests` 打上 `unit` 标签。
- `*_Integration`：集成测试，可能依赖数据库等外部资源，打上 `integration` 标签。

按标签选择运行：

```bash
ctest --test-dir build/linux -L unit --output-on-failure          # 仅单元测试
ctest --test-dir build/linux -L integration --output-on-failure   # 仅集成测试
```

## 数据库配置（集成测试）

集成测试如需连接 MariaDB/MySQL，通过环境变量覆盖默认配置（默认值见 `test/common/TestEnv.h`）：

| 环境变量 | 默认值 | 说明 |
| --- | --- | --- |
| `NANY_TEST_DB_HOST` | `127.0.0.1` | 数据库主机 |
| `NANY_TEST_DB_PORT` | `3306` | 数据库端口 |
| `NANY_TEST_DB_USER` | `root` | 用户名 |
| `NANY_TEST_DB_PASSWORD` | （空） | 密码 |
| `NANY_TEST_DB_DATABASE` | `nany_test` | 数据库名 |

也可以复制配置模板作为本地参考（当前测试代码通过环境变量读取配置，`test.toml` 仅作为文档化记录，不会被自动加载）：

```bash
cp test/config/test.example.toml test/config/test.toml
```

`test.toml`（及其他本地私密配置）不应提交到仓库。

若数据库不可用，依赖数据库的集成测试应调用 `GammaTest::SkipIfNoDb()`（`test/common/SkipIf.h`）并在返回 `true` 时立即 `return`，使该用例以 SKIPPED 状态结束，而不是失败。

## 共享测试辅助工具（`test/common/`）

- `TestEnv.h` — `GammaTest::GetEnvOr`、`GammaTest::DbConfig`、`GammaTest::LoadDbConfigFromEnv()`。仅依赖标准库，不引入任何 Gamma 模块。
- `TempDir.h` — `GammaTest::TempDir`：构造时创建一个随机命名的临时目录，析构时自动递归删除。仅依赖标准库，不引入任何 Gamma 模块。
- `SkipIf.h` — `GammaTest::SkipIfNoDb()`：尝试连接测试数据库，失败则 `GTEST_SKIP()`。**引入了 `GammaDatabase`，仅应在 Database / MTDbs 相关测试中包含**，不要在通用/无数据库依赖的测试中包含。
- `RefCountPtr.h` — `GammaTest::RefCountPtr<T>`：管理 `IGammaUnknown` 风格（`AddRef`/`Release`）接口指针的移动语义智能指针，测试代码中用于避免手动调用 `Release()` 忘记释放。

`test/common` 已通过 `gamma_add_gtest`（见 `test/cmake/GammaTest.cmake`）加入所有测试目标的 include 路径，直接 `#include "TempDir.h"` 等即可使用，无需相对路径。

## 编写测试的注意事项

- 用例命名必须以 `_Unit` 或 `_Integration` 结尾（对应 test suite 名，如 `TEST(Foo_Unit, Bar)`），否则不会被 `unit`/`integration` 标签发现，也不会被执行。
- 对继承自 `IGammaUnknown` 的对象（`AddRef`/`Release` 引用计数），测试中获取后必须调用 `Release()`（或使用 `GammaTest::RefCountPtr<T>` 自动管理），避免内存/资源泄漏导致后续用例失败或工具报警。
- 新增模块测试时，在 `test/<Module>/CMakeLists.txt` 中使用 `gamma_add_gtest(...)` 声明目标，并在 `test/CMakeLists.txt` 中 `add_subdirectory(<Module>)`。
