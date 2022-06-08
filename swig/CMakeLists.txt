find_package(SWIG)

if(SWIG_FOUND)
  # mysteriously necessary
  include(${SWIG_USE_FILE})
  add_subdirectory(python)
endif()
