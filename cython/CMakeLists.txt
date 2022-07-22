find_package(PythonExtensions REQUIRED)
find_package(Python COMPONENTS Interpreter Development)
find_package(Cython)

set_property(TARGET pocketsphinx PROPERTY POSITION_INDEPENDENT_CODE on)

add_cython_target(_pocketsphinx _pocketsphinx.pyx)
add_library(_pocketsphinx MODULE ${_pocketsphinx})
target_link_libraries(_pocketsphinx pocketsphinx)
target_include_directories(
  _pocketsphinx PRIVATE ${PYTHON_INCLUDE_DIR}
  _pocketsphinx PRIVATE ${PROJECT_SOURCE_DIR}/src
  _pocketsphinx PRIVATE ${PROJECT_SOURCE_DIR}/include
  _pocketsphinx PRIVATE ${CMAKE_BINARY_DIR} # for config.h
  )
python_extension_module(_pocketsphinx)
install(TARGETS _pocketsphinx LIBRARY DESTINATION cython/pocketsphinx5)
