# nany_add_server_module(<TargetName>)
# Expects sources in CMAKE_CURRENT_SOURCE_DIR; produces SHARED lib next to GameApp.
function(nany_add_server_module TargetName)
    file(GLOB _srcs CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
    add_library(${TargetName} SHARED ${_srcs})
    target_include_directories(${TargetName} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include/gamma
        ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${TargetName} PRIVATE GammaCommon GammaApp)
    set_target_properties(${TargetName} PROPERTIES
        FOLDER "module"
        OUTPUT_NAME "${TargetName}"
    )
    if(WIN32)
        set_target_properties(${TargetName} PROPERTIES PREFIX "")
    endif()
    # Ensure same output dirs as root project already set via LIBRARY_OUTPUT_PATH
endfunction()
