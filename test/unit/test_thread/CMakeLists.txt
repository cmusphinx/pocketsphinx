set(TEST_EXECUTABLES
  test_thread
  test_event
  test_msgq
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
  add_dependencies(check ${TEST_EXECUTABLE})
endforeach()
