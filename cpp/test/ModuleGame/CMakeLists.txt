gamma_add_gtest(test_ModuleGame
  SOURCES test_GamePlayerStore.cpp
          ${CMAKE_SOURCE_DIR}/source/module/ModuleGame/CGamePlayerStore.cpp
)
target_include_directories(test_ModuleGame PRIVATE
  ${CMAKE_SOURCE_DIR}/source/module/ModuleGame
  ${CMAKE_SOURCE_DIR}/include
)
