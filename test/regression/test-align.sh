#!/bin/bash

: ${CMAKE_BINARY_DIR:=$(pwd)}
. ${CMAKE_BINARY_DIR}/test/testfuncs.sh

bn=`basename $0 .sh`

echo "Test: $bn"
run_program pocketsphinx_batch \
    -loglevel INFO \
    -hmm $model/en-us/en-us \
    -dict $model/en-us/cmudict-en-us.dict \
    -alignctl $data/librivox/fileids \
    -aligndir $data/librivox \
    -alignext .txt \
    -ctl $data/librivox/fileids \
    -cepdir $data/librivox \
    -cepext .wav \
    -adcin yes \
    -adchdr 44 \
    -hyp $bn.align \
    -hypseg $bn.matchseg \
    -bestpath no \
    -backtrace yes \
    > $bn.log 2>&1

# Test whether it actually completed
if [ $? = 0 ]; then
    pass "run"
else
    fail "run"
fi

# Check the decoding results
grep AVERAGE $bn.log
compare_table "matchseg" $data/librivox/test-align.matchseg $bn.matchseg 1000000
compare_table "matchseg" $data/librivox/test-align.align $bn.align 1000000
