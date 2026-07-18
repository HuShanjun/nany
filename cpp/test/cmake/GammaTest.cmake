include(GoogleTest)

# gamma_add_gtest(<target>
#   SOURCES <src>...
#   LIBS <lib>...
#   [RUN_SERIAL]          # set on all discovered tests
# )
function(gamma_add_gtest target_name)
  set(options RUN_SERIAL)
  set(oneValueArgs)
  set(multiValueArgs SOURCES LIBS)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT ARG_SOURCES)
    message(FATAL_ERROR "gamma_add_gtest(${target_name}): SOURCES required")
  endif()

  add_executable(${target_name} ${ARG_SOURCES})
  target_include_directories(${target_name} PRIVATE
    ${CMAKE_SOURCE_DIR}/test/common
    ${CMAKE_SOURCE_DIR}/test
  )
  target_link_libraries(${target_name} PRIVATE
    GTest::gtest_main
    ${ARG_LIBS}
  )
  set_target_properties(${target_name} PROPERTIES FOLDER "test")

  set(_serial_props "")
  if(ARG_RUN_SERIAL)
    set(_serial_props "RUN_SERIAL;TRUE")
  endif()

  # Suite naming convention: *_Unit / *_Integration
  gtest_discover_tests(${target_name}
    TEST_FILTER "*_Unit.*"
    TEST_PREFIX "${target_name}."
    PROPERTIES LABELS "unit" ${_serial_props}
    DISCOVERY_TIMEOUT 60
  )
  gtest_discover_tests(${target_name}
    TEST_FILTER "*_Integration.*"
    TEST_PREFIX "${target_name}."
    PROPERTIES LABELS "integration" ${_serial_props}
    DISCOVERY_TIMEOUT 60
  )
endfunction()
