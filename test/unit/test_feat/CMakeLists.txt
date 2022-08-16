set(TEST_EXECUTABLES
  test_feat
  test_feat_live
  test_feat_fe
  test_subvq
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
  _test_feat.test
  test_feat_live
  test_feat_fe
  test_subvq
)
foreach(TEST ${TESTS})
  if(${TEST} MATCHES "\.(test|sh)$")
    add_test(NAME ${TEST} COMMAND ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/${TEST})
  else()
    add_test(NAME ${TEST} COMMAND ${TEST})
  endif()
  set_property(TEST ${TEST} PROPERTY ENVIRONMENT CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR})
endforeach()
