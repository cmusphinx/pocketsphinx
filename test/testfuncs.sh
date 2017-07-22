# Utility functions and parameters for regression tests

# Predefined directories you may need
# Stupid broken CMU Facilities autoconf doesn't do /home/mshah1/sphinx/pocketsphinx
builddir=../".."
sourcedir=../".."
tests=$sourcedir/test
data=$sourcedir/test/data
model=$sourcedir/model
programs="$builddir/src/programs"

# Automatically report failures on exit
failures=""
trap "report_failures" 0

run_program() {
    program="$1"
    shift
    $builddir/libtool --mode=execute "$programs/$program" $@
}

debug_program() {
    program="$1"
    shift
    $builddir/libtool --mode=execute gdb --args "$programs/$program" $@
}

memcheck_program() {
    program="$1"
    shift
    $builddir/libtool --mode=execute valgrind --leak-check=full "$programs/$program" $@
}

pass() {
    title="$1"
    echo "$title PASSED"
}

fail() {
    title="$1"
    echo "$title FAILED"
    failures="$failures,$title"
}

compare_table() {
    title="$1"
    shift
    if perl "$tests/compare_table.pl" $@ | grep SUCCESS >/dev/null 2>&1; then
	pass "$title"
    else
	fail "$title"
    fi 
}

report_failures() {
    if test x"$failures" = x; then
	echo "All sub-tests passed"
	exit 0
    else
	echo "Sub-tests failed:$failures" | sed -e 's/,/ /g'
	exit 1
    fi
}
