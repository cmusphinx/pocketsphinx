#!/bin/sh

. ../testfuncs.sh

bn=`basename $0 .sh`

echo "Test: $bn"
run_program pocketsphinx_batch \
    -hmm $model/hmm/wsj1 \
    -lmctlfn $data/test.lmctl \
    -lmname turtle \
    -dict $data/wsj/wlist5o.nvp.dict \
    > $bn.log 2>&1

# Test whether it actually completed
if [ $? = 0 ]; then
    pass "run"
else
    fail "run"
fi

# Make sure that the language model initialization worked
if grep -q 'Reading LM control file.*test\.lmctl' $bn.log; then
    pass "read lmctl"
else
    fail "read lmctl"
fi
if grep -q 'ngrams 1=5001' $bn.log; then
    pass "read swb"
else
    fail "read swb"
fi
if grep -q 'ngrams 1=14' $bn.log; then
    pass "read tidigits"
else
    fail "read tidigits"
fi
if grep -q 'ngrams 1=91' $bn.log; then
    pass "read turtle"
else
    fail "read turtle"
fi
