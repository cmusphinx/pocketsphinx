#!/bin/sh

# Script for training language model.  This is mainly for
# informational purposes as it requires you to have a bunch of other
# software installed.
CMUCLMTK_DIR=/net/redwood/usr1/dhuggins/cmuclmtk
SRILM_BINDIR=/net/redwood/usr1/dhuggins/srilm/bin/i686-m64

# Build the baseline model
$CMUCLMTK_DIR/perl/ngram_train --bindir $CMUCLMTK_DIR/bin \
    --cutoffs=0,1 --vocab wlist5o.vp \
    -o - wsj_vp_lm.lsn | gzip -c9 > wlist5o.vp.tg.lm.gz

# Prune it
$SRILM_BINDIR/ngram -prune 3e-7 -ppl wsj0_trans.lsn \
    -lm wlist5o.vp.tg.lm.gz \
    -write-lm - | ../../../../sphinxbase/src/sphinx_lmtools/sphinx_lm_sort \
    | gzip -c9 > wlist5o.3e-7.vp.tg.lm.gz

# Convert it to DMP
../../../../sphinx3/src/programs/sphinx3_lm_convert \
    -i wlist5o.3e-7.vp.tg.lm.gz -o wlist5o.3e-7.vp.tg.lm.DMP
