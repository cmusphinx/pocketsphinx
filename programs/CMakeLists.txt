set(POCKETSPHINX_PROGRAMS
  pocketsphinx_main
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
# CMake and its lovely flat namespace
set_target_properties(pocketsphinx_main PROPERTIES OUTPUT_NAME pocketsphinx)

# The cmake docs indicate you should check MACOSX_BUNDLE
# and that MACOSX_BUNDLE is initialized based on the value
# of CMAKE_MACOSX_BUNDLE but in practice, MACOSX_BUNDLE
# seems blank regardless of the value of CMAKE_MACOSX_BUNDLE.
# Since MACOSX_BUNDLE seems intended to just forward the value
# of CMAKE_MACOSX_BUNDLE anyway, we check CMAKE_MACOSX_BUNDLE
# directly.
if(CMAKE_MACOSX_BUNDLE)
  install(TARGETS ${POCKETSPHINX_PROGRAMS}
          RUNTIME DESTINATION bin
          BUNDLE DESTINATION bin
  )
else()
  install(TARGETS ${POCKETSPHINX_PROGRAMS} RUNTIME)
endif()
