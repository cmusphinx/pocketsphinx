find_package(PythonExtensions REQUIRED)
find_package(Python COMPONENTS Interpreter Development)
find_package(Cython)

if(NOT USE_INSTALLED_POCKETSPHINX)
  set_property(TARGET pocketsphinx PROPERTY POSITION_INDEPENDENT_CODE on)
endif()

add_cython_target(_pocketsphinx _pocketsphinx.pyx)
add_library(_pocketsphinx MODULE ${_pocketsphinx})
target_link_libraries(_pocketsphinx pocketsphinx)
target_include_directories(
  _pocketsphinx PRIVATE ${PYTHON_INCLUDE_DIR}
  _pocketsphinx PRIVATE ${CMAKE_BINARY_DIR}
  _pocketsphinx PRIVATE ${CMAKE_SOURCE_DIR}/src
  _pocketsphinx PUBLIC ${CMAKE_SOURCE_DIR}/include
  _pocketsphinx PUBLIC ${CMAKE_BINARY_DIR}/include
  _pocketsphinx INTERFACE ${CMAKE_SOURCE_DIR}/include
  _pocketsphinx INTERFACE ${CMAKE_BINARY_DIR}/include
  )
python_extension_module(_pocketsphinx)
install(TARGETS _pocketsphinx LIBRARY DESTINATION cython/pocketsphinx)
if(NOT USE_INSTALLED_POCKETSPHINX)
  install(DIRECTORY ${PROJECT_SOURCE_DIR}/model DESTINATION cython/pocketsphinx)
endif()
