#!/bin/bash

: ${CMAKE_BINARY_DIR:=$(pwd)}
. ${CMAKE_BINARY_DIR}/test/testfuncs.sh

bn=`basename $0 .sh`

echo "Test: $bn"
run_program pocketsphinx_batch \
    -hmm $data/tidigits/hmm \
    -lm $data/tidigits/lm/tidigits.lm.bin \
    -dict $data/tidigits/lm/tidigits.dic \
    -ctl $data/tidigits/tidigits.ctl \
    -cepdir $data/tidigits \
    -hyp $bn.match \
    -loglevel INFO \
    > $bn.log 2>&1

# Test whether it actually completed
if [ $? = 0 ]; then
    pass "run"
else
    fail "run"
fi

# Check the decoding results
grep AVERAGE $bn.log
$tests/word_align.pl -i $data/tidigits/tidigits.lsn $bn.match | grep 'TOTAL Percent'
compare_table "match" $data/tidigits/$bn.match $bn.match 100000
