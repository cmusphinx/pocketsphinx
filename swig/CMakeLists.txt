find_package(SWIG REQUIRED)
# mysteriously necessary
include(${SWIG_USE_FILE})

add_subdirectory(python)
