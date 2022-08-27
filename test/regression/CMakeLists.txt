set(TESTS
  test-cards.sh
  test-lm.sh
  test-main.sh
  test-align.sh
  test-tidigits-fsg.sh
  test-tidigits-simple.sh
)
foreach(TEST ${TESTS})
  if(${TEST} MATCHES "\.(test|sh)$")
    add_test(NAME ${TEST} COMMAND ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/${TEST})
  else()
    add_test(NAME ${TEST} COMMAND ${TEST})
  endif()
  set_property(TEST ${TEST} PROPERTY ENVIRONMENT CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR})
endforeach()
