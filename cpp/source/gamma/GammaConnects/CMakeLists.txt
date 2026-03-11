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
	add_library(${ProjectName} STATIC ${SOURCES_})
else(WIN32)
	add_definitions(-DGAMMA_DLL)
	add_library(${ProjectName} SHARED ${SOURCES_})
	target_link_libraries(${ProjectName} GammaCommon GammaNetwork)
	include(${CMAKE_SOURCE_DIR}/common.cmake)
endif(WIN32)

set_target_properties(${ProjectName} PROPERTIES FOLDER "lib")


