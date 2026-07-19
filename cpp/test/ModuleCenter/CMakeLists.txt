gamma_add_gtest(test_ModuleCenter
  SOURCES test_CenterRegistryStore.cpp
          ${CMAKE_SOURCE_DIR}/source/module/ModuleCenter/CCenterRegistryStore.cpp
)
target_include_directories(test_ModuleCenter PRIVATE
  ${CMAKE_SOURCE_DIR}/source/module/ModuleCenter
  ${CMAKE_SOURCE_DIR}/include
)
