# C++ 构建说明

## Windows (MSVC + vcpkg)

1. 设置环境变量 `VCPKG_ROOT` 指向 vcpkg 根目录（例如 `C:\vcpkg\vcpkg`）。
2. 使用 CMake 预设 `vcpkg-static` 配置并构建。

若出现 **"Unable to find a valid Visual Studio instance"**：

- 当前预设默认使用：`C:/Program Files/Microsoft Visual Studio/2022/Community`
- 若你安装的是 **Professional** 或 **Build Tools**，请修改 `CMakePresets.json` 里 `vcpkg-static` 的 `environment.VCPKG_VISUAL_STUDIO_PATH` 为实际路径，例如：
  - Professional: `C:/Program Files/Microsoft Visual Studio/2022/Professional`
  - Build Tools: `C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools`

## Linux (vcpkg)

1. 设置环境变量 `VCPKG_ROOT` 指向 vcpkg 根目录。
2. 使用 CMake 预设 `linux-vcpkg-static` 配置并构建。

若出现 **"Permission denied" / "Failed to take the filesystem lock"**（例如对 `/usr/local/vcpkg/.vcpkg-root`）：

- 说明当前使用的 vcpkg 目录（由 `VCPKG_ROOT` 指定）对当前用户不可写，vcpkg 无法在该目录下加锁。
- **做法**：使用本用户有写权限的 vcpkg，例如在用户目录克隆一份，并让 `VCPKG_ROOT` 指向该路径：

  ```bash
  git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
  export VCPKG_ROOT=~/vcpkg
  cmake --preset linux-vcpkg-static
  ```
