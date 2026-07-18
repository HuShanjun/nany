find_package(GTest CONFIG REQUIRED)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/GammaTest.cmake)

add_subdirectory(GammaCommon)
add_subdirectory(GammaNetwork)
add_subdirectory(GammaScript)
add_subdirectory(GammaConnects)
add_subdirectory(GammaDatabase)
# Later tasks append: ...
