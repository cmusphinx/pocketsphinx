/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2004 Carnegie Mellon University.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * This work was supported in part by funding from the Defense Advanced 
 * Research Projects Agency and the National Science Foundation of the 
 * United States of America, and the CMU Sphinx Speech Consortium.
 *
 * THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND 
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
 * NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ====================================================================
 *
 */

/*
 * NOTE: this module assumes that the dictionary is organized as follows:
 *     Main, real dictionary words
 *     </s>
 *     <s>... (possibly more than one of these)
 *     <sil>
 *     noise-words...
 * In particular, note that </s> comes before <s> since </s> occurs in the LM, but
 * not <s> (well, there's no transition to <s> in the LM).
 */

/* System includes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

/* SphinxBase includes. */
#include <ckd_alloc.h>
#include <listelem_alloc.h>
#include <err.h>
#include <cmd_ln.h>
#include <ngram_model.h>
#include <prim_type.h>

/* Local includes. */
#include "search_const.h"
#include "dict.h"
#include "phone.h"
#include "kb.h"
#include "log.h"
#include "s2_semi_mgau.h"
#include "ms_mgau.h"
#include "senscr.h"
#include "fbs.h"
#include "search.h"

/* Turn this on to dump channels for debugging */
#define __CHAN_DUMP__		0

#define ISA_FILLER_WORD(x)	((x) >= SilenceWordId)
#define ISA_REAL_WORD(x)	((x) < FinishWordId)

#define SEARCH_PROFILE			1
#define SEARCH_TRACE_CHAN		0
#define SEARCH_TRACE_CHAN_DETAILED	0
#define SEARCH_SELFTEST_DETAILED	0

/* FIXME: For the time being, we keep this global configuration object. */
static cmd_ln_t *config;

/*
 * Search structure of HMM instances (channels; see chan_t and root_chan_t definitions):
 * The word triphone sequences (HMM instances) are transformed into tree structures,
 * one tree per unique left triphone in the entire dictionary (actually diphone, since
 * its left context varies dyamically during the search process).  The entire set of
 * trees of channels is allocated once and for all during initialization (since
 * dynamic management of active CHANs is time consuming), with one exception: the
 * last phones of words, that need multiple right context modelling, are not maintained
 * in this static structure since there are too many of them and few are active at any
 * time.  Instead they are maintained as linked lists of CHANs, one list per word,
 * and each CHAN in this set is allocated only on demand and freed if inactive.
 */
static root_chan_t *root_chan;  /* one per unique root channel */
static int32 n_root_chan_alloc; /* total number of root channels allocated */
static int32 n_root_chan;       /* number of root channels valid for a given utt;
                                   depends on words in the LM for that utt */
static int32 n_nonroot_chan;    /* #non-root channels in search tree */

/* MAX #non-root channels in search tree for allocating active_chan_list[]... */
static int32 max_nonroot_chan = 0;

static int32 n_phone_eval;
static int32 n_root_chan_eval;
static int32 n_nonroot_chan_eval;
static int32 n_last_chan_eval;
static int32 n_word_lastchan_eval;
static int32 n_lastphn_cand_utt;

/* HMM context structure */
static hmm_context_t *hmmctx;

/* Allocator for chan_t */
static listelem_alloc_t *chan_alloc;
/* Allocator for root_chan_t */
static listelem_alloc_t *root_chan_alloc;
/* Allocator for latnode_t (shared with searchlat.c) */
listelem_alloc_t *latnode_alloc;

/*
 * word_chan[w] = separate linked list of channels for each word w, normally used only
 * to model the last phone of w, with multiple channels representing different right
 * context phones.
 */
static chan_t **word_chan;

/* word_active[w] = 1 if word w active in current frame, 0 otherwise */
static uint8 *word_active;

/*
 * Each node in the HMM tree structure may point to a set of words whose last phone
 * would follow that node in the tree structure (but is not included in the tree
 * structure for reasons explained above).  The channel node points to one word in this
 * set of words.  The remaining words are linked through homophone_set[].
 * 
 * Single-phone words are not represented in the HMM tree; they are kept in word_chan.
 *
 * Specifically, homophone_set[w] = wid of next word in the same set as w.
 */
static int32 *homophone_set;

/*
 * In any frame, only some HMM tree nodes are active.  active_chan_list[f mod 2] =
 * list of nonroot channels in the HMM tree active in frame f.
 * Similarly, active_word_list[f mod 2] = list of word ids for which active channels
 * exist in word_chan in frame f.
 */
static chan_t **active_chan_list[2] = { NULL, NULL };
static int32 n_active_chan[2];  /* #entries in active_chan_list */
static int32 *active_word_list[2];
static int32 n_active_word[2];  /* #entries in active_word_list */

static int32 NumWords;          /* Total #words in dictionary */
static int32 NumMainDictWords;  /* #words in main dictionary, excluding fillers
                                   (i.e., <s>, </s>, <sil>, and noise words).
                                   These come first in WordDict. */
static int32 NumCiPhones;

static int32 StartWordId;
static int32 FinishWordId;
static int32 SilenceWordId;
static int32 SilencePhoneId;

static int32 BestScore;         /* Best among all phones */
static int32 LastPhoneBestScore;        /* Best among last phones only */
static int32 LogBeamWidth;
static int32 DynamicLogBeamWidth;       /* Modified by absolute pruning */
static int32 NewPhoneLogBeamWidth;
static int32 NewWordLogBeamWidth;
static int32 LastPhoneAloneLogBeamWidth;
static int32 LastPhoneLogBeamWidth;

static int32 FwdflatLogBeamWidth;
static int32 FwdflatLogWordBeamWidth;

static int32 FillerWordPenalty = 0;
static int32 SilenceWordPenalty = 0;
static int32 LogInsertionPenalty = 0;
static int32 logPhoneInsertionPenalty = 0;
static float32 fwdtree_lw = 6.5;
static float32 fwdflat_lw = 8.5;
static float32 bestpath_lw = 9.5;
static float32 bestpath_fwdtree_lw_ratio = 9.5 / 6.5;
static float32 fwdflat_fwdtree_lw_ratio = 8.5 / 6.5;

static int32 newword_penalty = 0;

static int32 compute_all_senones = TRUE;

static int32 ChannelsPerFrameTarget = 0;        /* #channels to eval / frame */

static int32 CurrentFrame;
static int32 LastFrame;

static int32 n_senone_active_utt;

static BPTBL_T *BPTable;        /* Forward pass lattice */
static int32 BPIdx;             /* First free BPTable entry */
static int32 BPTableSize;
static int32 *BScoreStack;      /* Score stack for all possible right contexts */
static int32 BSSHead;           /* First free BScoreStack entry */
static int32 BScoreStackSize;
static int32 *BPTableIdx;       /* First BPTable entry for each frame */
static int32 *WordLatIdx;       /* BPTable index for any word in current frame;
                                   cleared before each frame */
static int32 BPTblOflMsg;       /* Whether BPtable overflow msg has been printed */

static int32 *lattice_density;  /* #words/frame in lattice */

static int32 *zeroPermTab;

/* Word-id sequence hypothesized by decoder */
static search_hyp_t hyp[HYP_SZ];        /* no <s>, </s>, or filler words */
static char hyp_str[4096];      /* hyp as string of words sep. by blanks */
static int32 hyp_wid[4096];
static int32 n_hyp_wid;

static int32 HypTotalScore;
static int32 TotalLangScore;

static int32 hyp_alternates = FALSE;

static int32 *single_phone_wid;       /* list of single-phone word ids */
static int32 n_1ph_words;       /* #single phone words in dict (total) */
static int32 n_1ph_LMwords;     /* #single phone dict words also in LM;
                                   these come first in single_phone_wid */

static void seg_back_trace(int32, char const *);
static void renormalize_scores(int32);
static void fwdflat_renormalize_scores(int32);
static int32 renormalized;

static int32 skip_alt_frm = 0;

/*
 * Two word context prior to utterance, to be used instead of <s>.  Faked by entering
 * <s> and the context into BPTable before starting search (see search_start_fwd).
 * If no context, both should be -ve; if only one context word, [1] should be -ve.
 */
static int32 context_word[2];

#if 0
static dump_search_tree_root();
static dump_search_tree();
#endif

static int16 *topsen_score;     /* Top senone score in each frame */

int32
context_frames(void)
{
    if (context_word[0] < 0)
        return 0;
    if (context_word[1] < 0)
        return 2;
    return 3;
}

/*
 * Candidates words for entering their last phones.  Cleared and rebuilt in each
 * frame.
 * NOTE: candidates can only be multi-phone, real dictionary words.
 */
typedef struct lastphn_cand_s {
    int32 wid;
    int32 score;
    int32 bp;
    int32 next;                 /* next candidate starting at the same frame */
} lastphn_cand_t;
static lastphn_cand_t *lastphn_cand;
static int32 n_lastphn_cand;

#if __CHAN_DUMP__
#define chan_v_eval(chan) hmm_dump_vit_eval(&(chan)->hmm, stderr)
#else
#define chan_v_eval(chan) hmm_vit_eval(&(chan)->hmm)
#endif

int32
eval_root_chan(void)
{
    root_chan_t *rhmm;
    int32 i, cf, bestscore, k;

    cf = CurrentFrame;
    bestscore = WORST_SCORE;
    k = 0;
    for (i = n_root_chan, rhmm = root_chan; i > 0; --i, rhmm++) {
        if (hmm_frame(&rhmm->hmm) == cf) {
            int32 score;

            score = chan_v_eval(rhmm);
            if (bestscore < score)
                bestscore = score;

#if (SEARCH_PROFILE || SEARCH_TRACE_CHAN)
            k++;
#endif
        }
    }

#if SEARCH_PROFILE
    n_root_chan_eval += k;
#endif

#if SEARCH_TRACE_CHAN
    E_INFO(" %3d #root(%10d)", cf, k, bestscore);
#endif

    return (bestscore);
}

int32
eval_nonroot_chan(void)
{
    chan_t *hmm, **acl;
    int32 i, cf, bestscore, k;

    cf = CurrentFrame;
    i = n_active_chan[cf & 0x1];
    acl = active_chan_list[cf & 0x1];
    bestscore = WORST_SCORE;

    k = i;
    for (hmm = *(acl++); i > 0; --i, hmm = *(acl++)) {
        int32 score;

        assert(hmm_frame(&hmm->hmm) == cf);
        score = chan_v_eval(hmm);
        if (bestscore < score)
            bestscore = score;
    }

#if SEARCH_PROFILE
    n_nonroot_chan_eval += k;
#endif

#if SEARCH_TRACE_CHAN
    E_INFO(" %5d #non-root(%10d)", k, bestscore);
#endif

    return (bestscore);
}

int32
eval_word_chan(void)
{
    root_chan_t *rhmm;
    chan_t *hmm;
    int32 i, w, cf, bestscore, *awl, j, k;

    k = 0;
    cf = CurrentFrame;
    bestscore = WORST_SCORE;
    awl = active_word_list[cf & 0x1];

    i = n_active_word[cf & 0x1];
    for (w = *(awl++); i > 0; --i, w = *(awl++)) {
        assert(word_active[w] != 0);

        word_active[w] = 0;

        assert(word_chan[w] != NULL);

        for (hmm = word_chan[w]; hmm; hmm = hmm->next) {
            int32 score;

            assert(hmm_frame(&hmm->hmm) == cf);
            score = chan_v_eval(hmm);

            if (bestscore < score)
                bestscore = score;

#if (SEARCH_PROFILE || SEARCH_TRACE_CHAN)
            k++;
#endif
        }
    }

    /* Similarly for statically allocated single-phone words */
    j = 0;
    for (i = 0; i < n_1ph_words; i++) {
        int32 score;

        w = single_phone_wid[i];
        rhmm = (root_chan_t *) word_chan[w];
        if (hmm_frame(&rhmm->hmm) < cf)
            continue;

        score = chan_v_eval(rhmm);

        if (bestscore < score && w != FinishWordId)
            bestscore = score;

#if (SEARCH_PROFILE || SEARCH_TRACE_CHAN)
        j++;
#endif
    }

#if (SEARCH_PROFILE || SEARCH_TRACE_CHAN)
    k += j;
#endif

#if SEARCH_PROFILE
    n_last_chan_eval += k;
    n_nonroot_chan_eval += k;
    n_word_lastchan_eval += n_active_word[cf & 0x1] + j;
#endif

#if SEARCH_TRACE_CHAN
    printf(" %5d #leaf(%10d)\n", k, bestscore);
#endif

    return (bestscore);
}

static void
cache_bptable_paths(int32 bp)
{
    int32 w, prev_bp;
    BPTBL_T *bpe;

    bpe = &(BPTable[bp]);
    prev_bp = bp;
    w = bpe->wid;
    while (ISA_FILLER_WORD(w)) {
        prev_bp = BPTable[prev_bp].bp;
        w = BPTable[prev_bp].wid;
    }
    bpe->real_wid = g_word_dict->dict_list[w]->wid;

    if (cmd_ln_boolean_r(config, "-fwd3g")) {
        prev_bp = BPTable[prev_bp].bp;
        bpe->prev_real_wid =
            (prev_bp != NO_BP) ? BPTable[prev_bp].real_wid : -1;
    }
    else
        bpe->prev_real_wid = -1;
}

void
save_bwd_ptr(int32 w, int32 score, int32 path, int32 rc)
{
    int32 _bp_;

    _bp_ = WordLatIdx[w];
    if (_bp_ != NO_BP) {
        if (BPTable[_bp_].score < score) {
            if (BPTable[_bp_].bp != path) {
                BPTable[_bp_].bp = path;
                cache_bptable_paths(_bp_);
            }
            BPTable[_bp_].score = score;
        }
        BScoreStack[BPTable[_bp_].s_idx + rc] = score;
    }
    else {
        int32 i, rcsize, *bss;
        dict_entry_t *de;
        BPTBL_T *bpe;

        if ((BPIdx >= BPTableSize)
            || (BSSHead >= BScoreStackSize - NumCiPhones)) {
            if (!BPTblOflMsg) {
                E_ERROR
                    ("BPTable OVERFLOWED; IGNORING REST OF UTTERANCE!!\n");
                BPTblOflMsg = 1;
            }
            return;
        }

        de = g_word_dict->dict_list[w];
        WordLatIdx[w] = BPIdx;
        bpe = &(BPTable[BPIdx]);
        bpe->wid = w;
        bpe->frame = CurrentFrame;
        bpe->bp = path;
        bpe->score = score;
        bpe->s_idx = BSSHead;
        bpe->valid = 1;

        if ((de->len != 1) && (de->mpx)) {
            bpe->r_diph = de->phone_ids[de->len - 1];
            rcsize = g_word_dict->rcFwdSizeTable[bpe->r_diph];
        }
        else {
            bpe->r_diph = -1;
            rcsize = 1;
        }
        for (i = rcsize, bss = BScoreStack + BSSHead; i > 0; --i, bss++)
            *bss = WORST_SCORE;
        BScoreStack[BSSHead + rc] = score;
        cache_bptable_paths(BPIdx);

        BPIdx++;
        BSSHead += rcsize;
    }
}


/*
 * Limit the number of word exits in each frame to maxwpf.  And also limit the number of filler
 * words to 1.
 */
static void
bptable_maxwpf(int32 maxwpf)
{
    int32 cf, bp, n;
    int32 bestscr, worstscr;
    BPTBL_T *bpe, *bestbpe, *worstbpe;

    cf = CurrentFrame;

    /* Allow only one filler word exit (the best) per frame */
    bestscr = (int32) 0x80000000;
    bestbpe = NULL;
    n = 0;
    for (bp = BPTableIdx[cf]; bp < BPIdx; bp++) {
        bpe = &(BPTable[bp]);
        if (ISA_FILLER_WORD(bpe->wid)) {
            if (bpe->score > bestscr) {
                bestscr = bpe->score;
                bestbpe = bpe;
            }
            bpe->valid = 0;     /* Flag to indicate invalidation */
            n++;                /* No. of filler words */
        }
    }
    /* Restore bestbpe to valid state */
    if (bestbpe != NULL) {
        bestbpe->valid = 1;
        --n;
    }

    /* Allow up to maxwpf best entries to survive; mark the remaining with valid = 0 */
    n = (BPIdx - BPTableIdx[cf]) - n;   /* No. of entries after limiting fillers */
    for (; n > maxwpf; --n) {
        /* Find worst BPTable entry */
        worstscr = (int32) 0x7fffffff;
        worstbpe = NULL;
        for (bp = BPTableIdx[cf]; (bp < BPIdx); bp++) {
            bpe = &(BPTable[bp]);
            if (bpe->valid && (bpe->score < worstscr)) {
                worstscr = bpe->score;
                worstbpe = bpe;
            }
        }
        if (worstbpe == NULL)
            E_FATAL("PANIC: No worst BPtable entry remaining\n");
        worstbpe->valid = 0;
    }
}


/*
 * Prune currently active root channels for next frame.  Also, perform exit
 * transitions out of them and activate successors.
 * score[] of pruned root chan set to WORST_SCORE elsewhere.
 */
void
prune_root_chan(void)
{
    root_chan_t *rhmm;
    chan_t *hmm;
    int32 i, cf, nf, w, pip;
    int32 thresh, newphone_thresh, lastphn_thresh, newphone_score;
    chan_t **nacl;              /* next active list */
    lastphn_cand_t *candp;
    dict_entry_t *de;

    cf = CurrentFrame;
    nf = cf + 1;
    thresh = BestScore + DynamicLogBeamWidth;
    newphone_thresh =
        BestScore + (DynamicLogBeamWidth >
                     NewPhoneLogBeamWidth ? DynamicLogBeamWidth :
                     NewPhoneLogBeamWidth);
    lastphn_thresh =
        BestScore + (DynamicLogBeamWidth >
                     LastPhoneLogBeamWidth ? DynamicLogBeamWidth :
                     LastPhoneLogBeamWidth);

    pip = logPhoneInsertionPenalty;

#if 0
    printf("%10d =Th, %10d =NPTh, %10d =LPTh, %10d =bestscore\n",
           thresh, newphone_thresh, lastphn_thresh, BestScore);
#endif

    nacl = active_chan_list[nf & 0x1];

    for (i = 0, rhmm = root_chan; i < n_root_chan; i++, rhmm++) {
        /* First check if this channel was active in current frame */
        if (hmm_frame(&rhmm->hmm) < cf)
            continue;

        if (hmm_bestscore(&rhmm->hmm) > thresh) {
            hmm_frame(&rhmm->hmm) = nf;  /* rhmm will be active in next frame */

            if (skip_alt_frm && (!(cf % skip_alt_frm)))
                continue;

            /* transitions out of this root channel */
            newphone_score = hmm_out_score(&rhmm->hmm) + pip;
            if (newphone_score > newphone_thresh) {
                /* transition to all next-level channels in the HMM tree */
                for (hmm = rhmm->next; hmm; hmm = hmm->alt) {
                    if ((hmm_frame(&hmm->hmm) < cf)
                        || (hmm_in_score(&hmm->hmm)
                            < newphone_score)) {
                        hmm_enter(&hmm->hmm, newphone_score,
                                  hmm_out_history(&rhmm->hmm), nf);
                        *(nacl++) = hmm;
                    }
                }

                /*
                 * Transition to last phone of all words for which this is the
                 * penultimate phone (the last phones may need multiple right contexts).
                 * Remember to remove the temporary newword_penalty.
                 */
                if (newphone_score > lastphn_thresh) {
                    for (w = rhmm->penult_phn_wid; w >= 0;
                         w = homophone_set[w]) {
                        de = g_word_dict->dict_list[w];
                        candp = lastphn_cand + n_lastphn_cand;
                        n_lastphn_cand++;
                        candp->wid = w;
                        candp->score =
                            newphone_score - newword_penalty;
                        candp->bp = hmm_out_history(&rhmm->hmm);
                    }
                }
            }
        }
    }
    n_active_chan[nf & 0x1] = nacl - active_chan_list[nf & 0x1];
}

/*
 * Prune currently active nonroot channels in HMM tree for next frame.  Also, perform
 * exit transitions out of such channels and activate successors.
 */
void
prune_nonroot_chan(void)
{
    chan_t *hmm, *nexthmm;
    int32 cf, nf, w, i, pip;
    int32 thresh, newphone_thresh, lastphn_thresh, newphone_score;
    chan_t **acl, **nacl;       /* active list, next active list */
    lastphn_cand_t *candp;
    dict_entry_t *de;

    cf = CurrentFrame;
    nf = cf + 1;

    thresh = BestScore + DynamicLogBeamWidth;
    newphone_thresh =
        BestScore + (DynamicLogBeamWidth >
                     NewPhoneLogBeamWidth ? DynamicLogBeamWidth :
                     NewPhoneLogBeamWidth);
    lastphn_thresh =
        BestScore + (DynamicLogBeamWidth >
                     LastPhoneLogBeamWidth ? DynamicLogBeamWidth :
                     LastPhoneLogBeamWidth);

    pip = logPhoneInsertionPenalty;

    acl = active_chan_list[cf & 0x1];   /* currently active HMMs in tree */
    nacl = active_chan_list[nf & 0x1] + n_active_chan[nf & 0x1];

    for (i = n_active_chan[cf & 0x1], hmm = *(acl++); i > 0;
         --i, hmm = *(acl++)) {
        assert(hmm_frame(&hmm->hmm) >= cf);

        if (hmm_bestscore(&hmm->hmm) > thresh) {
            /* retain this channel in next frame */
            if (hmm_frame(&hmm->hmm) != nf) {
                hmm_frame(&hmm->hmm) = nf;
                *(nacl++) = hmm;
            }

            if (skip_alt_frm && (!(cf % skip_alt_frm)))
                continue;

            /* transitions out of this channel */
            newphone_score = hmm_out_score(&hmm->hmm) + pip;
            if (newphone_score > newphone_thresh) {
                /* transition to all next-level channel in the HMM tree */
                for (nexthmm = hmm->next; nexthmm; nexthmm = nexthmm->alt) {
                    if ((hmm_frame(&nexthmm->hmm) < cf)
                        || (hmm_in_score(&nexthmm->hmm) < newphone_score)) {
                        if (hmm_frame(&nexthmm->hmm) != nf) {
                            /* Keep this HMM on the active list */
                            *(nacl++) = nexthmm;
                        }
                        hmm_enter(&nexthmm->hmm, newphone_score,
                                  hmm_out_history(&hmm->hmm), nf);
                    }
                }

                /*
                 * Transition to last phone of all words for which this is the
                 * penultimate phone (the last phones may need multiple right contexts).
                 * Remember to remove the temporary newword_penalty.
                 */
                if (newphone_score > lastphn_thresh) {
                    for (w = hmm->info.penult_phn_wid; w >= 0;
                         w = homophone_set[w]) {
                        de = g_word_dict->dict_list[w];
                        candp = lastphn_cand + n_lastphn_cand;
                        n_lastphn_cand++;
                        candp->wid = w;
                        candp->score =
                            newphone_score - newword_penalty;
                        candp->bp = hmm_out_history(&hmm->hmm);
                    }
                }
            }
        }
        else if (hmm_frame(&hmm->hmm) != nf) {
            hmm_clear_scores(&hmm->hmm);
        }
    }
    n_active_chan[nf & 0x1] = nacl - active_chan_list[nf & 0x1];
}

/*
 * Since the same instance of a word (i.e., <word,start-frame>) reaches its last
 * phone several times, we can compute its best BP and LM transition score info
 * just the first time and cache it for future occurrences.  Structure for such
 * a cache.
 */
typedef struct {
    int32 sf;                   /* Start frame */
    int32 dscr;                 /* Delta-score upon entering last phone */
    int32 bp;                   /* Best BP */
} last_ltrans_t;
static last_ltrans_t *last_ltrans;      /* one per word */

#define CAND_SF_ALLOCSIZE	32
typedef struct {
    int32 bp_ef;
    int32 cand;
} cand_sf_t;
static int32 cand_sf_alloc = 0;
static cand_sf_t *cand_sf;

/*
 * Execute the transition into the last phone for all candidates words emerging from
 * the HMM tree.  Attach LM scores to such transitions.
 * (Executed after pruning root and non-root, but before pruning word-chan.)
 */
void
last_phone_transition(void)
{
    int32 i, j, k, cf, nf, bp, bplast, w;
    lastphn_cand_t *candp;
    int32 *nawl;
    int32 thresh;
    int32 *rcpermtab, ciph0;
    int32 bestscore, dscr;
    dict_entry_t *de;
    chan_t *hmm;
    BPTBL_T *bpe;
    int32 n_cand_sf = 0;

    cf = CurrentFrame;
    nf = cf + 1;
    nawl = active_word_list[nf & 0x1];

#if SEARCH_PROFILE
    n_lastphn_cand_utt += n_lastphn_cand;
#endif

    /* For each candidate word (entering its last phone) */
    /* If best LM score and bp for candidate known use it, else sort cands by startfrm */
    for (i = 0, candp = lastphn_cand; i < n_lastphn_cand; i++, candp++) {
        /* Backpointer entry for it. */
        bpe = &(BPTable[candp->bp]);
        /* Right context phone table. */
        rcpermtab =
            (bpe->r_diph >=
             0) ? g_word_dict->rcFwdPermTable[bpe->r_diph] : zeroPermTab;

        /* Subtract starting score for candidate, leave it with only word score */
        de = g_word_dict->dict_list[candp->wid];
        ciph0 = de->ci_phone_ids[0];
        candp->score -= BScoreStack[bpe->s_idx + rcpermtab[ciph0]];

        /*
         * If this candidate not occurred in an earlier frame, prepare for finding
         * best transition score into last phone; sort by start frame.
         */
        /* i.e. if we don't have an entry in last_ltrans for this
         * <word,sf>, then create one */
        if (last_ltrans[candp->wid].sf != bpe->frame + 1) {
            /* Look for an entry in cand_sf matching the backpointer
             * for this candidate. */
            for (j = 0; j < n_cand_sf; j++) {
                if (cand_sf[j].bp_ef == bpe->frame)
                    break;
            }
            /* Oh, we found one, so chain onto it. */
            if (j < n_cand_sf)
                candp->next = cand_sf[j].cand;
            else {
                /* Nope, let's make a new one, allocating cand_sf if necessary. */
                if (n_cand_sf >= cand_sf_alloc) {
                    if (cand_sf_alloc == 0) {
                        cand_sf =
                            ckd_calloc(CAND_SF_ALLOCSIZE,
                                                    sizeof(cand_sf_t));
                        cand_sf_alloc = CAND_SF_ALLOCSIZE;
                    }
                    else {
                        cand_sf_alloc += CAND_SF_ALLOCSIZE;
                        cand_sf = ckd_realloc(cand_sf,
                                              cand_sf_alloc * sizeof(cand_sf_t));
                        E_INFO("cand_sf[] increased to %d entries\n",
                               cand_sf_alloc);
                    }
                }

                /* Use the newly created cand_sf. */
                j = n_cand_sf++;
                candp->next = -1; /* End of the chain. */
                cand_sf[j].bp_ef = bpe->frame;
            }
            /* Update it to point to this candidate. */
            cand_sf[j].cand = i;

            last_ltrans[candp->wid].dscr = WORST_SCORE;
            last_ltrans[candp->wid].sf = bpe->frame + 1;
        }
    }

    /* Compute best LM score and bp for new cands entered in the sorted lists above */
    for (i = 0; i < n_cand_sf; i++) {
        /* For the i-th unique end frame... */
        bp = BPTableIdx[cand_sf[i].bp_ef];
        bplast = BPTableIdx[cand_sf[i].bp_ef + 1] - 1;

        for (bpe = &(BPTable[bp]); bp <= bplast; bp++, bpe++) {
            if (!bpe->valid)
                continue;

            /* For each bp entry in the i-th end frame... */
            rcpermtab = (bpe->r_diph >= 0) ?
                g_word_dict->rcFwdPermTable[bpe->r_diph] : zeroPermTab;

            /* For each candidate at the start frame find bp->cand transition-score */
            for (j = cand_sf[i].cand; j >= 0; j = candp->next) {
                int32 n_used;
                candp = &(lastphn_cand[j]);
                de = g_word_dict->dict_list[candp->wid];
                ciph0 = de->ci_phone_ids[0];

                dscr = BScoreStack[bpe->s_idx + rcpermtab[ciph0]];
                dscr += ngram_tg_score(g_lmset, de->wid, bpe->real_wid,
                                       bpe->prev_real_wid, &n_used);

                if (last_ltrans[candp->wid].dscr < dscr) {
                    last_ltrans[candp->wid].dscr = dscr;
                    last_ltrans[candp->wid].bp = bp;
                }
            }
        }
    }

    /* Update best transitions for all candidates; also update best lastphone score */
    bestscore = LastPhoneBestScore;
    for (i = 0, candp = lastphn_cand; i < n_lastphn_cand; i++, candp++) {
        candp->score += last_ltrans[candp->wid].dscr;
        candp->bp = last_ltrans[candp->wid].bp;

        if (bestscore < candp->score)
            bestscore = candp->score;
    }
    LastPhoneBestScore = bestscore;

    /* At this pt, we know the best entry score (with LM component) for all candidates */
    thresh = bestscore + LastPhoneAloneLogBeamWidth;
    for (i = n_lastphn_cand, candp = lastphn_cand; i > 0; --i, candp++) {
        if (candp->score > thresh) {
            w = candp->wid;

            alloc_all_rc(w);

            k = 0;
            for (hmm = word_chan[w]; hmm; hmm = hmm->next) {
                if ((hmm_frame(&hmm->hmm) < cf)
                    || (hmm_in_score(&hmm->hmm) < candp->score)) {
                    assert(hmm_frame(&hmm->hmm) != nf);
                    hmm_enter(&hmm->hmm,
                              candp->score, candp->bp, nf);
                    k++;
                }
            }
            if (k > 0) {
                assert(!word_active[w]);
                assert(g_word_dict->dict_list[w]->len > 1);
                *(nawl++) = w;
                word_active[w] = 1;
            }
        }
    }
    n_active_word[nf & 0x1] = nawl - active_word_list[nf & 0x1];
}

/*
 * Prune currently active word channels for next frame.  Also, perform exit
 * transitions out of such channels and active successors.
 */
void
prune_word_chan(void)
{
    root_chan_t *rhmm;
    chan_t *hmm, *thmm;
    chan_t **phmmp;             /* previous HMM-pointer */
    int32 cf, nf, w, i, k;
    int32 newword_thresh, lastphn_thresh;
    int32 *awl, *nawl;

    cf = CurrentFrame;
    nf = cf + 1;
    newword_thresh =
        LastPhoneBestScore + (DynamicLogBeamWidth >
                              NewWordLogBeamWidth ? DynamicLogBeamWidth :
                              NewWordLogBeamWidth);
    lastphn_thresh =
        LastPhoneBestScore + (DynamicLogBeamWidth >
                              LastPhoneAloneLogBeamWidth ?
                              DynamicLogBeamWidth :
                              LastPhoneAloneLogBeamWidth);

    awl = active_word_list[cf & 0x1];
    nawl = active_word_list[nf & 0x1] + n_active_word[nf & 0x1];

    /* Dynamically allocated last channels of multi-phone words */
    for (i = n_active_word[cf & 0x1], w = *(awl++); i > 0;
         --i, w = *(awl++)) {
        k = 0;
        phmmp = &(word_chan[w]);
        for (hmm = word_chan[w]; hmm; hmm = thmm) {
            assert(hmm_frame(&hmm->hmm) >= cf);

            thmm = hmm->next;
            if (hmm_bestscore(&hmm->hmm) > lastphn_thresh) {
                /* retain this channel in next frame */
                hmm_frame(&hmm->hmm) = nf;
                k++;
                phmmp = &(hmm->next);

                /* Could if ((! skip_alt_frm) || (cf & 0x1)) the following */
                if (hmm_out_score(&hmm->hmm) > newword_thresh) {
                    /* can exit channel and recognize word */
                    save_bwd_ptr(w, hmm_out_score(&hmm->hmm),
                                 hmm_out_history(&hmm->hmm),
                                 hmm->info.rc_id);
                }
            }
            else if (hmm_frame(&hmm->hmm) == nf) {
                phmmp = &(hmm->next);
            }
            else {
                hmm_deinit(&hmm->hmm);
                listelem_free(chan_alloc, hmm);
                *phmmp = thmm;
            }
        }
        if ((k > 0) && (!word_active[w])) {
            assert(g_word_dict->dict_list[w]->len > 1);
            *(nawl++) = w;
            word_active[w] = 1;
        }
    }
    n_active_word[nf & 0x1] = nawl - active_word_list[nf & 0x1];

    /*
     * Prune permanently allocated single-phone channels.
     * NOTES: score[] of pruned channels set to WORST_SCORE elsewhere.
     */
    for (i = 0; i < n_1ph_words; i++) {
        w = single_phone_wid[i];
        rhmm = (root_chan_t *) word_chan[w];
        if (hmm_frame(&rhmm->hmm) < cf)
            continue;
        if (hmm_bestscore(&rhmm->hmm) > lastphn_thresh) {
            hmm_frame(&rhmm->hmm) = nf;

            /* Could if ((! skip_alt_frm) || (cf & 0x1)) the following */
            if (hmm_out_score(&rhmm->hmm) > newword_thresh) {
                save_bwd_ptr(w, hmm_out_score(&rhmm->hmm),
                             hmm_out_history(&rhmm->hmm), 0);
            }
        }
    }
}

/*
 * Allocate last phone channels for all possible right contexts for word w.  (Some
 * may already exist.)
 * (NOTE: Assume that w uses context!!)
 */
void
alloc_all_rc(int32 w)
{
    dict_entry_t *de;
    chan_t *hmm, *thmm;
    int32 *sseq_rc;             /* list of sseqid for all possible right context for w */
    int32 i;

    de = g_word_dict->dict_list[w];

    assert(de->mpx);

    sseq_rc = g_word_dict->rcFwdTable[de->phone_ids[de->len - 1]];

    hmm = word_chan[w];
    if ((hmm == NULL) || (hmm->hmm.s.ssid != *sseq_rc)) {
        hmm = listelem_malloc(chan_alloc);
        hmm->next = word_chan[w];
        word_chan[w] = hmm;

        hmm->info.rc_id = 0;
        hmm->ciphone = de->ci_phone_ids[de->len - 1];
        hmm_init(hmmctx, &hmm->hmm, FALSE, *sseq_rc, hmm->ciphone);
    }
    for (i = 1, sseq_rc++; *sseq_rc >= 0; sseq_rc++, i++) {
        if ((hmm->next == NULL) || (hmm->next->hmm.s.ssid != *sseq_rc)) {
            thmm = listelem_malloc(chan_alloc);
            thmm->next = hmm->next;
            hmm->next = thmm;
            hmm = thmm;

            hmm->info.rc_id = i;
            hmm->ciphone = de->ci_phone_ids[de->len - 1];
            hmm_init(hmmctx, &hmm->hmm, FALSE, *sseq_rc, hmm->ciphone);
        }
        else
            hmm = hmm->next;
    }
}

void
free_all_rc(int32 w)
{
    chan_t *hmm, *thmm;

    for (hmm = word_chan[w]; hmm; hmm = thmm) {
        thmm = hmm->next;
        hmm_deinit(&hmm->hmm);
        listelem_free(chan_alloc, hmm);
    }
    word_chan[w] = NULL;
}

/*
 * Structure for reorganizing the BP table entries in the current frame according
 * to distinct right context ci-phones.  Each entry contains the best BP entry for
 * a given right context.  Each successor word will pick up the correct entry based
 * on its first ci-phone.
 */
struct bestbp_rc_s {
    int32 score;
    int32 path;                 /* BP table index corresponding to this entry */
    int32 lc;                   /* right most ci-phone of above BP entry word */
} *bestbp_rc;

void
word_transition(void)
{
    int32 i, k, bp, w, cf, nf;
    /* int32 prev_bp, prev_wid, prev_endframe, prev2_bp, prev2_wid; */
    int32 /* rcsize, */ rc;
    int32 *rcss;                /* right context score stack */
    int32 *rcpermtab;
    int32 thresh, /* newword_thresh, */ newscore;
    BPTBL_T *bpe;
    dict_entry_t *pde, *de;     /* previous dict entry, dict entry */
    root_chan_t *rhmm;
    struct bestbp_rc_s *bestbp_rc_ptr;
    int32 last_ciph;
    int32 pip;
    int32 ssid;

    cf = CurrentFrame;

    /*
     * Transition to start of new word instances (HMM tree roots); but only if words
     * other than </s> finished here.
     * But, first, find the best starting score for each possible right context phone.
     */
    for (i = NumCiPhones - 1; i >= 0; --i)
        bestbp_rc[i].score = WORST_SCORE;
    k = 0;
    for (bp = BPTableIdx[cf]; bp < BPIdx; bp++) {
        bpe = &(BPTable[bp]);
        WordLatIdx[bpe->wid] = NO_BP;

        if (bpe->wid == FinishWordId)
            continue;
        k++;

        de = g_word_dict->dict_list[bpe->wid];
        rcpermtab =
            (bpe->r_diph >=
             0) ? g_word_dict->rcFwdPermTable[bpe->r_diph] : zeroPermTab;
        last_ciph = de->ci_phone_ids[de->len - 1];

        rcss = &(BScoreStack[bpe->s_idx]);
        for (rc = NumCiPhones - 1; rc >= 0; --rc) {
            if (rcss[rcpermtab[rc]] > bestbp_rc[rc].score) {
                bestbp_rc[rc].score = rcss[rcpermtab[rc]];
                bestbp_rc[rc].path = bp;
                bestbp_rc[rc].lc = last_ciph;
            }
        }
    }
    if (k == 0)
        return;

    nf = cf + 1;
    thresh = BestScore + DynamicLogBeamWidth;
    pip = logPhoneInsertionPenalty;
    /*
     * Hypothesize successors to words finished in this frame.
     * Main dictionary, multi-phone words transition to HMM-trees roots.
     */
    for (i = n_root_chan, rhmm = root_chan; i > 0; --i, rhmm++) {
        bestbp_rc_ptr = &(bestbp_rc[rhmm->ciphone]);

        newscore = bestbp_rc_ptr->score + newword_penalty + pip;
        if (newscore > thresh) {
            if ((hmm_frame(&rhmm->hmm) < cf)
                || (hmm_in_score(&rhmm->hmm) < newscore)) {
                ssid =
                    g_word_dict->lcFwdTable[rhmm->diphone][bestbp_rc_ptr->lc];
                hmm_enter(&rhmm->hmm, newscore,
                          bestbp_rc_ptr->path, nf);
                if (hmm_is_mpx(&rhmm->hmm)) {
                    rhmm->hmm.s.mpx_ssid[0] = ssid;
                }
            }
        }
    }

    /*
     * Single phone words; no right context for these.  Cannot use bestbp_rc as
     * LM scores have to be included.  First find best transition to these words.
     */
    for (i = 0; i < n_1ph_LMwords; i++) {
        w = single_phone_wid[i];
        last_ltrans[w].dscr = (int32) 0x80000000;
    }
    for (bp = BPTableIdx[cf]; bp < BPIdx; bp++) {
        bpe = &(BPTable[bp]);
        if (!bpe->valid)
            continue;

        rcpermtab =
            (bpe->r_diph >=
             0) ? g_word_dict->rcFwdPermTable[bpe->r_diph] : zeroPermTab;
        rcss = BScoreStack + bpe->s_idx;

        for (i = 0; i < n_1ph_LMwords; i++) {
            int32 n_used;
            w = single_phone_wid[i];
            de = g_word_dict->dict_list[w];

            newscore = rcss[rcpermtab[de->ci_phone_ids[0]]];
            newscore += ngram_tg_score(g_lmset, de->wid, bpe->real_wid,
                                       bpe->prev_real_wid, &n_used);

            if (last_ltrans[w].dscr < newscore) {
                last_ltrans[w].dscr = newscore;
                last_ltrans[w].bp = bp;
            }
        }
    }

    /* Now transition to in-LM single phone words */
    for (i = 0; i < n_1ph_LMwords; i++) {
        w = single_phone_wid[i];
        rhmm = (root_chan_t *) word_chan[w];
        if ((newscore = last_ltrans[w].dscr + pip) > thresh) {
            bpe = BPTable + last_ltrans[w].bp;
            pde = g_word_dict->dict_list[bpe->wid];

            if ((hmm_frame(&rhmm->hmm) < cf)
                || (hmm_in_score(&rhmm->hmm) < newscore)) {
                hmm_enter(&rhmm->hmm,
                          newscore, last_ltrans[w].bp, nf);
                if (hmm_is_mpx(&rhmm->hmm)) {
                    rhmm->hmm.s.mpx_ssid[0] =
                        g_word_dict->lcFwdTable[rhmm->diphone][pde->ci_phone_ids[pde->len - 1]];
                }
            }
        }
    }

    /* Remaining words: <sil>, noise words.  No mpx for these! */
    bestbp_rc_ptr = &(bestbp_rc[SilencePhoneId]);
    newscore = bestbp_rc_ptr->score + SilenceWordPenalty + pip;
    if (newscore > thresh) {
        w = SilenceWordId;
        rhmm = (root_chan_t *) word_chan[w];
        if ((hmm_frame(&rhmm->hmm) < cf)
            || (hmm_in_score(&rhmm->hmm) < newscore)) {
            hmm_enter(&rhmm->hmm,
                      newscore, bestbp_rc_ptr->path, nf);
        }
    }
    newscore = bestbp_rc_ptr->score + FillerWordPenalty + pip;
    if (newscore > thresh) {
        for (w = SilenceWordId + 1; w < NumWords; w++) {
            rhmm = (root_chan_t *) word_chan[w];
            if ((hmm_frame(&rhmm->hmm) < cf)
                || (hmm_in_score(&rhmm->hmm) < newscore)) {
                hmm_enter(&rhmm->hmm,
                          newscore, bestbp_rc_ptr->path, nf);
            }
        }
    }
}

void
search_initialize(cmd_ln_t *cmdln)
{
    int32 bptable_size;
    
    config = cmdln;
    bptable_size = cmd_ln_int32_r(config, "-latsize");

    chan_alloc = listelem_alloc_init(sizeof(chan_t));
    root_chan_alloc = listelem_alloc_init(sizeof(root_chan_t));
    latnode_alloc = listelem_alloc_init(sizeof(latnode_t));

    NumWords = g_word_dict->dict_entry_count;
    StartWordId = kb_get_word_id("<s>");
    FinishWordId = kb_get_word_id("</s>");
    SilenceWordId = kb_get_word_id("<sil>");
    SilencePhoneId = bin_mdef_ciphone_id(g_mdef, "SIL");
    NumCiPhones = bin_mdef_n_ciphone(g_mdef);
    NumMainDictWords = dict_get_num_main_words(g_word_dict);

    hmmctx = hmm_context_init(bin_mdef_n_emit_state(g_mdef),
                              g_tmat->tp, NULL,
                              g_mdef->sseq);

    word_chan = ckd_calloc(NumWords, sizeof(chan_t *));
    WordLatIdx = ckd_calloc(NumWords, sizeof(int32));
    zeroPermTab = ckd_calloc(bin_mdef_n_ciphone(g_mdef), sizeof(int32));
    word_active = ckd_calloc(NumWords, sizeof(int32));

    if (NumWords / 1000 < 25)
        BPTableSize = 25 * MAX_FRAMES;
    else
        BPTableSize = NumWords / 1000 * MAX_FRAMES;

    BScoreStackSize = BPTableSize * 20;
    if ((bptable_size > 0) && (bptable_size < 0x7fffffff)) {
        BPTableSize = bptable_size;
        BScoreStackSize = BPTableSize * 20;     /* 20 = ave. rc fanout */
    }
    BPTable = ckd_calloc(BPTableSize, sizeof(BPTBL_T));
    BScoreStack = ckd_calloc(BScoreStackSize, sizeof(int32));
    BPTableIdx = ckd_calloc(MAX_FRAMES + 2, sizeof(int32));
    BPTableIdx++;               /* Make BPTableIdx[-1] valid */

    lattice_density = ckd_calloc(MAX_FRAMES, sizeof(int32));

    active_word_list[0] =
        ckd_calloc(2 * (NumWords + 1), sizeof(int32));
    active_word_list[1] = active_word_list[0] + NumWords + 1;

    bestbp_rc = ckd_calloc(NumCiPhones,
                           sizeof(struct
                                  bestbp_rc_s));
#if SEARCH_TRACE_CHAN_DETAILED
    load_trace_wordlist("_TRACEWORDS_");
#endif
    lastphn_cand =
        ckd_calloc(NumWords, sizeof(lastphn_cand_t));

    senone_active = ckd_calloc(bin_mdef_n_sen(g_mdef), sizeof(int32));
    senone_active_vec = bitvec_alloc(bin_mdef_n_sen(g_mdef));

    last_ltrans =
        ckd_calloc(NumWords, sizeof(last_ltrans_t));

    init_search_tree(g_word_dict);
    search_fwdflat_init();
    searchlat_init();

    context_word[0] = -1;
    context_word[1] = -1;

    senone_scores = ckd_calloc(bin_mdef_n_sen(g_mdef), sizeof(*senone_scores));
    topsen_score = ckd_calloc(MAX_FRAMES, sizeof(*topsen_score));

    search_set_beam_width(cmd_ln_float64_r(config, "-beam"));
    search_set_new_word_beam_width(cmd_ln_float64_r(config, "-wbeam"));
    search_set_new_phone_beam_width(cmd_ln_float64_r(config, "-pbeam"));
    search_set_last_phone_beam_width(cmd_ln_float64_r(config, "-lpbeam"));
    search_set_lastphone_alone_beam_width(cmd_ln_float64_r(config, "-lponlybeam"));
    search_set_silence_word_penalty(cmd_ln_float32_r(config, "-silpen"),
                                    cmd_ln_float32_r(config, "-pip"));
    search_set_filler_word_penalty(cmd_ln_float32_r(config, "-fillpen"),
                                    cmd_ln_float32_r(config, "-pip"));
    search_set_newword_penalty(cmd_ln_float32_r(config, "-nwpen"));
    search_set_lw(cmd_ln_float32_r(config, "-lw"),
                  cmd_ln_float32_r(config, "-fwdflatlw"),
                  cmd_ln_float32_r(config, "-bestpathlw"));
    search_set_ip(cmd_ln_float32_r(config, "-wip"));
    search_set_skip_alt_frm(cmd_ln_boolean_r(config, "-skipalt"));
    search_set_fwdflat_bw(cmd_ln_float64_r(config, "-fwdflatbeam"),
                          cmd_ln_float64_r(config, "-fwdflatwbeam"));
}

void
search_free(void)
{
    delete_search_tree();
    ckd_free(word_chan);
    ckd_free(WordLatIdx);
    ckd_free(zeroPermTab);
    ckd_free(word_active);
    ckd_free(BPTable);
    ckd_free(BScoreStack);
    ckd_free(BPTableIdx - 1);
    ckd_free(lattice_density);
    ckd_free(bestbp_rc);
    ckd_free(lastphn_cand);
    n_lastphn_cand = 0;
    ckd_free(senone_active);
    ckd_free(senone_active_vec);
    ckd_free(last_ltrans);
    ckd_free(cand_sf);
    cand_sf_alloc = 0;
    ckd_free(senone_scores);
    ckd_free(topsen_score);
    free_search_tree();
    hmm_context_free(hmmctx);
    ckd_free(active_word_list[0]);
    active_word_list[0] = active_word_list[1] = NULL;
    ckd_free(active_chan_list[0]);
    active_chan_list[0] = active_chan_list[1] = NULL;
    max_nonroot_chan = 0; /* Signal that active_chan_list is empty */
    search_fwdflat_free();
    searchlat_free();
    listelem_alloc_free(chan_alloc);
    listelem_alloc_free(root_chan_alloc);
    listelem_alloc_free(latnode_alloc);
}

/*
 * Set previous two LM context words for search; ie, the first word decoded by
 * search will use w1 and w2 as history for trigram scoring.  If w2 < 0, only
 * one-word history available.  If both < 0, no history available.
 */
void
search_set_context(int32 w1, int32 w2)
{
    context_word[0] = w1;
    context_word[1] = w2;
}

/*
 * Hyp produced by search includes w1 and w2 from search_set_context() as the first
 * two words.  Remove them, if present, from hyp.
 */
void
search_remove_context(search_hyp_t * hyp)
{
    int32 i, j;

    j = 0;
    if (context_word[0] >= 0)
        j++;
    if (context_word[1] >= 0)
        j++;
    if (j > 0) {
        for (i = j; hyp[i].wid >= 0; i++)
            hyp[i - j] = hyp[i];
        hyp[i - j].wid = -1;

        /* hyp_wid has EVERYTHING, including the initial <s>; remove context after it */
        for (i = j + 1; i < n_hyp_wid; i++)
            hyp[i - j] = hyp[i];
        n_hyp_wid -= j;
    }
}

/*
 * Mark the active senones for all senones belonging to channels that are active in the
 * current frame.
 */
void
compute_sen_active(void)
{
    root_chan_t *rhmm;
    chan_t *hmm, **acl;
    int32 i, cf, w, *awl;

    cf = CurrentFrame;

    sen_active_clear();

    /* Flag active senones for root channels */
    for (i = n_root_chan, rhmm = root_chan; i > 0; --i, rhmm++) {
        if (hmm_frame(&rhmm->hmm) == cf)
            hmm_sen_active(&rhmm->hmm);
    }

    /* Flag active senones for nonroot channels in HMM tree */
    i = n_active_chan[cf & 0x1];
    acl = active_chan_list[cf & 0x1];
    for (hmm = *(acl++); i > 0; --i, hmm = *(acl++)) {
        hmm_sen_active(&hmm->hmm);
    }

    /* Flag active senones for individual word channels */
    i = n_active_word[cf & 0x1];
    awl = active_word_list[cf & 0x1];
    for (w = *(awl++); i > 0; --i, w = *(awl++)) {
        for (hmm = word_chan[w]; hmm; hmm = hmm->next) {
            hmm_sen_active(&hmm->hmm);
        }
    }
    for (i = 0; i < n_1ph_words; i++) {
        w = single_phone_wid[i];
        rhmm = (root_chan_t *) word_chan[w];

        if (hmm_frame(&rhmm->hmm) == cf)
            hmm_sen_active(&rhmm->hmm);
    }

    sen_active_flags2list();
}

/*
 * Tree-Search one frame forward.
 */
void
search_fwd(mfcc_t **feat)
{
    if (!compute_all_senones) {
        compute_sen_active();
        topsen_score[CurrentFrame] = senscr_active(feat, CurrentFrame);
    }
    else {
        topsen_score[CurrentFrame] = senscr_all(feat, CurrentFrame);
    }
    n_senone_active_utt += n_senone_active;

    search_one_ply_fwd();
}

void
search_start_fwd(void)
{
    int32 i, rcsize, lscr, w;
    root_chan_t *rhmm;
    dict_entry_t *de;

    n_phone_eval = 0;
    n_root_chan_eval = 0;
    n_nonroot_chan_eval = 0;
    n_last_chan_eval = 0;
    n_word_lastchan_eval = 0;
    n_lastphn_cand_utt = 0;
    n_senone_active_utt = 0;

    BPIdx = 0;
    BSSHead = 0;
    BPTblOflMsg = 0;
    CurrentFrame = 0;

    for (i = 0; i < NumWords; i++)
        WordLatIdx[i] = NO_BP;

    n_active_chan[0] = n_active_chan[1] = 0;
    n_active_word[0] = n_active_word[1] = 0;

    BestScore = 0;
    renormalized = 0;

    for (i = 0; i < NumWords; i++)
        last_ltrans[i].sf = -1;

    hyp_str[0] = '\0';
    hyp[0].wid = -1;

    /* Reset the permanently allocated single-phone words, since they
     * may have junk left over in them from FWDFLAT. */
    for (i = 0; i < n_1ph_words; i++) {
        w = single_phone_wid[i];
        rhmm = (root_chan_t *) word_chan[w];
        hmm_clear(&rhmm->hmm);
    }

    if (context_word[0] < 0) {
        /* Start search with <s>; word_chan[<s>] is permanently allocated */
        rhmm = (root_chan_t *) word_chan[StartWordId];
        hmm_clear(&rhmm->hmm);
        hmm_enter(&rhmm->hmm, 0, NO_BP, 0);
    }
    else {
        int32 n_used;
        /* Simulate insertion of context words into BPTable; first <s> */
        BPTableIdx[0] = 0;
        save_bwd_ptr(StartWordId, 0, NO_BP, 0);
        WordLatIdx[StartWordId] = NO_BP;
        CurrentFrame++;

        /* Insert first context word */
        BPTableIdx[1] = 1;
        de = g_word_dict->dict_list[context_word[0]];
        rcsize = (de->mpx && (de->len > 1)) ?
            g_word_dict->rcFwdSizeTable[de->phone_ids[de->len - 1]] : 1;
        lscr = ngram_bg_score(g_lmset, context_word[0], StartWordId, &n_used);
        for (i = 0; i < rcsize; i++)
            save_bwd_ptr(context_word[0], lscr, 0, i);
        WordLatIdx[context_word[0]] = NO_BP;
        CurrentFrame++;

        /* Insert 2nd context word, if any */
        if (context_word[1] >= 0) {
            int32 n_used;
            BPTableIdx[2] = 2;
            de = g_word_dict->dict_list[context_word[1]];
            rcsize = (de->mpx && (de->len > 1)) ?
                g_word_dict->rcFwdSizeTable[de->phone_ids[de->len - 1]] : 1;
            lscr +=
                ngram_tg_score(g_lmset, context_word[1], context_word[0],
                               StartWordId, &n_used);
            for (i = 0; i < rcsize; i++)
                save_bwd_ptr(context_word[1], lscr, 1, i);
            WordLatIdx[context_word[0]] = NO_BP;
            CurrentFrame++;
        }

        /* Search from silence */
        rhmm = (root_chan_t *) word_chan[SilenceWordId];
        hmm_enter(&rhmm->hmm,
                  BPTable[BPIdx - 1].score, BPIdx - 1, CurrentFrame);
    }

    compute_all_senones = cmd_ln_boolean_r(config, "-compallsen");
}

void
evaluateChannels(void)
{
    int32 bs;

    hmm_context_set_senscore(hmmctx, senone_scores);

    BestScore = eval_root_chan();
    if ((bs = eval_nonroot_chan()) > BestScore)
        BestScore = bs;
    if ((bs = eval_word_chan()) > BestScore)
        BestScore = bs;
    LastPhoneBestScore = bs;
}

void
pruneChannels(void)
{
    int32 maxhmmpf, cf;
    n_lastphn_cand = 0;

    /* Set the dynamic beam based on maxhmmpf here. */
    DynamicLogBeamWidth = LogBeamWidth;
    maxhmmpf = cmd_ln_int32_r(config, "-maxhmmpf");
    cf = CurrentFrame;
    if (maxhmmpf != -1 && n_root_chan_eval + n_nonroot_chan_eval > maxhmmpf) {
        /* Build a histogram to approximately prune them. */
        int32 bins[256], bw, nhmms, i;
        root_chan_t *rhmm;
        chan_t **acl, *hmm;

        /* Bins go from zero (best score) to edge of beam. */
        bw = -LogBeamWidth / 256;
        memset(bins, 0, sizeof(bins));
        /* For each active root channel. */
        for (i = 0, rhmm = root_chan; i < n_root_chan; i++, rhmm++) {
            int32 b;

            /* Put it in a bin according to its bestscore. */
            b = (BestScore - hmm_bestscore(&rhmm->hmm)) / bw;
            if (b >= 256)
                b = 255;
            ++bins[b];
        }
        /* For each active non-root channel. */
        acl = active_chan_list[cf & 0x1];       /* currently active HMMs in tree */
        for (i = n_active_chan[cf & 0x1], hmm = *(acl++);
             i > 0; --i, hmm = *(acl++)) {
            int32 b;

            /* Put it in a bin according to its bestscore. */
            b = (BestScore - hmm_bestscore(&hmm->hmm)) / bw;
            if (b >= 256)
                b = 255;
            ++bins[b];
        }
        /* Walk down the bins to find the new beam. */
        for (i = nhmms = 0; i < 256; ++i) {
            nhmms += bins[i];
            if (nhmms > maxhmmpf)
                break;
        }
        DynamicLogBeamWidth = -(i * bw);
#if 0
        E_INFO("LogBeamWidth %d DynamicLogBeamWidth %d\n",
               LogBeamWidth, DynamicLogBeamWidth);
#endif
    }

    prune_root_chan();
    prune_nonroot_chan();
    last_phone_transition();
    prune_word_chan();
}

void
search_one_ply_fwd(void)
{
    int32 /* bs, */ i, n, cf, nf, w;    /*, *awl; */
    root_chan_t *rhmm;

    if (CurrentFrame >= MAX_FRAMES - 1)
        return;

    BPTableIdx[CurrentFrame] = BPIdx;

    /* Need to renormalize? */
    if ((BestScore + (2 * LogBeamWidth)) < WORST_SCORE) {
        E_INFO("Renormalizing Scores at frame %d, best score %d\n",
               CurrentFrame, BestScore);
        renormalize_scores(BestScore);
    }

    BestScore = WORST_SCORE;

    evaluateChannels();

    /* Apply beam pruning, perform cross-HMM transitions and word-exits */
    pruneChannels();

    /* Apply absolute pruning to word-exits, if specified */
    n = cmd_ln_int32_r(config, "-maxwpf");
    if (n != -1 && n < NumWords)
        bptable_maxwpf(n);

    /* Do inter-word transitions */
    if (BPTableIdx[CurrentFrame] < BPIdx)
        word_transition();

    /* Clear score[] of pruned root channels (UGLY!) */
    cf = CurrentFrame;
    nf = cf + 1;
    for (i = n_root_chan, rhmm = root_chan; i > 0; --i, rhmm++) {
        if (hmm_frame(&rhmm->hmm) == cf) {
            hmm_clear_scores(&rhmm->hmm);
        }
    }
    /* Clear score[] of pruned single-phone channels (UGLY!) */
    for (i = 0; i < n_1ph_words; i++) {
        w = single_phone_wid[i];
        rhmm = (root_chan_t *) word_chan[w];
        if (hmm_frame(&rhmm->hmm) == cf) {
            hmm_clear_scores(&rhmm->hmm);
        }
    }

    /* This code terminates the loop by updating for the next pass */
    CurrentFrame++;
    if (CurrentFrame >= MAX_FRAMES - 1) {
        E_ERROR("MAX_FRAMES (%d) EXCEEDED; IGNORING REST OF UTTERANCE\n",
                MAX_FRAMES);
    }
}

void
search_finish_fwd(void)
{
    int32 i, w, cf, nf, *awl;
    root_chan_t *rhmm;
    chan_t *hmm, **acl;

    BPTableIdx[CurrentFrame] = BPIdx;
    if (CurrentFrame > 0)
        CurrentFrame--;
    LastFrame = CurrentFrame;

    /* Deactivate channels lined up for the next frame */
    cf = CurrentFrame;
    nf = cf + 1;
    /* First, root channels of HMM tree */
    for (i = n_root_chan, rhmm = root_chan; i > 0; --i, rhmm++) {
        hmm_clear(&rhmm->hmm);
    }

    /* nonroot channels of HMM tree */
    i = n_active_chan[nf & 0x1];
    acl = active_chan_list[nf & 0x1];
    for (hmm = *(acl++); i > 0; --i, hmm = *(acl++)) {
        hmm_clear(&hmm->hmm);
    }

    /* word channels */
    i = n_active_word[nf & 0x1];
    awl = active_word_list[nf & 0x1];
    for (w = *(awl++); i > 0; --i, w = *(awl++)) {
        /* Don't accidentally free single-phone words! */
        if (g_word_dict->dict_list[w]->len == 1)
            continue;
        word_active[w] = 0;
        if (word_chan[w] == NULL)
            continue;
        free_all_rc(w);
    }

    /* Obtain lattice density info for this utterance */
    bptbl2latdensity(BPIdx, lattice_density);
    search_postprocess_bptable(1.0, "FWDTREE");

#if SEARCH_PROFILE
    if (LastFrame > 0) {
        E_INFO("%8d words recognized (%d/fr)\n",
               BPIdx, (BPIdx + (LastFrame >> 1)) / (LastFrame + 1));
        E_INFO("%8d senones evaluated (%d/fr)\n", n_senone_active_utt,
               (n_senone_active_utt + (LastFrame >> 1)) / (LastFrame + 1));
        E_INFO("%8d channels searched (%d/fr), %d 1st, %d last\n",
               n_root_chan_eval + n_nonroot_chan_eval,
               (n_root_chan_eval + n_nonroot_chan_eval) / (LastFrame + 1),
               n_root_chan_eval, n_last_chan_eval);
        E_INFO("%8d words for which last channels evaluated (%d/fr)\n",
               n_word_lastchan_eval,
               n_word_lastchan_eval / (LastFrame + 1));
        E_INFO("%8d candidate words for entering last phone (%d/fr)\n",
               n_lastphn_cand_utt, n_lastphn_cand_utt / (LastFrame + 1));
    }
#endif
}

void
search_postprocess_bptable(float32 lwf, char const *pass)
{
    /* register int32 idx; */
    int32 /* i, j, w, */ cf, nf, f;     /*, *awl; */
    /* root_chan_t *rhmm; */
    /* chan_t *hmm, *thmm, **acl; */
    int32 bp;
    int32 l_scr;

    if (LastFrame < 10) {       /* HACK!!  Hardwired constant 10 */
        E_WARN("UTTERANCE TOO SHORT; IGNORED\n");
        LastFrame = 0;

        return;
    }

    /* Deactivate channels lined up for the next frame */
    cf = CurrentFrame;
    nf = cf + 1;

    /*
     * Print final Path
     */
    /* Look for </s> - strangely enough this seems to happen quite
     * frequently.  Maybe a bug? */
    for (bp = BPTableIdx[cf]; bp < BPIdx; bp++) {
        if (BPTable[bp].wid == FinishWordId)
            break;
    }
    if (bp >= BPIdx) {
        int32 bestbp = 0, bestscore = 0;        /* FIXME: good defaults? */
        E_WARN("Failed to terminate in final state\n\n");

        /* Find the most recent frame containing the best BP entry */
        for (f = cf; (f >= 0) && (BPTableIdx[f] == BPIdx); --f);
        if (f < 0) {
            E_WARN("**EMPTY BPTABLE**\n\n");
            return;
        }

        bestscore = WORST_SCORE;
        for (bp = BPTableIdx[f]; bp < BPTableIdx[f + 1]; bp++) {
            int32 n_used;
            l_scr = ngram_tg_score(g_lmset, FinishWordId,
                                   BPTable[bp].real_wid,
                                   BPTable[bp].prev_real_wid,
                                   &n_used);
            l_scr = l_scr * lwf;

            if (BPTable[bp].score + l_scr > bestscore) {
                bestscore = BPTable[bp].score + l_scr;
                bestbp = bp;
            }
        }

        /* Force </s> into bptable */
        CurrentFrame++;
        LastFrame++;
        save_bwd_ptr(FinishWordId, bestscore, bestbp, 0);
        BPTableIdx[CurrentFrame + 1] = BPIdx;
        bp = BPIdx - 1;
    }
    HypTotalScore = BPTable[bp].score;

    compute_seg_scores(lwf);

    seg_back_trace(bp, pass);
    search_remove_context(hyp);
    search_hyp_to_str();

    E_INFO("%s: %s (%s %d (A=%d L=%d))\n",
           pass, hyp_str, uttproc_get_uttid(),
           HypTotalScore, HypTotalScore - TotalLangScore, TotalLangScore);
}

void
bestpath_search(void)
{
    if (!renormalized) {
        lattice_rescore(bestpath_fwdtree_lw_ratio);
    }
    else {
        E_INFO("Renormalized in fwd pass; cannot rescore lattice\n");
    }
}

/*
 * Convert search hypothesis (word-id sequence) to a single string.
 */
void
search_hyp_to_str(void)
{
    int32 i, k, l;
    char const *wd;

    hyp_str[0] = '\0';
    k = 0;
    for (i = 0; hyp[i].wid >= 0; i++) {
        wd = dict_word_str(g_word_dict, hyp[i].wid);
        l = strlen(wd);

        if (k + l > 4090)
            E_FATAL("**ERROR** Increase hyp_str[] size\n");

        strcpy(hyp_str + k, wd);
        k += l;
        hyp_str[k] = ' ';
        k++;
        hyp_str[k] = '\0';
    }
}

int32
seg_topsen_score(int32 sf, int32 ef)
{
    int32 f, sum;

    sum = 0;
    for (f = sf; f <= ef; f++)
        sum += topsen_score[f];

    return (sum);
}

/* SEG_BACK_TRACE
 *-------------------------------------------------------------*
 * Print out the backtrace
 */
static void
seg_back_trace(int32 bpidx, char const *pass)
{
    static int32 last_score;
    static int32 last_time;
    static int32 seg;
    /* int32 *probs; */
    int32 l_scr;
    int32 a_scr;
    int32 a_scr_norm;           /* time normalized acoustic score */
    int32 raw_scr;
    int32 seg_len;
    int32 altpron;
    int32 topsenscr_norm;
    int32 f, latden;

    altpron = cmd_ln_boolean_r(config, "-reportpron");

    /* Stop condition, no more backpointers. */
    if (bpidx != NO_BP) {
        /* This assertion catches loops in the backtrace. */
        assert(BPTable[bpidx].bp < bpidx);
        seg_back_trace(BPTable[bpidx].bp, pass);

        l_scr = BPTable[bpidx].lscr;
        raw_scr = BPTable[bpidx].score - last_score;
        a_scr = raw_scr - l_scr;
        seg_len = BPTable[bpidx].frame - last_time;
        a_scr_norm = ((seg_len == 0) ? 0 : (a_scr / seg_len));
        topsenscr_norm = ((seg_len == 0) ? 0 :
                          seg_topsen_score(last_time + 1,
                                           BPTable[bpidx].frame) /
                          seg_len);

        TotalLangScore += l_scr;

        /* Fill in lattice density information for this word */
        latden = 0;
        for (f = last_time + 1; f <= BPTable[bpidx].frame; f++) {
            latden += lattice_density[f];
        }
        if (seg_len > 0) {
            latden /= seg_len;
        }

        if (cmd_ln_boolean_r(config, "-backtrace"))
            printf("\t%4d %4d %10d %11d %8d %8d %6d %s\n",
                   last_time + 1, BPTable[bpidx].frame,
                   a_scr_norm, a_scr, l_scr,
                   topsenscr_norm,
                   latden,
                   dict_word_str(g_word_dict, BPTable[bpidx].wid));
        hyp_wid[n_hyp_wid++] = BPTable[bpidx].wid;

        /* Store hypothesis word sequence and segmentation */
        if (ISA_REAL_WORD(BPTable[bpidx].wid)) {
            if (seg >= HYP_SZ - 1)
                E_FATAL("**ERROR** Increase HYP_SZ\n");
            hyp[seg].wid = altpron ? BPTable[bpidx].wid :
                g_word_dict->dict_list[BPTable[bpidx].wid]->wid;
            hyp[seg].sf = last_time + 1;
            hyp[seg].ef = BPTable[bpidx].frame;
            hyp[seg].ascr = a_scr;
            hyp[seg].lscr = l_scr;
            hyp[seg].latden = latden;
            seg++;

            hyp[seg].wid = -1;
        }

        last_score = BPTable[bpidx].score;
        last_time = BPTable[bpidx].frame;
    }
    else {
        if (cmd_ln_boolean_r(config, "-backtrace")) {
            printf("\t%4s %4s %10s %11s %8s %8s %6s %s (%s) (%s)\n",
                   "SFrm", "Efrm", "AScr/Frm", "AScr", "LScr", "BSDiff",
                   "LatDen", "Word", pass, uttproc_get_uttid());
            printf
                ("\t---------------------------------------------------------------------\n");
        }
        TotalLangScore = 0;
        last_score = 0;
        last_time = -1;         /* Use -1 to count frame 0 */
        seg = 0;
        hyp[0].wid = -1;
        n_hyp_wid = 0;
    }
}

/* PARTIAL_SEG_BACK_TRACE
 *-------------------------------------------------------------*
 * Like seg_back_trace, but not as detailed.
 */
static void
partial_seg_back_trace(int32 bpidx)
{
    static int32 seg;
    static int32 last_time;
    int32 altpron;

    altpron = cmd_ln_boolean_r(config, "-reportpron");

    /* Stop condition, no more backpointers */
    if (bpidx != NO_BP) {
        partial_seg_back_trace(BPTable[bpidx].bp);

        /* Store hypothesis word sequence and segmentation */
        if (ISA_REAL_WORD(BPTable[bpidx].wid)) {
            if (seg >= HYP_SZ - 1)
                E_FATAL("**ERROR** Increase HYP_SZ\n");
            hyp[seg].wid = altpron ?
                BPTable[bpidx].wid
                : g_word_dict->dict_list[BPTable[bpidx].wid]->wid;
            hyp[seg].sf = last_time + 1;
            hyp[seg].ef = BPTable[bpidx].frame;
            seg++;
            hyp[seg].wid = -1;
        }

        last_time = BPTable[bpidx].frame;
    }
    else {
        last_time = -1;         /* Use -1 to count frame 0 */
        seg = 0;
        hyp[0].wid = -1;
    }
}

static void
renormalize_scores(int32 norm)
{
    root_chan_t *rhmm;
    chan_t *hmm, **acl;
    int32 i, cf, w, *awl;

    cf = CurrentFrame;

    /* Renormalize root channels */
    for (i = n_root_chan, rhmm = root_chan; i > 0; --i, rhmm++) {
        if (hmm_frame(&rhmm->hmm) == cf) {
            hmm_normalize(&rhmm->hmm, norm);
        }
    }

    /* Renormalize nonroot channels in HMM tree */
    i = n_active_chan[cf & 0x1];
    acl = active_chan_list[cf & 0x1];
    for (hmm = *(acl++); i > 0; --i, hmm = *(acl++)) {
        hmm_normalize(&hmm->hmm, norm);
    }

    /* Renormalize individual word channels */
    i = n_active_word[cf & 0x1];
    awl = active_word_list[cf & 0x1];
    for (w = *(awl++); i > 0; --i, w = *(awl++)) {
        for (hmm = word_chan[w]; hmm; hmm = hmm->next) {
            hmm_normalize(&hmm->hmm, norm);
        }
    }
    for (i = 0; i < n_1ph_words; i++) {
        w = single_phone_wid[i];
        rhmm = (root_chan_t *) word_chan[w];
        if (hmm_frame(&rhmm->hmm) == cf) {
            hmm_normalize(&rhmm->hmm, norm);
        }
    }

    renormalized = 1;
}

int32
search_get_score(void)
/*---------------------*/
{
    return HypTotalScore;
}

int32
search_get_lscr(void)
{
    return TotalLangScore;
}

void
search_set_beam_width(double beam)
{
    DynamicLogBeamWidth = LogBeamWidth = logmath_log(g_lmath, beam);
    E_INFO("%8d = beam width\n", LogBeamWidth);
}

/* SEARCH_SET_NEW_WORD_BEAM
 *-------------------------------------------------------------*
 */
void
search_set_new_word_beam_width(double beam)
{
    NewWordLogBeamWidth = logmath_log(g_lmath, beam);
    E_INFO("%8d = new word beam width\n", NewWordLogBeamWidth);
}

/* SEARCH_SET_LASTPHONE_ALONE_BEAM_WIDTH
 *-------------------------------------------------------------*
 */
void
search_set_lastphone_alone_beam_width(double beam)
{
    LastPhoneAloneLogBeamWidth = logmath_log(g_lmath, beam);
    E_INFO("%8d = Last phone alone beam width\n",
           LastPhoneAloneLogBeamWidth);
}

/* SEARCH_SET_NEW_PHONE_BEAM
 *-------------------------------------------------------------*
 */
void
search_set_new_phone_beam_width(double beam)
{
    NewPhoneLogBeamWidth = logmath_log(g_lmath, beam);
    E_INFO("%8d = new phone beam width\n", NewPhoneLogBeamWidth);
}

/* SEARCH_SET_LAST_PHONE_BEAM
 *-------------------------------------------------------------*
 */
void
search_set_last_phone_beam_width(double beam)
{
    LastPhoneLogBeamWidth = logmath_log(g_lmath, beam);
    E_INFO("%8d = last phone beam width\n", LastPhoneLogBeamWidth);
}

/* SEARCH_SET_CHANNELS_PER_FRAME_TARGET
 *-------------------------------------------------------------*
 */
void
search_set_channels_per_frame_target(int32 cpf)
{
    ChannelsPerFrameTarget = cpf;
}

int32
searchFrame(void)
{
    return LastFrame;
}

void
searchSetFrame(int32 frame)
{
    LastFrame = frame;
}

int32
searchCurrentFrame(void)
{
    return CurrentFrame;
}

void
search_set_newword_penalty(double nw_pen)
{
    newword_penalty = logmath_log(g_lmath, nw_pen);
    E_INFO("%8d = newword penalty\n", newword_penalty);
}

void
search_set_silence_word_penalty(float pen, float pip)
{                               /* Phone insertion penalty */
    logPhoneInsertionPenalty = logmath_log(g_lmath, pip);
    SilenceWordPenalty = logmath_log(g_lmath, pen) + logmath_log(g_lmath, pip);
    E_INFO("%8d = LOG (Silence Word Penalty) + LOG (Phone Penalty)\n",
           SilenceWordPenalty);
}

int32
search_get_logs2pip(void)
{
    return logPhoneInsertionPenalty;
}

void
search_set_filler_word_penalty(float pen, float pip)
{
    FillerWordPenalty = logmath_log(g_lmath, pen) + logmath_log(g_lmath, pip);;
    E_INFO("%8d = LOG (Filler Word Penalty) + LOG (Phone Penalty)\n",
           FillerWordPenalty);
}

void
search_set_lw(double p1lw, double p2lw, double p3lw)
{
    fwdtree_lw = p1lw;
    fwdflat_lw = p2lw;
    bestpath_lw = p3lw;
    bestpath_fwdtree_lw_ratio = p3lw / p1lw;
    fwdflat_fwdtree_lw_ratio = p2lw / p1lw;

    E_INFO("LW = fwdtree: %.1f, fwdflat: %.1f, bestpath: %.1f\n",
           fwdtree_lw, fwdflat_lw, bestpath_lw);
}

void
search_set_ip(float ip)
{
    LogInsertionPenalty = logmath_log(g_lmath, ip);
}

void
search_set_hyp_alternates(int32 arg)
{
    hyp_alternates = arg;
    if (hyp_alternates)
        E_INFO("Will report alternate hypotheses\n");
    else
        E_INFO("Will NOT report alternate hypotheses\n");
}

void
search_set_skip_alt_frm(int32 flag)
{
    skip_alt_frm = flag;
}

/*
 * Return the best path in the bptable until now.
 */
int32
search_partial_result(int32 * fr, char **res)
{
    int32 bp, bestscore = 0, bestbp = 0, f;     /* FIXME: good defaults? */

    bestscore = WORST_SCORE;
    f = CurrentFrame - 1;

    /* Look for the last frame with word exits */
    for (; f >= 0; --f) {
        for (bp = BPTableIdx[f]; bp < BPIdx; bp++) {
            if (BPTable[bp].score > bestscore) {
                bestscore = BPTable[bp].score;
                bestbp = bp;
            }
        }
        if (bestscore > WORST_SCORE)
            break;
    }

    if (f >= 0) {
        partial_seg_back_trace(bestbp);
        search_hyp_to_str();
    }
    else
        hyp_str[0] = '\0';

    *fr = CurrentFrame;
    *res = hyp_str;

    return 0;
}

/* SEARCH_RESULT
 *-------------------------------------------------------------*
 * Return the result of the search.
 */
int32
search_result(int32 * fr, char **res)
{
    *fr = LastFrame;
    *res = hyp_str;

    return 0;
}

/*
 * Return recognized word-id sequence; Note that the user cannot clobber it,
 * and must make a copy of it, if needed over the long term.
 */
search_hyp_t *
search_get_hyp(void)
{
    return (hyp);
}

char *
search_get_wordlist(int *len, char sep_char)
{
    dict_entry_t **dents = g_word_dict->dict_list;
    int32 dent_cnt = g_word_dict->dict_entry_count;
    int32 i, p;
    static char *fwrdl = NULL;
    static int32 flen = 0;

    /* malloc memory for all word strings in one shot; do it only once */
    if (fwrdl == NULL) {
        for (i = 0, flen = 0; i < dent_cnt; i++)
            flen += strlen(dents[i]->word) + 1; /* +1 for sep_char */

        ++flen;                 /* for the terminal '\0' */

        fwrdl = ckd_calloc(flen, sizeof(char));

        for (i = 0, p = 0; i < dent_cnt; i++) {
            strcpy(&fwrdl[p], dents[i]->word);
            p += strlen(dents[i]->word);
            fwrdl[p++] = sep_char;
        }
        fwrdl[p++] = '\0';
    }

    *len = flen;
    return fwrdl;
}

void
search_filtered_endpts(void)
{
    E_FATAL("search_filtered_endpts() not implemented\n");
}

static int32 *first_phone_rchan_map;    /* map 1st (left) diphone to root-chan index */
static root_chan_t *all_rhmm; /* All root HMMs */

/*
 * Allocate that part of the search channel tree structure that is independent of the
 * LM in use.
 */
void
init_search_tree(dict_t * dict)
{
    int32 w, mpx, max_ph0, i;
    dict_entry_t *de;

    homophone_set =
        ckd_calloc(NumMainDictWords, sizeof(int32));

    /* Find #single phone words, and #unique first diphones (#root channels) in dict. */
    max_ph0 = -1;
    n_1ph_words = 0;
    mpx = dict->dict_list[0]->mpx;
    for (w = 0; w < NumMainDictWords; w++) {
        de = dict->dict_list[w];

        /* Paranoia Central; this check can probably be removed (RKM) */
        if (de->mpx != mpx)
            E_FATAL("HMM tree words not all mpx or all non-mpx\n");

        if (de->len == 1)
            n_1ph_words++;
        else {
            if (max_ph0 < de->phone_ids[0])
                max_ph0 = de->phone_ids[0];
        }
    }

    /* Add remaining dict words (</s>, <s>, <sil>, noise words) to single-phone words */
    n_1ph_words += (NumWords - NumMainDictWords);
    n_root_chan_alloc = max_ph0 + 1;

    /* Allocate and initialize root channels */
    root_chan =
        ckd_calloc(n_root_chan_alloc, sizeof(root_chan_t));
    for (i = 0; i < n_root_chan_alloc; i++) {
        hmm_init(hmmctx, &root_chan[i].hmm, mpx, -1, -1);
        root_chan[i].penult_phn_wid = -1;
        root_chan[i].next = NULL;
    }

    /* Allocate space for left-diphone -> root-chan map */
    first_phone_rchan_map =
        ckd_calloc(n_root_chan_alloc, sizeof(int32));

    /* Permanently allocate channels for single-phone words (1/word) */
    all_rhmm = ckd_calloc(n_1ph_words, sizeof(root_chan_t));
    i = 0;
    for (w = 0; w < NumWords; w++) {
        de = g_word_dict->dict_list[w];
        if (de->len != 1)
            continue;

        all_rhmm[i].diphone = de->phone_ids[0];
        all_rhmm[i].ciphone = de->ci_phone_ids[0];
        hmm_init(hmmctx, &all_rhmm[i].hmm, de->mpx,
                 de->phone_ids[0], de->ci_phone_ids[0]);
        all_rhmm[i].next = NULL;

        word_chan[w] = (chan_t *) &(all_rhmm[i]);
        i++;
    }

    single_phone_wid = ckd_calloc(n_1ph_words, sizeof(int32));
}

/*
 * One-time initialization of internal channels in HMM tree.
 */
static void
init_nonroot_chan(chan_t * hmm, int32 ph, int32 ci)
{
    hmm->next = NULL;
    hmm->alt = NULL;
    hmm->info.penult_phn_wid = -1;
    hmm->ciphone = ci;
    hmm_init(hmmctx, &hmm->hmm, FALSE, ph, ci);
}

/*
 * Allocate and initialize search channel-tree structure.  If (use_lm) do so wrt the
 * currently active LM for the next utterance (i.e., ignore words not in LM).
 * At this point, all the root-channels have been allocated and partly initialized
 * (as per init_search_tree()), and channels for all the single-phone words have been
 * allocated and initialized.  None of the interior channels of search-trees have
 * been allocated.
 * This routine may be called on every utterance, after delete_search_tree() clears
 * the search tree created for the previous utterance.  Meant for reconfiguring the
 * search tree to suit the currently active LM.
 */
void
create_search_tree(dict_t * dict, int32 use_lm)
{
    dict_entry_t *de;
    chan_t *hmm;
    root_chan_t *rhmm;
    int32 w, i, j, p, ph;

    if (use_lm)
        E_INFO("Creating search tree\n");
    else
        E_INFO("Estimating maximal search tree\n");

    for (w = 0; w < NumMainDictWords; w++)
        homophone_set[w] = -1;

    for (i = 0; i < n_root_chan_alloc; i++)
        first_phone_rchan_map[i] = -1;

    n_1ph_LMwords = 0;
    n_root_chan = 0;
    n_nonroot_chan = 0;

    for (w = 0; w < NumMainDictWords; w++) {
        de = dict->dict_list[w];

        /* Ignore dictionary words not in LM */
        /* NOTE: This leaves open the possibility for doing
         * open-vocabulary decoding in the future... */
        if (use_lm && !ngram_model_set_known_wid(g_lmset, de->wid)) {
            /* E_INFO("Skipping word %s, not in LM\n", de->word); */
            continue;
        }

        /* Handle single-phone words individually; not in channel tree */
        if (de->len == 1) {
            single_phone_wid[n_1ph_LMwords++] = w;
            continue;
        }

        /* Insert w into channel tree; first find or allocate root channel */
        if (first_phone_rchan_map[de->phone_ids[0]] < 0) {
            first_phone_rchan_map[de->phone_ids[0]] = n_root_chan;
            rhmm = &(root_chan[n_root_chan]);
            if (hmm_is_mpx(&rhmm->hmm))
                rhmm->hmm.s.mpx_ssid[0] = de->phone_ids[0];
            else
                rhmm->hmm.s.ssid = de->phone_ids[0];
            rhmm->hmm.tmatid = de->ci_phone_ids[0];
            rhmm->diphone = de->phone_ids[0];
            rhmm->ciphone = de->ci_phone_ids[0];

            n_root_chan++;
        }
        else
            rhmm = &(root_chan[first_phone_rchan_map[de->phone_ids[0]]]);

        /* Now, rhmm = root channel for w.  Go on to remaining phones */
        if (de->len == 2) {
            /* Next phone is the last; not kept in tree; add w to penult_phn_wid set */
            if ((j = rhmm->penult_phn_wid) < 0)
                rhmm->penult_phn_wid = w;
            else {
                for (; homophone_set[j] >= 0; j = homophone_set[j]);
                homophone_set[j] = w;
            }
        }
        else {
            /* Add remaining phones, except the last, to tree */
            ph = de->phone_ids[1];
            hmm = rhmm->next;
            if (hmm == NULL) {
                rhmm->next = hmm = listelem_malloc(chan_alloc);
                init_nonroot_chan(hmm, ph, de->ci_phone_ids[1]);
                n_nonroot_chan++;
            }
            else {
                chan_t *prev_hmm = NULL;

                for (; hmm && (hmm->hmm.s.ssid != ph); hmm = hmm->alt)
                    prev_hmm = hmm;
                if (!hmm) {     /* thanks, rkm! */
                    prev_hmm->alt = hmm = listelem_malloc(chan_alloc);
                    init_nonroot_chan(hmm, ph, de->ci_phone_ids[1]);
                    n_nonroot_chan++;
                }
            }
            /* de->phone_ids[1] now in tree; pointed to by hmm */

            for (p = 2; p < de->len - 1; p++) {
                ph = de->phone_ids[p];
                if (!hmm->next) {
                    hmm->next = listelem_malloc(chan_alloc);
                    hmm = hmm->next;
                    init_nonroot_chan(hmm, ph, de->ci_phone_ids[p]);
                    n_nonroot_chan++;
                }
                else {
                    chan_t *prev_hmm = NULL;

                    for (hmm = hmm->next; hmm && (hmm->hmm.s.ssid != ph);
                         hmm = hmm->alt)
                        prev_hmm = hmm;
                    if (!hmm) { /* thanks, rkm! */
                        prev_hmm->alt = hmm = listelem_malloc(chan_alloc);
                        init_nonroot_chan(hmm, ph, de->ci_phone_ids[p]);
                        n_nonroot_chan++;
                    }
                }
            }

            /* All but last phone of w in tree; add w to hmm->info.penult_phn_wid set */
            if ((j = hmm->info.penult_phn_wid) < 0)
                hmm->info.penult_phn_wid = w;
            else {
                for (; homophone_set[j] >= 0; j = homophone_set[j]);
                homophone_set[j] = w;
            }
        }
    }

    n_1ph_words = n_1ph_LMwords;
    n_1ph_LMwords++;            /* including </s> */

    for (w = FinishWordId; w < NumWords; w++) {
        de = dict->dict_list[w];
        if (use_lm
            && (!(ISA_FILLER_WORD(w)))
            && (!ngram_model_set_known_wid(g_lmset, de->wid))) {
            /* E_INFO("Skipping word %s, not in LM\n", de->word); */
            continue;
        }

        single_phone_wid[n_1ph_words++] = w;
    }

    if (max_nonroot_chan < n_nonroot_chan + 1) {
        /* Give some room for channels for new words added dynamically at run time */
        max_nonroot_chan = n_nonroot_chan + 128;
        E_INFO("max nonroot chan increased to %d\n", max_nonroot_chan);

        /* Free old active channel list array if any and allocate new one */
        if (active_chan_list[0] != NULL)
            free(active_chan_list[0]);
        active_chan_list[0] =
            ckd_calloc(max_nonroot_chan * 2, sizeof(chan_t *));
        active_chan_list[1] = active_chan_list[0] + max_nonroot_chan;
    }

    E_INFO("%d root, %d non-root channels, %d single-phone words\n",
           n_root_chan, n_nonroot_chan, n_1ph_words);

#if 0
    E_INFO("Main Dictionary:\n");
    for (w = 0; w < NumWords; w++) {
        de = dict->dict_list[w];
        printf("%s", de->word);
        for (i = 0; i < de->len; i++)
            printf(" %d", de->phone_ids[i]);
        printf("\n");
    }
    printf("\n");

    E_INFO("Single-phone words:\n");
    for (i = 0; i < n_1ph_words; i++) {
        printf("    %5d %s\n",
               single_phone_wid[i],
               dict->dict_list[single_phone_wid[i]]->word);
    }
    printf("\n");

    E_INFO("HMM tree:\n");
    for (i = 0; i < n_root_chan; i++)
        dump_search_tree_root(dict, root_chan + i);
    printf("\n");
#endif
}

#if 0
int32 mid_stk[100];
static
dump_search_tree_root(dict_t * dict, root_chan_t * hmm)
{
    int32 i;
    chan_t *t;
    dict_entry_t *de;

    printf(" %d(%d):", hmm->diphone, hmm_is_mpx(&hmm->hmm));

    for (i = hmm->penult_phn_wid; i >= 0; i = homophone_set[i]) {
        de = dict->dict_list[i];
        printf("     %d: %s\n", de->phone_ids[de->len - 1], de->word);
    }

    printf("    ");
    for (t = hmm->next; t; t = t->alt)
        printf(" %d", hmm_ssid(&t->hmm, 0));
    printf("\n");

    mid_stk[0] = hmm->diphone;
    if (hmm_is_mpx(&hmm->hmm))
        mid_stk[0] |= 0x80000000;
    for (t = hmm->next; t; t = t->alt)
        dump_search_tree(dict, t, 1);
}

static
dump_search_tree(dict_t * dict, chan_t * hmm, int32 level)
{
    int32 i;
    chan_t *t;
    dict_entry_t *de;

    printf(" %d(%d)", mid_stk[0] & 0x7fffffff, (mid_stk[0] < 0));
    for (i = 1; i < level; i++)
        printf(" %d", mid_stk[i]);
    printf(" %d:\n", hmm_nonmpx_ssid(&hmm->hmm));

    for (i = hmm->info.penult_phn_wid; i >= 0; i = homophone_set[i]) {
        de = dict->dict_list[i];
        printf("     %d: %s\n", de->phone_ids[de->len - 1], de->word);
    }

    printf("    ");
    for (t = hmm->next; t; t = t->alt)
        printf(" %d", hmm_nonmpx_ssid(&t->hmm));
    printf("\n");

    mid_stk[level] = hmm_nonmpx_ssid(&hmm->hmm);
    for (t = hmm->next; t; t = t->alt)
        dump_search_tree(dict, t, level + 1);
}
#endif

/*
 * Delete search tree by freeing all interior channels within search tree and
 * restoring root channel state to the init state (i.e., just after init_search_tree()).
 */
void
delete_search_tree(void)
{
    int32 i;
    chan_t *hmm, *sibling;

    for (i = 0; i < n_root_chan; i++) {
        hmm = root_chan[i].next;

        while (hmm) {
            sibling = hmm->alt;
            delete_search_subtree(hmm);
            hmm = sibling;
        }

        root_chan[i].penult_phn_wid = -1;
        root_chan[i].next = NULL;
    }
}

void
delete_search_subtree(chan_t * hmm)
{
    chan_t *child, *sibling;

    /* First free all children under hmm */
    for (child = hmm->next; child; child = sibling) {
        sibling = child->alt;
        delete_search_subtree(child);
    }

    /* Now free hmm */
    hmm_deinit(&hmm->hmm);
    listelem_free(chan_alloc, hmm);
}

void
free_search_tree(void)
{
    int i, w;
    delete_search_tree();
    for (i = 0; i < n_root_chan_alloc; i++) {
        hmm_deinit(&root_chan[i].hmm);
    }
    for (i = w = 0; w < NumWords; w++) {
        if (g_word_dict->dict_list[w]->len != 1)
            continue;
        hmm_deinit(&all_rhmm[i++].hmm);
    }
    ckd_free(all_rhmm);
    ckd_free(first_phone_rchan_map);
    ckd_free(root_chan);
    ckd_free(homophone_set);
    ckd_free(single_phone_wid);
}

/*
 * Switch search module to new LM.  NOTE: The LM module should have been switched first.
 */
void
search_set_current_lm(void)
{
    delete_search_tree();
    create_search_tree(g_word_dict, 1);
}

/*
 * Compute acoustic and LM scores for each BPTable entry (segment).
 */
void
compute_seg_scores(float32 lwf)
{
    int32 bp, start_score;
    BPTBL_T *bpe, *p_bpe;
    int32 *rcpermtab;
    dict_entry_t *de;

    for (bp = 0; bp < BPIdx; bp++) {

        bpe = &(BPTable[bp]);

        if (bpe->bp == NO_BP) {
            bpe->ascr = bpe->score;
            bpe->lscr = 0;
            continue;
        }

        de = g_word_dict->dict_list[bpe->wid];
        p_bpe = &(BPTable[bpe->bp]);
        rcpermtab = (p_bpe->r_diph >= 0) ?
            g_word_dict->rcFwdPermTable[p_bpe->r_diph] : zeroPermTab;
        start_score =
            BScoreStack[p_bpe->s_idx + rcpermtab[de->ci_phone_ids[0]]];

        if (bpe->wid == SilenceWordId) {
            bpe->lscr = SilenceWordPenalty;
        }
        else if (ISA_FILLER_WORD(bpe->wid)) {
            bpe->lscr = FillerWordPenalty;
        }
        else {
            int32 n_used;
            bpe->lscr = ngram_tg_score(g_lmset, de->wid,
                                       p_bpe->real_wid,
                                       p_bpe->prev_real_wid, &n_used);
            bpe->lscr = bpe->lscr * lwf;
        }
        bpe->ascr = bpe->score - start_score - bpe->lscr;
    }
}

int32
search_get_bptable_size(void)
{
    return (BPIdx);
}

int32 *
search_get_lattice_density(void)
{
    return lattice_density;
}

void
search_set_hyp_total_score(int32 score)
{
    HypTotalScore = score;
}

void
search_set_hyp_total_lscr(int32 lscr)
{
    TotalLangScore = lscr;
}

int32
search_get_sil_penalty(void)
{
    return (SilenceWordPenalty);
}

int32
search_get_filler_penalty(void)
{
    return (FillerWordPenalty);
}

BPTBL_T *
search_get_bptable(void)
{
    return (BPTable);
}

int32 *
search_get_bscorestack(void)
{
    return (BScoreStack);
}

float32
search_get_lw(void)
{
    return (fwdtree_lw);
}

/* --------------- Re-process lattice a la fbs6 ----------------- */

static latnode_t *frm_wordlist[MAX_FRAMES];
static int32 *fwdflat_wordlist = NULL;

static int32 n_fwdflat_chan;
static int32 n_fwdflat_words;
static int32 n_fwdflat_word_transition;

static char *expand_word_flag = NULL;
static int32 *expand_word_list = NULL;
static int32 n_expand_words;

static int32 min_ef_width = 4;
static int32 max_sf_win = 25;

static void
build_fwdflat_wordlist(void)
{
    int32 i, f, sf, ef, wid, nwd;
    BPTBL_T *bp;
    latnode_t *node, *prevnode, *nextnode;
    dict_entry_t *de;

    if (!cmd_ln_boolean_r(config, "-fwdtree")) {
        /* No tree-search run; include all words in expansion list */
        nwd = 0;
        for (i = 0; i < StartWordId; i++) {
            if (ngram_model_set_known_wid(g_lmset, g_word_dict->dict_list[i]->wid))
                fwdflat_wordlist[nwd++] = i;
        }
        fwdflat_wordlist[nwd] = -1;
        return;
    }

    memset(frm_wordlist, 0, MAX_FRAMES * sizeof(latnode_t *));

    /* Scan the backpointer table for all active words and record
     * their exit frames. */
    for (i = 0, bp = BPTable; i < BPIdx; i++, bp++) {
        sf = (bp->bp < 0) ? 0 : BPTable[bp->bp].frame + 1;
        ef = bp->frame;
        wid = bp->wid;

        /* Ignore silence and <s> */
        if ((wid >= SilenceWordId) || (wid == StartWordId))
            continue;

        /* Look for it in the wordlist. */
        de = g_word_dict->dict_list[wid];
        for (node = frm_wordlist[sf]; node && (node->wid != wid);
             node = node->next);

        /* Update last end frame. */
        if (node)
            node->lef = ef;
        else {
            /* New node; link to head of list */
            node = listelem_malloc(latnode_alloc);
            node->wid = wid;
            node->fef = node->lef = ef;

            node->next = frm_wordlist[sf];
            frm_wordlist[sf] = node;
        }
    }

    /* Eliminate "unlikely" words, for which there are too few end points */
    for (f = 0; f <= LastFrame; f++) {
        prevnode = NULL;
        for (node = frm_wordlist[f]; node; node = nextnode) {
            nextnode = node->next;
            if ((node->lef - node->fef < min_ef_width) ||
                ((node->wid == FinishWordId)
                 && (node->lef < LastFrame - 1))) {
                if (!prevnode)
                    frm_wordlist[f] = nextnode;
                else
                    prevnode->next = nextnode;
                listelem_free(latnode_alloc, node);
            }
            else
                prevnode = node;
        }
    }

    /* Form overall wordlist for 2nd pass */
    nwd = 0;

    memset(word_active, 0, NumWords * sizeof(int32));
    for (f = 0; f <= LastFrame; f++) {
        for (node = frm_wordlist[f]; node; node = node->next) {
            if (!word_active[node->wid]) {
                word_active[node->wid] = 1;
                fwdflat_wordlist[nwd++] = node->wid;
            }
        }
    }
    fwdflat_wordlist[nwd] = -1;

    /*
     * NOTE: fwdflat_wordlist excludes <s>, <sil> and noise words; it includes </s>.
     * That is, it includes anything to which a transition can be made in the LM.
     */
}

void
destroy_frm_wordlist(void)
{
    latnode_t *node, *tnode;
    int32 f;

    if (!cmd_ln_boolean_r(config, "-fwdtree"))
        return;

    for (f = 0; f <= LastFrame; f++) {
        for (node = frm_wordlist[f]; node; node = tnode) {
            tnode = node->next;
            listelem_free(latnode_alloc, node);
        }
    }
}

void
build_fwdflat_chan(void)
{
    int32 i, wid, p;
    dict_entry_t *de;
    root_chan_t *rhmm;
    chan_t *hmm, *prevhmm;

    /* Build word HMMs for each word in the lattice. */
    for (i = 0; fwdflat_wordlist[i] >= 0; i++) {
        wid = fwdflat_wordlist[i];
        de = g_word_dict->dict_list[wid];

        /* Omit single-phone words as they are permanently allocated */
        if (de->len == 1)
            continue;

        assert(de->mpx);
        assert(word_chan[wid] == NULL);

        /* Multiplex root HMM for first phone (one root per word, flat
         * lexicon) */
        rhmm = listelem_malloc(root_chan_alloc);
        rhmm->diphone = de->phone_ids[0];
        rhmm->ciphone = de->ci_phone_ids[0];
        rhmm->next = NULL;
        hmm_init(hmmctx, &rhmm->hmm, TRUE, rhmm->diphone, rhmm->ciphone);

        /* HMMs for word-internal phones */
        prevhmm = NULL;
        for (p = 1; p < de->len - 1; p++) {
            hmm = listelem_malloc(chan_alloc);
            hmm->ciphone = de->ci_phone_ids[p];
            hmm->info.rc_id = p + 1 - de->len;
            hmm->next = NULL;
            hmm_init(hmmctx, &hmm->hmm, FALSE, de->phone_ids[p], hmm->ciphone);

            if (prevhmm)
                prevhmm->next = hmm;
            else
                rhmm->next = hmm;

            prevhmm = hmm;
        }

        /* Right-context phones */
        alloc_all_rc(wid);

        /* Link in just allocated right-context phones */
        if (prevhmm)
            prevhmm->next = word_chan[wid];
        else
            rhmm->next = word_chan[wid];
        word_chan[wid] = (chan_t *) rhmm;
    }
}

void
destroy_fwdflat_chan(void)
{
    int32 i, wid;               /*, p; */
    dict_entry_t *de;

    for (i = 0; fwdflat_wordlist[i] >= 0; i++) {
        wid = fwdflat_wordlist[i];
        de = g_word_dict->dict_list[wid];

        if (de->len == 1)
            continue;

        assert(de->mpx);
        assert(word_chan[wid] != NULL);

        free_all_rc(wid);
    }
}

void
search_set_fwdflat_bw(double bw, double nwbw)
{
    FwdflatLogBeamWidth = logmath_log(g_lmath, bw);
    FwdflatLogWordBeamWidth = logmath_log(g_lmath, nwbw);
    E_INFO("Flat-pass bw = %.1e (%d), nwbw = %.1e (%d)\n",
           bw, FwdflatLogBeamWidth, nwbw, FwdflatLogWordBeamWidth);
}

void
search_fwdflat_start(void)
{
    int32 i;
    root_chan_t *rhmm;

    build_fwdflat_wordlist();
    build_fwdflat_chan();

    BPIdx = 0;
    BSSHead = 0;
    BPTblOflMsg = 0;
    CurrentFrame = 0;

    for (i = 0; i < NumWords; i++)
        WordLatIdx[i] = NO_BP;

    /* Start search with <s>; word_chan[<s>] is permanently allocated */
    rhmm = (root_chan_t *) word_chan[StartWordId];
    hmm_enter(&rhmm->hmm, 0, NO_BP, 0);

    active_word_list[0][0] = StartWordId;
    n_active_word[0] = 1;

    BestScore = 0;
    renormalized = 0;

    for (i = 0; i < NumWords; i++)
        last_ltrans[i].sf = -1;

    hyp_str[0] = '\0';
    hyp[0].wid = -1;

    n_fwdflat_chan = 0;
    n_fwdflat_words = 0;
    n_fwdflat_word_transition = 0;
    n_senone_active_utt = 0;

    compute_all_senones = cmd_ln_boolean_r(config, "-compallsen");

    if (!cmd_ln_boolean_r(config, "-fwdtree")) {
        /* No tree-search run; include all words in language model
         * (upto </s>) in expansion list */
        n_expand_words = 0;

        for (i = 0; i < StartWordId; i++) {
            if (ngram_model_set_known_wid(g_lmset,
                                          g_word_dict->dict_list[i]->wid)) {
                expand_word_list[n_expand_words] = i;
                expand_word_flag[i] = 1;
                n_expand_words++;
            }
            else
                expand_word_flag[i] = 0;
        }
        expand_word_list[n_expand_words] = -1;
    }
}

void
search_fwdflat_frame(mfcc_t **feat)
{
    int32 nf, i, j;
    int32 *nawl;

    if (CurrentFrame >= MAX_FRAMES - 1)
        return;

    if (!compute_all_senones) {
        compute_fwdflat_senone_active();
        senscr_active(feat, CurrentFrame);
    }
    else {
        senscr_all(feat, CurrentFrame);
    }
    n_senone_active_utt += n_senone_active;

    BPTableIdx[CurrentFrame] = BPIdx;

    /* Need to renormalize? */
    if ((BestScore + (2 * LogBeamWidth)) < WORST_SCORE) {
        E_INFO("Renormalizing Scores at frame %d, best score %d\n",
               CurrentFrame, BestScore);
        fwdflat_renormalize_scores(BestScore);
    }

    BestScore = WORST_SCORE;
    fwdflat_eval_chan();
    fwdflat_prune_chan();
    fwdflat_word_transition();

    /* Create next active word list */
    nf = CurrentFrame + 1;
    nawl = active_word_list[nf & 0x1];
    for (i = 0, j = 0; fwdflat_wordlist[i] >= 0; i++) {
        if (word_active[fwdflat_wordlist[i]]) {
            *(nawl++) = fwdflat_wordlist[i];
            j++;
        }
    }
    for (i = StartWordId; i < NumWords; i++) {
        if (word_active[i]) {
            *(nawl++) = i;
            j++;
        }
    }
    n_active_word[nf & 0x1] = j;

    /* This code terminates the loop by updating for the next pass */
    CurrentFrame = nf;
    if (CurrentFrame >= MAX_FRAMES - 1) {
        E_ERROR("MAX_FRAMES (%d) EXCEEDED; IGNORING REST OF UTTERANCE\n",
                MAX_FRAMES);
    }
}

void
compute_fwdflat_senone_active(void)
{
    int32 i, cf, w;
    int32 *awl;
    root_chan_t *rhmm;
    chan_t *hmm;

    sen_active_clear();

    cf = CurrentFrame;

    i = n_active_word[cf & 0x1];
    awl = active_word_list[cf & 0x1];

    for (w = *(awl++); i > 0; --i, w = *(awl++)) {
        rhmm = (root_chan_t *) word_chan[w];
        if (hmm_frame(&rhmm->hmm) == cf) {
            hmm_sen_active(&rhmm->hmm);
        }

        for (hmm = rhmm->next; hmm; hmm = hmm->next) {
            if (hmm_frame(&hmm->hmm) == cf) {
                hmm_sen_active(&hmm->hmm);
            }
        }
    }

    sen_active_flags2list();
}

void
fwdflat_eval_chan(void)
{
    int32 i, cf, w, bestscore;
    int32 *awl;
    root_chan_t *rhmm;
    chan_t *hmm;

    cf = CurrentFrame;
    i = n_active_word[cf & 0x1];
    awl = active_word_list[cf & 0x1];
    bestscore = WORST_SCORE;

    n_fwdflat_words += i;

    hmm_context_set_senscore(hmmctx, senone_scores);

    /* Scan all active words. */
    for (w = *(awl++); i > 0; --i, w = *(awl++)) {
        rhmm = (root_chan_t *) word_chan[w];
        if (hmm_frame(&rhmm->hmm) == cf) {
            chan_v_eval(rhmm);
            n_fwdflat_chan++;
        }
        if ((bestscore < hmm_bestscore(&rhmm->hmm)) && (w != FinishWordId))
            bestscore = hmm_bestscore(&rhmm->hmm);

        for (hmm = rhmm->next; hmm; hmm = hmm->next) {
            if (hmm_frame(&hmm->hmm) == cf) {
                int32 score;

                score = chan_v_eval(hmm);
                if (bestscore < score)
                    bestscore = score;
                n_fwdflat_chan++;
            }
        }
    }

    BestScore = bestscore;
}

void
fwdflat_prune_chan(void)
{
    int32 i, cf, nf, w, pip, newscore, thresh, wordthresh;
    int32 *awl;
    root_chan_t *rhmm;
    chan_t *hmm, *nexthmm;
    dict_entry_t *de;

    cf = CurrentFrame;
    nf = cf + 1;
    i = n_active_word[cf & 0x1];
    awl = active_word_list[cf & 0x1];
    memset(word_active, 0, NumWords * sizeof(int32));

    thresh = BestScore + FwdflatLogBeamWidth;
    wordthresh = BestScore + FwdflatLogWordBeamWidth;
    pip = logPhoneInsertionPenalty;

    /* Scan all active words. */
    for (w = *(awl++); i > 0; --i, w = *(awl++)) {
        de = g_word_dict->dict_list[w];

        rhmm = (root_chan_t *) word_chan[w];
        /* Propagate active root channels */
        if (hmm_frame(&rhmm->hmm) == cf
            && hmm_bestscore(&rhmm->hmm) > thresh) {
            hmm_frame(&rhmm->hmm) = nf;
            word_active[w] = 1;

            /* Transitions out of root channel */
            newscore = hmm_out_score(&rhmm->hmm);
            if (rhmm->next) {
                assert(de->len > 1);

                newscore += pip;
                if (newscore > thresh) {
                    hmm = rhmm->next;
                    /* Enter all right context phones */
                    if (hmm->info.rc_id >= 0) {
                        for (; hmm; hmm = hmm->next) {
                            if ((hmm_frame(&hmm->hmm) < cf)
                                || (hmm_in_score(&hmm->hmm) < newscore)) {
                                hmm_enter(&hmm->hmm, newscore,
                                          hmm_out_history(&rhmm->hmm), nf);
                            }
                        }
                    }
                    /* Just a normal word internal phone */
                    else {
                        if ((hmm_frame(&hmm->hmm) < cf)
                            || (hmm_in_score(&hmm->hmm) < newscore)) {
                                hmm_enter(&hmm->hmm, newscore,
                                          hmm_out_history(&rhmm->hmm), nf);
                        }
                    }
                }
            }
            else {
                assert(de->len == 1);

                /* Word exit for single-phone words (where did their
                 * whmms come from?) */
                if (newscore > wordthresh) {
                    save_bwd_ptr(w, newscore,
                                 hmm_out_history(&rhmm->hmm), 0);
                }
            }
        }

        /* Transitions out of non-root channels. */
        for (hmm = rhmm->next; hmm; hmm = hmm->next) {
            if (hmm_frame(&hmm->hmm) >= cf) {
                /* Propagate forward HMMs inside the beam. */
                if (hmm_bestscore(&hmm->hmm) > thresh) {
                    hmm_frame(&hmm->hmm) = nf;
                    word_active[w] = 1;

                    newscore = hmm_out_score(&hmm->hmm);
                    /* Word-internal phones */
                    if (hmm->info.rc_id < 0) {
                        newscore += pip;
                        if (newscore > thresh) {
                            nexthmm = hmm->next;
                            /* Enter all right-context phones. */
                            if (nexthmm->info.rc_id >= 0) {
                                for (; nexthmm; nexthmm = nexthmm->next) {
                                    if ((hmm_frame(&nexthmm->hmm) < cf)
                                        || (hmm_in_score(&nexthmm->hmm)
                                            < newscore)) {
                                        hmm_enter(&nexthmm->hmm,
                                                  newscore,
                                                  hmm_out_history(&hmm->hmm),
                                                  nf);
                                    }
                                }
                            }
                            /* Enter single word-internal phone. */
                            else {
                                if ((hmm_frame(&nexthmm->hmm) < cf)
                                    || (hmm_in_score(&nexthmm->hmm)
                                        < newscore)) {
                                    hmm_enter(&nexthmm->hmm, newscore,
                                              hmm_out_history(&hmm->hmm), nf);
                                }
                            }
                        }
                    }
                    /* Right-context phones - apply word beam and exit. */
                    else {
                        if (newscore > wordthresh) {
                            save_bwd_ptr(w, newscore,
                                         hmm_out_history(&hmm->hmm),
                                         hmm->info.rc_id);
                        }
                    }
                }
                /* Zero out inactive HMMs. */
                else if (hmm_frame(&hmm->hmm) != nf) {
                    hmm_clear_scores(&hmm->hmm);
                }
            }
        }
    }
}

void
fwdflat_word_transition(void)
{
    int32 cf, nf, b, thresh, pip, i, w, newscore;
    int32 best_silrc_score = 0, best_silrc_bp = 0;      /* FIXME: good defaults? */
    BPTBL_T *bp;
    dict_entry_t *de, *newde;
    int32 *rcpermtab, *rcss;
    root_chan_t *rhmm;
    int32 *awl;
    float32 lwf;

    cf = CurrentFrame;
    nf = cf + 1;
    thresh = BestScore + FwdflatLogBeamWidth;
    pip = logPhoneInsertionPenalty;
    best_silrc_score = WORST_SCORE;
    lwf = fwdflat_fwdtree_lw_ratio;

    /* Search for all words starting within a window of this frame.
     * These are the successors for words exiting now. */
    get_expand_wordlist(cf, max_sf_win);

    /* Scan words exited in current frame */
    for (b = BPTableIdx[cf]; b < BPIdx; b++) {
        bp = BPTable + b;
        WordLatIdx[bp->wid] = NO_BP;

        if (bp->wid == FinishWordId)
            continue;

        de = g_word_dict->dict_list[bp->wid];
        rcpermtab =
            (bp->r_diph >=
             0) ? g_word_dict->rcFwdPermTable[bp->r_diph] : zeroPermTab;
        rcss = BScoreStack + bp->s_idx;

        /* Transition to all successor words. */
        for (i = 0; expand_word_list[i] >= 0; i++) {
            int32 n_used;
            w = expand_word_list[i];
            newde = g_word_dict->dict_list[w];
            /* Get the exit score we recorded in save_bwd_ptr(), or
             * something approximating it. */
            newscore = rcss[rcpermtab[newde->ci_phone_ids[0]]];
            newscore += lwf
                * ngram_tg_score(g_lmset, newde->wid, bp->real_wid,
                                 bp->prev_real_wid, &n_used);
            newscore += pip;

            /* Enter the next word */
            if (newscore > thresh) {
                rhmm = (root_chan_t *) word_chan[w];
                if ((hmm_frame(&rhmm->hmm) < cf)
                    || (hmm_in_score(&rhmm->hmm) < newscore)) {
                    hmm_enter(&rhmm->hmm, newscore, b, nf);
                    if (hmm_is_mpx(&rhmm->hmm)) {
                        rhmm->hmm.s.mpx_ssid[0] =
                            g_word_dict->lcFwdTable[rhmm->diphone]
                            [de->ci_phone_ids[de->len-1]];
                    }

                    word_active[w] = 1;
                }
            }
        }

        /* Get the best exit into silence. */
        if (best_silrc_score < rcss[rcpermtab[SilencePhoneId]]) {
            best_silrc_score = rcss[rcpermtab[SilencePhoneId]];
            best_silrc_bp = b;
        }
    }

    /* Transition to <sil> */
    newscore = best_silrc_score + SilenceWordPenalty + pip;
    if ((newscore > thresh) && (newscore > WORST_SCORE)) {
        w = SilenceWordId;
        rhmm = (root_chan_t *) word_chan[w];
        if ((hmm_frame(&rhmm->hmm) < cf)
            || (hmm_in_score(&rhmm->hmm) < newscore)) {
            hmm_enter(&rhmm->hmm, newscore,
                      best_silrc_bp, nf);
            word_active[w] = 1;
        }
    }
    /* Transition to noise words */
    newscore = best_silrc_score + FillerWordPenalty + pip;
    if ((newscore > thresh) && (newscore > WORST_SCORE)) {
        for (w = SilenceWordId + 1; w < NumWords; w++) {
            rhmm = (root_chan_t *) word_chan[w];
            if ((hmm_frame(&rhmm->hmm) < cf)
                || (hmm_in_score(&rhmm->hmm) < newscore)) {
                hmm_enter(&rhmm->hmm, newscore,
                          best_silrc_bp, nf);
                word_active[w] = 1;
            }
        }
    }

    /* Reset initial channels of words that have become inactive even after word trans. */
    i = n_active_word[cf & 0x1];
    awl = active_word_list[cf & 0x1];
    for (w = *(awl++); i > 0; --i, w = *(awl++)) {
        rhmm = (root_chan_t *) word_chan[w];

        if (hmm_frame(&rhmm->hmm) == cf) {
            hmm_clear_scores(&rhmm->hmm);
        }
    }
}

void
search_fwdflat_finish(void)
{
    destroy_fwdflat_chan();
    destroy_frm_wordlist();
    memset(word_active, 0, NumWords * sizeof(int32));

    BPTableIdx[CurrentFrame] = BPIdx;
    CurrentFrame--;             /* Backup to the last Active Frame */
    LastFrame = CurrentFrame;

    search_postprocess_bptable(fwdflat_fwdtree_lw_ratio, "FWDFLAT");

#if SEARCH_PROFILE
    E_INFO("%8d words recognized (%d/fr)\n",
           BPIdx, (BPIdx + (LastFrame >> 1)) / (LastFrame + 1));
    E_INFO("%8d senones evaluated (%d/fr)\n", n_senone_active_utt,
           (n_senone_active_utt + (LastFrame >> 1)) / (LastFrame + 1));
    E_INFO("%8d channels searched (%d/fr)\n",
           n_fwdflat_chan, n_fwdflat_chan / (LastFrame + 1));
    E_INFO("%8d words searched (%d/fr)\n",
           n_fwdflat_words, n_fwdflat_words / (LastFrame + 1));
    E_INFO("%8d word transitions (%d/fr)\n",
           n_fwdflat_word_transition,
           n_fwdflat_word_transition / (LastFrame + 1));
#endif
}

static void
fwdflat_renormalize_scores(int32 norm)
{
    root_chan_t *rhmm;
    chan_t *hmm;
    int32 i, cf, w, *awl;

    cf = CurrentFrame;

    /* Renormalize individual word channels */
    i = n_active_word[cf & 0x1];
    awl = active_word_list[cf & 0x1];
    for (w = *(awl++); i > 0; --i, w = *(awl++)) {
        rhmm = (root_chan_t *) word_chan[w];
        if (hmm_frame(&rhmm->hmm) == cf) {
            hmm_normalize(&rhmm->hmm, norm);
        }
        for (hmm = rhmm->next; hmm; hmm = hmm->next) {
            if (hmm_frame(&hmm->hmm) == cf) {
                hmm_normalize(&hmm->hmm, norm);
            }
        }
    }

    renormalized = 1;
}

void
get_expand_wordlist(int32 frm, int32 win)
{
    int32 f, sf, ef;
    latnode_t *node;

    if (!cmd_ln_boolean_r(config, "-fwdtree")) {
        n_fwdflat_word_transition += n_expand_words;
        return;
    }

    sf = frm - win;
    if (sf < 0)
        sf = 0;
    ef = frm + win;
    if (ef > MAX_FRAMES)
        ef = MAX_FRAMES;

    memset(expand_word_flag, 0, NumWords);
    n_expand_words = 0;

    for (f = sf; f < ef; f++) {
        for (node = frm_wordlist[f]; node; node = node->next) {
            if (!expand_word_flag[node->wid]) {
                expand_word_list[n_expand_words++] = node->wid;
                expand_word_flag[node->wid] = 1;
            }
        }
    }
    expand_word_list[n_expand_words] = -1;
    n_fwdflat_word_transition += n_expand_words;
}

void
search_fwdflat_init(void)
{
    fwdflat_wordlist = ckd_calloc(NumWords + 1, sizeof(int32));
    expand_word_flag = ckd_calloc(NumWords, 1);
    expand_word_list = ckd_calloc(NumWords + 1, sizeof(int32));

    min_ef_width = cmd_ln_int32_r(config, "-fwdflatefwid");
    max_sf_win = cmd_ln_int32_r(config, "-fwdflatsfwin");
    E_INFO("fwdflat: min_ef_width = %d, max_sf_win = %d\n",
           min_ef_width, max_sf_win);
}

void
search_fwdflat_free(void)
{
    ckd_free(fwdflat_wordlist);
    ckd_free(expand_word_flag);
    ckd_free(expand_word_list);

}

/*
 * Search bptable for word wid and return its BPTable index.
 * Start search from the given frame frm.
 * Return value: BPTable index that matched.  If none, -1.
 */
int32
search_bptbl_wordlist(int32 wid, int32 frm)
{
    int32 b, first;

    first = BPTableIdx[frm];
    for (b = BPIdx - 1; b >= first; --b) {
        if (wid == g_word_dict->dict_list[BPTable[b].wid]->wid)
            return b;
    }
    return -1;
}

int32
search_bptbl_pred(int32 b)
{
    for (b = BPTable[b].bp; ISA_FILLER_WORD(BPTable[b].wid);
         b = BPTable[b].bp);
    return (g_word_dict->dict_list[BPTable[b].wid]->wid);
}


void
search_get_logbeams(int32 * beam, int32 * pbeam, int32 * wbeam)
{
    *beam = LogBeamWidth;
    *pbeam = NewPhoneLogBeamWidth;
    *wbeam = NewWordLogBeamWidth;
}


void
search_set_topsen_score(int32 frm, int32 score)
{
    assert(frm < MAX_FRAMES);
    topsen_score[frm] = score;
}

void
search_set_hyp_wid(search_hyp_t * h)
{
    int32 i, j;

    /* Bug below: if hyp length exceeds 4096, hyp_wid is silently truncated */
    j = 0;
    for (i = 0; (i < 4096) && (h != NULL); i++, h = h->next) {
        if (h->wid >= 0)        /* FSG may have NULL transitions (wid < 0) */
            hyp_wid[j++] = h->wid;
    }

    n_hyp_wid = j;
}
