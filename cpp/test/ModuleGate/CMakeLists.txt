gamma_add_gtest(test_ModuleGate
  SOURCES test_GateSessionStore.cpp
          ${CMAKE_SOURCE_DIR}/source/module/ModuleGate/CGateSessionStore.cpp
)
target_include_directories(test_ModuleGate PRIVATE
  ${CMAKE_SOURCE_DIR}/source/module/ModuleGate
)
