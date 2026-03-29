cmake_minimum_required(VERSION 3.19)
string(REGEX REPLACE "/$" "" CURRENT_FOLDER_ABSOLUTE ${CMAKE_CURRENT_SOURCE_DIR})
string(REGEX REPLACE ".*/(.*)" "\\1" CURRENT_FOLDER ${CURRENT_FOLDER_ABSOLUTE})
set(ProjectName  ${CURRENT_FOLDER}Sample)
project(${ProjectName})

file(GLOB_RECURSE SRC_Files ./*.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRC_Files})

add_executable(${ProjectName} ${SRC_Files})
target_link_libraries(${ProjectName} GammaCommon GammaDatabase)

set_target_properties(${ProjectName} PROPERTIES FOLDER "sample")
