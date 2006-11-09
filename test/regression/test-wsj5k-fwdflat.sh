#!/bin/sh

. ../testfuncs.sh

bn=`basename $0 .sh`

echo "Test: $bn"
run_program pocketsphinx_batch \
    -hmm $model/hmm/wsj0 \
    -lm $data/wsj/wlist5o.nvp.lm.DMP \
    -dict $data/wsj/wlist5o.nvp.dict \
    -ctl $data/wsj/test5k.s1.ctl \
    -ctlcount 2 \
    -cepdir $data/wsj \
    -hyp $bn.match \
    -fwdtree FALSE \
    -fwdflat TRUE \
    -bestpath FALSE \
    > $bn.log 2>&1

# Test whether it actually completed
if [ $? = 0 ]; then
    pass "run"
else
    fail "run"
fi

# Check the decoding results
grep AVERAGE $bn.log
head -n2  $data/wsj/test5k.s1.lsn | $tests/word_align.pl -i - $bn.match | grep 'TOTAL Percent'
compare_table "match" $data/wsj/$bn.match $bn.match
