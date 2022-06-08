configure_file(testfuncs.sh.in testfuncs.sh @ONLY)

add_subdirectory(regression)
add_subdirectory(unit)
