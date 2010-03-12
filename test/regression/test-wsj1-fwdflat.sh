#!/bin/sh

. ../testfuncs.sh

bn=`basename $0 .sh`

echo "Test: $bn"
run_program pocketsphinx_batch \
    -hmm $model/hmm/en_US/wsj_sc_8k \
    -lm $model/lm/en_US/wsj0vp.5000.DMP \
    -dict $model/lm/en_US/cmu07a.dic \
    -ctl $data/wsj/test5k.s1.ctl -ctlcount 3 \
    -cepdir $data/wsj \
    -cepext .mfc \
    -hyp $bn.match \
    -fwdtree FALSE \
    -fwdflat TRUE \
    -bestpath FALSE \
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
head -n 3 $data/wsj/test5k.s1.lsn | $tests/word_align.pl -i - $bn.match | grep 'TOTAL Percent'
compare_table "match" $data/wsj/$bn.match $bn.match 1000000
