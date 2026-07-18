cmake_minimum_required(VERSION 3.20)
project(pystring)

set(PYSTRING_SOURCES
    pystring.cpp
)

add_library(pystring STATIC ${PYSTRING_SOURCES})

target_include_directories(pystring PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})