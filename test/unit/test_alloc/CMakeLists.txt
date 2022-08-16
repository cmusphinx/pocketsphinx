set(TEST_EXECUTABLES
  test_ckd_alloc
  test_ckd_alloc_catch
  test_ckd_alloc_fail
  test_ckd_alloc_abort
  test_listelem_alloc
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
add_test(NAME test_ckd_alloc COMMAND test_ckd_alloc)
add_test(NAME test_listelem_alloc COMMAND test_listelem_alloc)
add_test(NAME test_ckd_alloc_catch COMMAND test_ckd_alloc_catch)
add_test(NAME test_ckd_alloc_fail COMMAND test_ckd_alloc_fail)
set_property(TEST test_ckd_alloc_fail PROPERTY WILL_FAIL TRUE)
add_test(NAME test_ckd_alloc_abort COMMAND ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/test_ckd_alloc_abort.sh)
set_property(TEST test_ckd_alloc_abort PROPERTY ENVIRONMENT CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR})
