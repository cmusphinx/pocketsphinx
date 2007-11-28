#!/bin/sh

set -e

platform=${1:-i686-linux}

echo "SVN HEAD:"
./wsj1_test5k.sh $platform-simple ../i686-linux/src/programs/pocketsphinx_batch
./wsj1_test5k_fast.sh $platform-fast ../i686-linux/src/programs/pocketsphinx_batch

echo "0.3:"
./wsj1_test5k.sh $platform-simple-base ../../pocketsphinx-0.3/i686-linux/src/programs/pocketsphinx_batch
./wsj1_test5k_fast.sh $platform-fast-base ../../pocketsphinx-0.3/i686-linux/src/programs/pocketsphinx_batch
