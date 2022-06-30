set(POCKETSPHINX_PROGRAMS
  pocketsphinx_batch
  pocketsphinx_mdef_convert
  pocketsphinx_jsgf2fsg
  pocketsphinx_lm_convert
  pocketsphinx_lm_eval
  pocketsphinx_pitch
  )
foreach(PROGRAM ${POCKETSPHINX_PROGRAMS})
  add_executable(${PROGRAM} ${PROGRAM}.c)
  target_link_libraries(${PROGRAM} pocketsphinx)
  target_include_directories(
    ${PROGRAM} PRIVATE ${CMAKE_SOURCE_DIR}/src
    ${PROGRAM} PRIVATE ${CMAKE_BINARY_DIR})
endforeach()
install(TARGETS ${POCKETSPHINX_PROGRAMS} RUNTIME)
