configure_file(test_macros.h.in test_macros.h @ONLY)

# Build thread utilities as a library
add_library(test_thread_utils STATIC test_thread_utils.c)
target_include_directories(test_thread_utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set(TESTS
  test_acmod
  test_acmod_grow
  test_alignment
  test_allphone
  test_bitvec
  test_config
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
  test_ngram_model_read
  test_log_shifted
  test_log_int8
  test_log_int16
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
  test_vad
  test_word_align
  test_endpointer
  test_thread_local_compile
  test_thread_local_basic
  test_thread_utils_self
  test_genrand_baseline
  test_genrand_thread
  test_genrand_thread_tls
  )
foreach(TEST_EXECUTABLE ${TESTS})
  add_executable(${TEST_EXECUTABLE} EXCLUDE_FROM_ALL ${TEST_EXECUTABLE}.c)
  target_link_libraries(${TEST_EXECUTABLE} pocketsphinx)
  target_include_directories(
    ${TEST_EXECUTABLE} PRIVATE ${CMAKE_SOURCE_DIR}/src
    ${TEST_EXECUTABLE} PRIVATE ${CMAKE_BINARY_DIR}
    ${TEST_EXECUTABLE} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
    )
  add_test(NAME ${TEST_EXECUTABLE} COMMAND ${TEST_EXECUTABLE})
  add_dependencies(check ${TEST_EXECUTABLE})
endforeach()

# Some tests need thread utilities and pthread support
if(NOT WIN32)
  find_package(Threads REQUIRED)
  target_link_libraries(test_thread_local_basic ${CMAKE_THREAD_LIBS_INIT})
  target_link_libraries(test_thread_utils ${CMAKE_THREAD_LIBS_INIT})
endif()

# test_thread_utils_self needs the thread utilities library
target_link_libraries(test_thread_utils_self test_thread_utils)

# test_genrand_thread needs thread utilities
target_link_libraries(test_genrand_thread test_thread_utils)
target_link_libraries(test_genrand_thread_tls test_thread_utils)

add_subdirectory(test_alloc)
add_subdirectory(test_case)
add_subdirectory(test_feat)
add_subdirectory(test_fsg)
add_subdirectory(test_hash)
add_subdirectory(test_lineiter)
add_subdirectory(test_matrix)
add_subdirectory(test_ngram)
add_subdirectory(test_string)
add_subdirectory(test_util)
