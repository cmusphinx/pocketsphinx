#!/bin/bash

: ${CMAKE_BINARY_DIR:=$(pwd)}
. ${CMAKE_BINARY_DIR}/test/testfuncs.sh

bn=`basename $0 .sh`
ctl=$bn.ctl

echo "Test: $bn"

printf 'goforward 0 -1 gof_first\nnumbers 0 -1 num\nsomething 0 -1 som\ngoforward 0 -1 gof_again\n' >$ctl
rm -f $bn.hyp

run_program pocketsphinx_batch \
    -hmm $model/en-us/en-us \
    -lm $model/en-us/en-us.lm.bin \
    -dict $model/en-us/cmudict-en-us.dict \
    -ctl $ctl \
    -cepdir $data \
    -cepext .raw \
    -adcin yes \
    -hyp $bn.hyp \
    2>$bn.log

if [ $? = 0 ]; then
    pass "run"
else
    fail "run"
fi

first=`grep ' (gof_first ' $bn.hyp 2>/dev/null | sed 's/ (gof_first / (gof /'`
again=`grep ' (gof_again ' $bn.hyp 2>/dev/null | sed 's/ (gof_again / (gof /'`

if test -n "$first" && test "$first" = "$again"; then
    pass "repeated entry"
else
    fail "repeated entry"
fi
