# Set SWIG policies
cmake_policy(SET CMP0078 NEW)
cmake_policy(SET CMP0086 NEW)
find_package(SWIG 4.0)
if(SWIG_FOUND)
  set(UseSWIG_MODULE_VERSION 2)
  # mysteriously necessary
  include(${SWIG_USE_FILE})
  add_subdirectory(python)
endif()

set_property(
    SOURCE pocketsphinx.i
    PROPERTY SWIG_DEPENDS
    ps_decoder.i ps_lattice.i typemaps.i iterators.i
    cmd_ln.i fe.i feat.i fsg_model.i jsgf.i ngram_model.i logmath.i)    
