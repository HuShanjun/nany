# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

**nany-cpp** 是一个 C++ 游戏引擎基础框架，提供通用工具、网络、数据库、连接管理、Lua 脚本、共享内存等核心基础库。目标平台为 Windows（MSVC）和 Linux，Windows 上使用 C++20，Linux 上使用 C++17。

## 构建命令

前置条件：设置环境变量 `VCPKG_ROOT` 指向 vcpkg 安装目录。

**Windows (MSVC):**
```bash
cmake --preset vcpkg-static
cmake --build build/msvc
```

**Linux:**
```bash
cmake --preset linux-vcpkg-static
cmake --build build/linux
```

**跨平台构建脚本：**
```bash
python build.py
```

输出路径：可执行文件和动态库输出到 `bin/Debug` 或 `bin/Release`，静态库输出到 `libs/Debug` 或 `libs/Release`。

## 架构

### 模块结构与依赖关系

所有模块源码在 `source/` 下，公共头文件在 `include/` 下。依赖关系如下：

```
GammaCommon（动态库，所有模块的基础）
  ├── GammaNetwork（Win 静态库 / Linux 动态库）── 依赖 GammaCommon
  ├── GammaDatabase（Win 静态库 / Linux 动态库）── 依赖 GammaCommon
  ├── GammaScript（动态库）── 依赖 GammaCommon
  ├── GammaConnects（Win 静态库 / Linux 动态库）── 依赖 GammaCommon + GammaNetwork
  ├── GammaMTDbs（Win 静态库 / Linux 动态库）── 依赖 GammaCommon + GammaDatabase
  └── GammaShm（共享内存）
```

`sample/main/` 包含示例可执行程序。

### 关键设计模式

- **IGammaUnknown**：COM 风格的引用计数基接口（`AddRef`/`Release`）。大部分管理器接口继承自此。使用 `DEFAULT_OPERATOR(ClassName)` 或 `DEFAULT_METHOD(ClassName)` 宏来实现。
- **工厂函数**：各模块通过 C 风格工厂函数暴露创建接口（如 `CreateConnMgr()`、`CreateDbsThreadMgr()`），而非直接暴露构造函数。
- **DLL 导出宏**：每个模块定义独立的 `GAMMA_*_API` 宏（如 `GAMMA_COMMON_API`、`GAMMA_CONNECTS_API`）。Windows 下配合 `GAMMA_DLL` 宏解析为 `dllexport`/`dllimport`。
- **命名空间**：所有代码位于 `Gamma` 命名空间内。

### 第三方依赖

- **Lua**（从源码构建，位于 `3rd-src/lua/`）— 脚本引擎
- **vcpkg 管理**（位于 `3rd-libs/`）：OpenSSL、zlib、CURL、libmariadb
- Linux 额外依赖：libuuid

### CMake 约定

每个库的 `CMakeLists.txt` 遵循相同模式：通过 glob 收集源文件，从目录名推导项目名，并引入 `common.cmake` 来链接公共依赖（OpenSSL、ZLIB、CURL、MariaDB、Lua 及平台库）。

## 代码规范

- 使用标准库整数类型（`uint32_t`、`int64_t` 等），旧版自定义类型别名（`uint32`、`int64`）已移除
- 平台相关代码分离到独立文件：`Win32.cpp`/`Win32.h`、`Linux.cpp`/`Linux.h`、`Android.cpp`/`Android.h`、`IOS.h`
- 头文件保护使用 `#pragma once` 或传统 `#ifndef` 守卫
- 注释和文档主要使用中文
