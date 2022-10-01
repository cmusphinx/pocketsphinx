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
                > $utt.json 2>>$bn.log
    run_program pocketsphinx \
                -loglevel INFO \
                -phone_align yes \
                -hmm $model/en-us/en-us \
                -dict $model/en-us/cmudict-en-us.dict \
                align $wav $(cat $data/librivox/$utt.txt) \
                > $utt.phone.json 2>>$bn.phone.log
    run_program pocketsphinx \
                -loglevel INFO \
                -state_align yes \
                -hmm $model/en-us/en-us \
                -dict $model/en-us/cmudict-en-us.dict \
                align $wav $(cat $data/librivox/$utt.txt) \
                > $utt.state.json 2>>$bn.state.log

    # Test whether it actually completed
    if [ $? = 0 ]; then
        pass "run"
    else
        fail "run"
    fi
    # Check the decoding results
    compare_table "match" $data/librivox/$utt.json $utt.json 1000000
    compare_table "match" $data/librivox/$utt.phone.json $utt.phone.json 1000000
    compare_table "match" $data/librivox/$utt.state.json $utt.state.json 1000000
done

run_program pocketsphinx \
            -loglevel INFO \
            -hmm $model/en-us/en-us \
            -lm $model/en-us/en-us.lm.bin \
            -dict $model/en-us/cmudict-en-us.dict align $data/null.wav " " \
            2>>$bn.log >null-align.json
compare_table "match" $data/null-align.json null-align.json 1000000
