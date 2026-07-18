find_package(GTest CONFIG REQUIRED)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/GammaTest.cmake)

add_subdirectory(GammaCommon)
# Later tasks append: GammaNetwork, GammaScript, ...
