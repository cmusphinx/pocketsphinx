set(TEST_EXECUTABLES
  strtest
  test_atof
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
  set_property(TARGET ${TEST_EXECUTABLE}
    PROPERTY COMPILE_DEFINITIONS FILEDIR="${CMAKE_CURRENT_SOURCE_DIR}")
  add_dependencies(check ${TEST_EXECUTABLE})
endforeach()
set(TESTS
  _fread_line.test
  _nextword.test
  _str2words.test
  _string_join.test
  _string_trim.test
  test_atof
)
foreach(TEST ${TESTS})
  if(${TEST} MATCHES "\.(test|sh)$")
    add_test(NAME ${TEST} COMMAND ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/${TEST}
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
  else()
    add_test(NAME ${TEST} COMMAND ${TEST})
  endif()
  set_property(TEST ${TEST} PROPERTY ENVIRONMENT CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR})
endforeach()
