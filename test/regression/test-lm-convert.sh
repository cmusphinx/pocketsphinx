#!/bin/bash

: ${CMAKE_BINARY_DIR:=$(pwd)}
. ${CMAKE_BINARY_DIR}/test/testfuncs.sh

bn=`basename $0 .sh`

echo "Test: $bn"

run_program pocketsphinx_lm_convert \
            -i $data/turtle.lm.bin \
            -o $bn.arpa \
            > $bn.log 2>&1
if [ $? = 0 ]; then
    pass "bin -> arpa"
else
    fail "bin -> arpa"
fi

run_program pocketsphinx_lm_convert \
            -i $bn.arpa \
            -o $bn.bin \
            > $bn.log 2>&1
if [ $? = 0 ]; then
    pass "arpa -> bin"
else
    fail "arpa -> bin"
fi

run_program pocketsphinx_lm_convert \
            -i $bn.arpa \
            -o $bn.dmp \
            > $bn.log 2>&1
if [ $? = 0 ]; then
    pass "arpa -> dmp"
else
    fail "arpa -> dmp"
fi

run_program pocketsphinx_lm_convert \
            -i $bn.dmp \
            -o $bn.bin \
            > $bn.log 2>&1
if [ $? = 0 ]; then
    pass "dmp -> bin"
else
    fail "dmp -> bin"
fi

for evil in not-enough-ngrams too-many-ngrams; do
    run_program pocketsphinx_lm_convert \
                -i $data/$evil.arpa \
                -o $bn.bin \
                > $bn.log 2>&1
    # Expect failure
    if [ $? = 0 ]; then
        fail "$evil.arpa -> bin"
    elif grep -q order $bn.log; then
        pass "$evil.arpa -> bin"
    else
        fail "$evil.arpa -> bin"
    fi
    run_program pocketsphinx_lm_convert \
                -i $data/$evil.lm.bin \
                -o $bn.arpa \
                > $bn.log 2>&1
    # Expect failure and an error
    if [ $? = 0 ]; then
        fail "$evil.bin -> arpa"
    elif grep -q order $bn.log; then
        pass "$evil.bin -> arpa"
    else
        fail "$evil.bin -> arpa"
    fi
done

