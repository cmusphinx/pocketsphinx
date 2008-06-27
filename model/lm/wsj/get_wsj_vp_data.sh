#!/bin/sh

# Extremely simple script to set up data for language model training.
#
# Edit WSJ0_DIR to point to your WSJ0 corpus.  It should contain the
# 'transcrp' and 'lng_modl' directories.

WSJ0_DIR=${1:-/net/karybdis/usr3/robust/data/wsj/wsj0/}

# Preprocess the vocabulary a bit
perl -ne 'BEGIN { print "<s>\n</s>\n"; } print unless /^#(#|\s+)/' \
    $WSJ0_DIR/lng_modl/vocab/wlist5o.vp > wlist5o.vp
# Process WSJ0 transcripts (used for pruning/validation)
find $WSJ0_DIR/transcrp -name '*.dot' \
    | xargs $WSJ0_DIR/lng_modl/utils/dot2lsn \
    | perl -pe 's,^,<s> ,;s,(\s+\([^\)]+\))?$, </s>,;' > wsj0_trans.lsn
# Process language model training data
zcat $WSJ0_DIR/lng_modl/lm_train/vp_data/*/*.z \
    | perl -ne 'print unless /<.*>/' \
    | $WSJ0_DIR/lng_modl/utils/dot2lsn \
    | sed -e 's,^,<s> ,;s,$, </s>,' > wsj_vp_lm.lsn
