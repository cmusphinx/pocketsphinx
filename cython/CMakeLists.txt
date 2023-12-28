find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)
find_program(CYTHON "cython")

if(NOT USE_INSTALLED_POCKETSPHINX)
  set_property(TARGET pocketsphinx PROPERTY POSITION_INDEPENDENT_CODE on)
endif()

add_custom_command(
  OUTPUT _pocketsphinx.c
  DEPENDS _pocketsphinx.pyx
  VERBATIM
  COMMAND "${CYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/_pocketsphinx.pyx" --output-file
          "${CMAKE_CURRENT_BINARY_DIR}/_pocketsphinx.c")

python_add_library(_pocketsphinx MODULE "${CMAKE_CURRENT_BINARY_DIR}/_pocketsphinx.c" WITH_SOABI)
target_link_libraries(_pocketsphinx PRIVATE pocketsphinx)
target_include_directories(
  _pocketsphinx PRIVATE ${PYTHON_INCLUDE_DIR}
  _pocketsphinx PRIVATE ${CMAKE_BINARY_DIR}
  _pocketsphinx PRIVATE ${CMAKE_SOURCE_DIR}/src
  _pocketsphinx PUBLIC ${CMAKE_SOURCE_DIR}/include
  _pocketsphinx PUBLIC ${CMAKE_BINARY_DIR}/include
  _pocketsphinx INTERFACE ${CMAKE_SOURCE_DIR}/include
  _pocketsphinx INTERFACE ${CMAKE_BINARY_DIR}/include
  )
install(TARGETS _pocketsphinx LIBRARY DESTINATION pocketsphinx)
if(NOT USE_INSTALLED_POCKETSPHINX)
  install(DIRECTORY ${PROJECT_SOURCE_DIR}/model DESTINATION pocketsphinx)
endif()
