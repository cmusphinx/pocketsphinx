add_executable(pocketsphinx_batch batch.c)
target_link_libraries(pocketsphinx_batch pocketsphinx)
target_include_directories(
  pocketsphinx_batch PRIVATE ${CMAKE_SOURCE_DIR}/src/libpocketsphinx)
add_executable(pocketsphinx_mdef_convert mdef_convert.c)
target_link_libraries(pocketsphinx_mdef_convert pocketsphinx)
target_include_directories(
  pocketsphinx_mdef_convert PRIVATE ${CMAKE_SOURCE_DIR}/src/libpocketsphinx)
