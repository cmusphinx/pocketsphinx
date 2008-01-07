#!/bin/sh

set -e

platform=${1:-i686-winnt}

echo "SVN HEAD:"
./wsj1_test5k.sh $platform-simple ../bin/Release/pocketsphinx_batch
./wsj1_test5k_fast.sh $platform-fast ../bin/Release/pocketsphinx_batch

echo "0.3:"
./wsj1_test5k.sh $platform-simple-base ../../pocketsphinx-0.3/bin/Release/pocketsphinx_batch
./wsj1_test5k_fast.sh $platform-fast-base ../../pocketsphinx-0.3/bin/Release/pocketsphinx_batch
