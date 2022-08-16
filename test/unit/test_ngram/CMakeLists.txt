set(TEST_EXECUTABLES
  test_lm_read
  test_lm_score
  test_lm_add
  test_lm_casefold
  test_lm_class
  test_lm_set
  test_lm_write
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
  add_test(NAME ${TEST_EXECUTABLE} COMMAND ${TEST_EXECUTABLE})
  set_property(TARGET ${TEST_EXECUTABLE}
    PROPERTY COMPILE_DEFINITIONS LMDIR="${CMAKE_CURRENT_SOURCE_DIR}")
  add_dependencies(check ${TEST_EXECUTABLE})
endforeach()
