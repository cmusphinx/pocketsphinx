#!/bin/sh

. ../testfuncs.sh

bn=`basename $0 .sh`

echo "Test: $bn"
run_program pocketsphinx_batch \
    -hmm $data/tidigits/hmm \
    -fsg $data/tidigits/lm/tidigits.fsg \
    -dict $data/tidigits/lm/tidigits.dic \
    -ctl $data/tidigits/tidigits.ctl \
    -cepdir $data/tidigits \
    -hyp $bn.match \
    -wbeam 1e-48 \
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
