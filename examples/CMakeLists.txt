set(EXAMPLES
  live
  simple
  )
foreach(EXAMPLE ${EXAMPLES})
  add_executable(${EXAMPLE} EXCLUDE_FROM_ALL ${EXAMPLE}.c)
  target_link_libraries(${EXAMPLE} pocketsphinx)
  target_include_directories(
    ${EXAMPLE} PRIVATE ${CMAKE_BINARY_DIR}
    )
endforeach()

find_library(PORTAUDIO_LIBRARY portaudio)
if(PORTAUDIO_LIBRARY)
  add_executable(live_portaudio EXCLUDE_FROM_ALL live_portaudio.c)
  target_link_libraries(live_portaudio pocketsphinx ${PORTAUDIO_LIBRARY})
  target_include_directories(live_portaudio PRIVATE ${CMAKE_BINARY_DIR})
endif()

add_custom_target(examples DEPENDS ${EXAMPLES})
