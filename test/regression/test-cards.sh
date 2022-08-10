#!/bin/bash

: ${CMAKE_BINARY_DIR:=$(pwd)}
. ${CMAKE_BINARY_DIR}/test/testfuncs.sh

bn=`basename $0 .sh`

echo "Test: $bn"
run_program pocketsphinx_batch \
    -loglevel INFO \
    -hmm $model/en-us/en-us \
    -jsgf $data/cards/cards.gram \
    -dict $model/en-us/cmudict-en-us.dict\
    -ctl $data/cards/cards.fileids \
    -bestpath no \
    -adcin yes \
    -cepdir $data/cards \
    -cepext .wav \
    -hyp $bn.match \
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
$tests/word_align.pl -i $data/cards/cards.transcription $bn.match | grep 'TOTAL Percent'
compare_table "match" $data/cards/cards.hyp $bn.match 1000000
