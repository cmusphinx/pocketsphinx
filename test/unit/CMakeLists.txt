configure_file(test_macros.h.in test_macros.h @ONLY)
set(TESTS
  test_acmod
  test_acmod_grow
  test_alignment
  test_allphone
  test_bitvec
  test_dict2pid
  test_dict
  test_fe
  test_fwdflat
  test_fwdtree_bestpath
  test_fwdtree
  test_init
  test_jsgf
  test_keyphrase
  test_lattice
  test_lm_read
  test_mllr
  test_nbest
  test_pitch
  test_posterior
  test_ptm_mgau
  test_reinit
  test_senfh
  test_set_search
  test_simple
  test_state_align
  )
foreach(TEST_EXECUTABLE ${TESTS})
  add_executable(${TEST_EXECUTABLE} ${TEST_EXECUTABLE}.c)
  target_link_libraries(${TEST_EXECUTABLE} pocketsphinx)
  target_include_directories(
    ${TEST_EXECUTABLE} PRIVATE ${CMAKE_SOURCE_DIR}/src
    ${TEST_EXECUTABLE} PRIVATE ${CMAKE_BINARY_DIR}
    ${TEST_EXECUTABLE} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
    )
  add_test(${TEST_EXECUTABLE} ${TEST_EXECUTABLE})
endforeach()

add_subdirectory(test_alloc)
add_subdirectory(test_case)
add_subdirectory(test_cmdln)
add_subdirectory(test_fsg)
