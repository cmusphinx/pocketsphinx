cmake_minimum_required(VERSION 3.13)

project(PocketSphinx VERSION 1.0.0
  DESCRIPTION "A small speech recognizer"
  HOMEPAGE_URL "https://github.com/dhdaines/pocketsphinx")
include(CMakePrintHelpers)
set(PACKAGE_NAME ${PROJECT_NAME})
string(TOLOWER ${PROJECT_NAME} PROJECT_SHORTNAME)
set(PACKAGE_VERSION ${PROJECT_VERSION})
set(PACKAGE_STRING "${PROJECT_NAME} ${PROJECT_VERSION}")
set(PACKAGE_TARNAME "${PROJECT_SHORTNAME}-${PROJECT_VERSION}")
set(PACKAGE_URL ${PROJECT_HOMEPAGE_URL})
set(PACKAGE_BUGREPORT dhdaines@gmail.com)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  include(CTest)
  enable_testing()
endif()

include(CheckTypeSize)
include(CheckSymbolExists)
include(CheckLibraryExists)
include(TestBigEndian)

CHECK_INCLUDE_FILE(unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILE(sys/types.h HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILE(sys/stat.h HAVE_SYS_STAT_H)
CHECK_SYMBOL_EXISTS(snprintf stdio.h HAVE_SNPRINTF)
CHECK_SYMBOL_EXISTS(popen stdio.h HAVE_POPEN)
CHECK_TYPE_SIZE(long LONG)
CHECK_TYPE_SIZE("long long" LONG_LONG)
# OMG CMake is so incredibly awful
set(SIZEOF_LONG ${LONG})
set(SIZEOF_LONG_LONG ${LONG_LONG})
cmake_print_variables(SIZEOF_LONG SIZEOF_LONG_LONG)
test_big_endian(WORDS_BIGENDIAN)
cmake_print_variables(WORDS_BIGENDIAN)

if(CMAKE_BUILD_TYPE STREQUAL Debug)
  set(SPHINX_DEBUG 1)
endif()

if(MSVC)
  add_compile_options(/W3)
else()
  add_compile_options(-Wall)
endif()

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
option(FIXED_POINT "Build using fixed-point math" OFF)
if(NOT DEFAULT_RADIX)
  set(DEFAULT_RADIX 12)
endif()
cmake_print_variables(FIXED_POINT DEFAULT_RADIX)

configure_file(config.h.in config.h)
configure_file(sphinx_config.h.in include/sphinxbase/sphinx_config.h)
add_definitions(-DHAVE_CONFIG_H)

add_subdirectory(src)
add_subdirectory(include)
add_subdirectory(programs)
