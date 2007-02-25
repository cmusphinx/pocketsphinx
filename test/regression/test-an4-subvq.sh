#!/bin/sh

. ../testfuncs.sh

bn=`basename $0 .sh`

echo "Test: $bn"
run_program pocketsphinx_batch \
    -hmm $model/hmm/an4 \
    -feat 1s_c_d_dd \
    -subvq $model/hmm/an4/subvq \
    -lm $model/lm/an4/an4.ug.lm.DMP \
    -dict $model/lm/an4/an4.dic \
    -ctl $data/an4/an4.ctl \
    -cepdir $data/an4 \
    -hyp $bn.match \
    > $bn.log 2>&1

# Test whether it actually completed
if [ $? = 0 ]; then
    pass "run"
else
    fail "run"
fi

# Check the decoding results
grep AVERAGE $bn.log
$tests/word_align.pl -i $data/an4/an4.lsn $bn.match | grep 'TOTAL Percent'
compare_table "match" $data/an4/$bn.match $bn.match 10000
