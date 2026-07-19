gamma_add_gtest(test_ModuleLogin
  SOURCES test_LoginAuthStore.cpp
          ${CMAKE_SOURCE_DIR}/source/module/ModuleLogin/CLoginAuthStore.cpp
)
target_include_directories(test_ModuleLogin PRIVATE
  ${CMAKE_SOURCE_DIR}/source/module/ModuleLogin
  ${CMAKE_SOURCE_DIR}/include
)
