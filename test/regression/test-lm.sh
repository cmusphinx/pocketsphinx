#!/bin/bash

: ${CMAKE_BINARY_DIR:=$(pwd)}
. ${CMAKE_BINARY_DIR}/test/testfuncs.sh

bn=`basename $0 .sh`

echo "Test: $bn"
run_program pocketsphinx_batch \
    -hmm $model/en-us/en-us \
    -lm $model/en-us/en-us.lm.bin \
    -dict $model/en-us/cmudict-en-us.dict \
    -ctl $data/librivox/fileids \
    -cepdir $data/librivox \
    -cepext .wav \
    -adcin yes \
    -hyp $bn.match \
    -loglevel INFO \
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
$tests/word_align.pl $data/librivox/transcription $bn.match | grep 'TOTAL Percent'
compare_table "match" $data/librivox/$bn.match $bn.match 1000000
