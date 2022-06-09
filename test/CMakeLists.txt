find_program(BASH_PROGRAM bash)
configure_file(testfuncs.sh.in testfuncs.sh @ONLY)

add_subdirectory(regression)
add_subdirectory(unit)
