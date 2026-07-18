gamma_add_gtest(test_GameApp
  SOURCES test_TomlConfig.cpp test_ProcessModuleToml.cpp
)
target_compile_definitions(test_GameApp PRIVATE NANY_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
