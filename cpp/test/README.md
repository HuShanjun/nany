# nany-cpp 测试

依赖：vcpkg `gtest`。构建开关：`NANY_BUILD_TESTS`（默认 ON）。

```bash
cmake --preset linux-vcpkg-static
cmake --build build/linux
ctest --test-dir build/linux -L unit --output-on-failure
```

详见后续完整说明（集成 DB 配置等）。
