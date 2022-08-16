set(TEST_EXECUTABLES
  cmdln_parse
  cmdln_parse_multiple
  cmdln_parse_r
  )
foreach(TEST_EXECUTABLE ${TEST_EXECUTABLES})
  add_executable(${TEST_EXECUTABLE} EXCLUDE_FROM_ALL ${TEST_EXECUTABLE}.c)
  target_link_libraries(${TEST_EXECUTABLE} pocketsphinx)
  target_include_directories(
    ${TEST_EXECUTABLE} PRIVATE ${CMAKE_SOURCE_DIR}/src
    ${TEST_EXECUTABLE} PRIVATE ${CMAKE_BINARY_DIR}
    ${TEST_EXECUTABLE} PRIVATE ${CMAKE_BINARY_DIR}/test/unit
    ${TEST_EXECUTABLE} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
    )
  add_dependencies(check ${TEST_EXECUTABLE})
endforeach()
set(TESTS
  _test_parse_badargs.test
  _test_parse_defaults_r.test
  _test_parse_defaults.test
  _test_parse_goodargs.test
  _test_parse_multiple.test
)
foreach(TEST ${TESTS})
  add_test(NAME ${TEST} COMMAND ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/${TEST}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
  set_property(TEST ${TEST} PROPERTY ENVIRONMENT CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR})
endforeach()
