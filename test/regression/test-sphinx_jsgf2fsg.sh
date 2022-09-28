#!/bin/bash

: ${CMAKE_BINARY_DIR:=$(pwd)}
. ${CMAKE_BINARY_DIR}/test/testfuncs.sh

echo "JSGF2FSG TEST"
rules="test.rightRecursion test.nestedRightRecursion test.kleene test.nulltest test.command"

tmpout="test-jsgf2fsg.out"
rm -f $tmpout

JSGF_PATH=$tests/regression
export JSGF_PATH
for r in $rules; do
    run_program pocketsphinx_jsgf2fsg \
	-jsgf $tests/regression/test.gram -toprule $r -fsg $r.out 2>>$tmpout
    compare_table $r $r.out $tests/regression/$r.fsg
done

run_program pocketsphinx_jsgf2fsg \
	    -jsgf $tests/regression/right_recursion_53.gram -fsg right_recursion_53.fsg 2>>$tmpout
compare_table right_recursion_53 right_recursion_53.fsg $tests/regression/right_recursion_53.fsg

