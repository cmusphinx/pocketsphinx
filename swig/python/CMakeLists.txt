find_package(Python3 COMPONENTS Interpreter Development)
set_property(SOURCE ../pocketsphinx.i PROPERTY SWIG_MODULE_NAME pocketsphinx)
swig_add_library(pocketsphinx_python
  LANGUAGE python
  SOURCES
  ../pocketsphinx.i
  )
swig_link_libraries(pocketsphinx_python pocketsphinx Python3::Python)
set_property(SOURCE ../sphinxbase.i PROPERTY SWIG_MODULE_NAME sphinxbase)
swig_add_library(sphinxbase_python
  LANGUAGE python
  SOURCES
  ../sphinxbase.i
  )
swig_link_libraries(sphinxbase_python pocketsphinx Python3::Python)
