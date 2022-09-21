#!/bin/bash

set -e

: ${CMAKE_BINARY_DIR:=$(pwd)}
. ${CMAKE_BINARY_DIR}/test/testfuncs.sh

bn=`basename $0 .sh`

echo "Test: $bn"
for wav in $data/librivox/*.wav; do \
    utt=$(basename $wav .wav)
    run_program pocketsphinx \
                -loglevel INFO \
                -hmm $model/en-us/en-us \
                -dict $model/en-us/cmudict-en-us.dict \
                align $wav $(cat $data/librivox/$utt.txt) \
                > $utt.json 2>$bn.log

    # Test whether it actually completed
    if [ $? = 0 ]; then
        pass "run"
    else
        fail "run"
    fi
    # Check the decoding results
    compare_table "match" $data/librivox/$utt.json $utt.json 1000000
done
