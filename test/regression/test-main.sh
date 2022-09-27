#!/bin/bash

: ${CMAKE_BINARY_DIR:=$(pwd)}
. ${CMAKE_BINARY_DIR}/test/testfuncs.sh

bn=`basename $0 .sh`

echo "Test: $bn"
sox $data/librivox/*.wav $(run_program pocketsphinx soxflags) | \
    run_program pocketsphinx \
                -loglevel INFO \
                -hmm $model/en-us/en-us \
                -lm $model/en-us/en-us.lm.bin \
                -dict $model/en-us/cmudict-en-us.dict - \
                2>$bn.log >$bn.json

# Test whether it actually completed
if [ $? = 0 ]; then
    pass "run"
else
    fail "run"
fi

if grep -q 'define FIXED' config.h; then
   ref="$data/librivox/$bn.fixed.json"
else
   ref="$data/librivox/$bn.json"
fi

# Check the decoding results
compare_table "match" $ref $bn.json 1000000
