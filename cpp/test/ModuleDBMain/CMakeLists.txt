gamma_add_gtest(test_ModuleDBMain
  SOURCES test_DBMainJobStore.cpp
          ${CMAKE_SOURCE_DIR}/source/module/ModuleDBMain/CDBMainJobStore.cpp
)
target_include_directories(test_ModuleDBMain PRIVATE
  ${CMAKE_SOURCE_DIR}/source/module/ModuleDBMain
  ${CMAKE_SOURCE_DIR}/include
)
