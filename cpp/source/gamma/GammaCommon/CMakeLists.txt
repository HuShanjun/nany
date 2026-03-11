file (GLOB SOURCES_HeadFiles "*.h" "./*/*.h")
file (GLOB SOURCES_SourceFiles "*.cpp" "*.c" "./*/*.cpp" "./*/*.c") 

string(REGEX REPLACE ".*/\(.*\)" "\\1" ProjectName ${CMAKE_CURRENT_SOURCE_DIR})
file (GLOB SOURCES_InterfaceFiles "../../inc/${ProjectName}/*.h" "../../inc/${ProjectName}/*/*.h") 
source_group("Interface Files" FILES ${SOURCES_InterfaceFiles})

set(SOURCES_
	${SOURCES_SourceFiles}
	${SOURCES_HeadFiles}
	${SOURCES_InterfaceFiles}
)

if(WIN32)
	add_definitions(-DGAMMA_COMMON_EXPORTS)
	add_library(${ProjectName} SHARED ${SOURCES_})
else(WIN32)
	add_definitions(-DGAMMA_COMMON_EXPORTS)
	add_library(${ProjectName} SHARED ${SOURCES_})
endif(WIN32)

set_target_properties(${ProjectName} PROPERTIES FOLDER "lib")
include(${CMAKE_SOURCE_DIR}/common.cmake)
