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
find_package(PkgConfig QUIET)
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

# Try to find portaudio with its old package finder thing
if(NOT PORTAUDIO_FOUND)
  find_package(portaudio QUIET)
  if(TARGET portaudio_static)
    add_executable(live_portaudio EXCLUDE_FROM_ALL live_portaudio.c)
    target_link_libraries(live_portaudio pocketsphinx portaudio_static)
    set(PORTAUDIO_FOUND 1)
  endif()
endif()

# Try to find portaudio with its new package finder thing
if(NOT PORTAUDIO_FOUND)
  find_package(PortAudio QUIET)
  if(TARGET PortAudio::PortAudio)
    add_executable(live_portaudio EXCLUDE_FROM_ALL live_portaudio.c)
    target_link_libraries(live_portaudio pocketsphinx PortAudio::PortAudio)
    set(PORTAUDIO_FOUND 1)
  endif()
endif()

if(WIN32)
  add_executable(live_win32 EXCLUDE_FROM_ALL live_win32.c)
  target_link_libraries(live_win32 pocketsphinx winmm)
endif()