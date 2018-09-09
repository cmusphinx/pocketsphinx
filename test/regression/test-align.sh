#!/bin/sh

. ../testfuncs.sh

bn=`basename $0 .sh`

echo "Test: $bn"
run_program pocketsphinx_batch \
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
    -hypseg $bn.matchseg \
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
