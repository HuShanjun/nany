cmake_minimum_required(VERSION 3.19)
set(ProjectName  nany-cpp)
project(${ProjectName})

# 设置编译选项
# set(CMAKE_CXX_FLAGS   "-no-pie")  
# set(CMAKE_C_FLAGS   "-no-pie")
# 设置 C++ 标准为 C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)  # 必须支持指定的标准
set(CMAKE_CXX_EXTENSIONS OFF)        # 禁用编译器特定扩展

# 设置编译选项
if(WIN32)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE -D_SCL_SECURE_NO_WARNINGS -DWIN32_LEAN_AND_MEAN -D_WIN32 -DPLATFORM_WIN32 -DNOMINMAX)
	add_definitions(/we"4715")
	add_definitions(/MP)
	add_definitions(-D_HAS_STD_BYTE=0)
	add_definitions(-DNOMINMAX)
	add_definitions(-DSTRICT)

    set(CMAKE_CXX_FLAGS_DEBUG	"${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -D_DEBUG -DENABLE_LUADEBUG /bigobj")
    set(CMAKE_CXX_FLAGS_RELEASE	"${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG /bigobj")
    include_directories(${CMAKE_SOURCE_DIR}/3rd-libs/msvc/x64-windows-static-md/include)
    set(PLATFORM_NAME "msvc")
else(WIN32)
	#add_definitions(-fvisibility=hidden)
	add_definitions(-Wno-terminate)
	add_definitions(-D_PORT_VERSION -Wno-deprecated -fPIC -std=c++17)
    set(CMAKE_CXX_FLAGS_DEBUG	"${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -D_DEBUG -ggdb")
    set(CMAKE_CXX_FLAGS_RELEASE	"${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -O2 -ggdb")
    set(PLATFORM_NAME "linux")
    set(RELEASE_SUB_DIR "Release")
    set(DEBUG_SUB_DIR "Debug")
endif(WIN32)


# 设置变量
set(THIRD_PARTY_PATH ${CMAKE_SOURCE_DIR}/3rd-src)

#include 路径
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${THIRD_PARTY_PATH}/lua/inc)

# 设置PATH
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin)
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)

# 区分release和debug设置链接lib库路径
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${PLATFORM_NAME}/${RELEASE_SUB_DIR}) # 可执行文件输出路径
    set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${PLATFORM_NAME}/${RELEASE_SUB_DIR}) # 动态库输出路径
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/libs/${PLATFORM_NAME}/${RELEASE_SUB_DIR}) # 静态库输出路径

    # link_directories(${CMAKE_SOURCE_DIR}/libs/Release) # 链接静态库路径
    # link_directories(${CMAKE_SOURCE_DIR}/bin/Release) # 链接动态库路径
else(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${PLATFORM_NAME}/${DEBUG_SUB_DIR}) # 可执行文件输出路径
    set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${PLATFORM_NAME}/${DEBUG_SUB_DIR}) # 动态库输出路径
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/libs/${PLATFORM_NAME}/${DEBUG_SUB_DIR}) # 静态库输出路径

    # link_directories(${CMAKE_SOURCE_DIR}/libs/Debug) # 链接静态库路径
    # link_directories(${CMAKE_SOURCE_DIR}/bin/Debug) # 链接动态库路径
endif(CMAKE_BUILD_TYPE STREQUAL "Release")

# 源码版3rd目录
add_subdirectory(3rd-src/lua)

# 递归枚举目录下所有文件
add_subdirectory(source/GammaCommon)
add_subdirectory(source/GammaDatabase)
add_subdirectory(source/GammaMTDbs)
add_subdirectory(source/GammaNetwork)
add_subdirectory(source/GammaConnects)
add_subdirectory(source/GammaScript)
add_subdirectory(source/GammaShm)


add_subdirectory(sample/main)


