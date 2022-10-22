find_package(PkgConfig REQUIRED)
pkg_check_modules(GOBJECT gobject-2.0)
pkg_check_modules(GSTREAMER gstreamer-1.0 gstreamer-base-1.0)
add_library(gstpocketsphinx SHARED gstpocketsphinx.c)
set_property(TARGET pocketsphinx PROPERTY POSITION_INDEPENDENT_CODE on)
target_link_libraries(gstpocketsphinx PUBLIC
  pocketsphinx
  ${GSTREAMER_LIBRARIES}
  ${GOBJECT_LIBRARIES}
  )
target_include_directories(
  gstpocketsphinx PRIVATE ${CMAKE_BINARY_DIR}
  gstpocketsphinx PRIVATE ${CMAKE_SOURCE_DIR}/src
  gstpocketsphinx PUBLIC ${CMAKE_SOURCE_DIR}/include
  gstpocketsphinx PUBLIC ${CMAKE_BINARY_DIR}/include
  gstpocketsphinx INTERFACE ${CMAKE_SOURCE_DIR}/include
  gstpocketsphinx INTERFACE ${CMAKE_BINARY_DIR}/include
  gstpocketsphinx PUBLIC ${GSTREAMER_INCLUDE_DIRS} ${GOBJECT_INCLUDE_DIRS}
  )
message("Installing GStreamer plugin to ${CMAKE_INSTALL_FULL_LIBDIR}/gstreamer-1.0")
install(TARGETS gstpocketsphinx LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/gstreamer-1.0)
