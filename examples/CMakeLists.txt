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

add_custom_target(examples DEPENDS ${EXAMPLES})

# Try to find portaudio and pulseaudio with pkg-config
find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PULSEAUDIO libpulse-simple)
  if(PULSEAUDIO_FOUND)
    add_executable(live_pulseaudio EXCLUDE_FROM_ALL live_pulseaudio.c)
    target_link_libraries(live_pulseaudio pocketsphinx ${PULSEAUDIO_LIBRARIES})
    target_include_directories(live_pulseaudio PRIVATE ${CMAKE_BINARY_DIR}
      live_pulseaudio PUBLIC ${PULSEAUDIO_INCLUDE_DIRS})
  endif()

  pkg_check_modules(PORTAUDIO portaudio-2.0)
  if(PORTAUDIO_FOUND)
    add_executable(live_portaudio EXCLUDE_FROM_ALL live_portaudio.c)
    target_link_libraries(live_portaudio pocketsphinx ${PORTAUDIO_LIBRARIES})
    target_include_directories(live_portaudio PRIVATE ${CMAKE_BINARY_DIR}
      live_portaudio PUBLIC ${PORTAUDIO_INCLUDE_DIRS})
  endif()
endif()
# Otherwise ... you're on your own, sorry, because PortAudio's claim
# to include a CMake configuration is blatantly false.
