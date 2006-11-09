#!/bin/sh

. ../testfuncs.sh

bn=`basename $0 .sh`

echo "Test: $bn"
run_program pocketsphinx_batch \
    -hmm $model/hmm/wsj0 \
    -lm $data/wsj/wlist5o.nvp.lm.DMP \
    -dict $data/wsj/wlist5o.nvp.dict \
    -ctl $data/wsj/test5k.s1.ctl \
    -cepdir $data/wsj \
    -hyp $bn.match \
    -fwdtree TRUE \
    -fwdflat FALSE \
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
$tests/word_align.pl -i $data/wsj/test5k.s1.lsn $bn.match | grep 'TOTAL Percent'
compare_table "match" $data/wsj/$bn.match $bn.match
