set(TEST_EXECUTABLES
  chgCase
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
  _lcase1.test
  _lcase2.test
  _lcase3.test
  _strcmp1.test
  _strcmp2.test
  _strcmp3.test
  _ucase1.test
  _ucase2.test
  _ucase3.test
)
foreach(TEST ${TESTS})
  add_test(NAME ${TEST} COMMAND ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/${TEST}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
endforeach()
