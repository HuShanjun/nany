cmake_minimum_required(VERSION 3.19)
#设置项目名称(文件名称即工程名)
string(REGEX REPLACE "/$" "" CURRENT_FOLDER_ABSOLUTE ${CMAKE_CURRENT_SOURCE_DIR})
string(REGEX REPLACE ".*/(.*)" "\\1" CURRENT_FOLDER ${CURRENT_FOLDER_ABSOLUTE})
set(ProjectName  ${CURRENT_FOLDER}Sample)
project(${ProjectName})

# 设置编译选项
# set(CMAKE_CXX_FLAGS   "-no-pie")
# set(CMAKE_C_FLAGS   "-no-pie")

# 递归枚举目录下所有文件
file(GLOB_RECURSE SRC_Files ./*.cpp)
source_group(TREE  ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRC_Files})

add_executable(${ProjectName} ${SRC_Files})
target_link_libraries(${ProjectName} GammaCommon GammaShm)

set_target_properties(${ProjectName} PROPERTIES FOLDER "sample")
