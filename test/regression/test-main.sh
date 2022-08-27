#!/bin/bash

: ${CMAKE_BINARY_DIR:=$(pwd)}
. ${CMAKE_BINARY_DIR}/test/testfuncs.sh

bn=`basename $0 .sh`

echo "Test: $bn"
sox $data/librivox/*.wav $(run_program pocketsphinx soxflags) | \
    run_program pocketsphinx \
                -hmm $model/en-us/en-us \
                -lm $model/en-us/en-us.lm.bin \
                -dict $model/en-us/cmudict-en-us.dict \
                > $bn.json

# Test whether it actually completed
if [ $? = 0 ]; then
    pass "run"
else
    fail "run"
fi

# Check the decoding results
compare_table "match" $data/librivox/$bn.json $bn.json 1000000
