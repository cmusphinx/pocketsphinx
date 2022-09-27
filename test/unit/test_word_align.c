/* -*- c-basic-offset: 4 -*- */
#include <pocketsphinx.h>

#include "test_macros.h"
#include "pocketsphinx_internal.h"

//#define AUSTEN_TEXT "and mister john dashwood had then leisure to consider how much there might be prudently in his power to do for them"
#define AUSTEN_TEXT "he was not an ill disposed young man"
static int
do_decode(ps_decoder_t *ps)
{
    FILE *rawfh;
    const char *hyp;
    long nsamp;
    int score;
    
    TEST_ASSERT(rawfh = fopen(DATADIR "/librivox/sense_and_sensibility_01_austen_64kb-0880.wav", "rb"));
    fseek(rawfh, 44, SEEK_SET);
    nsamp = ps_decode_raw(ps, rawfh, -1);
    hyp = ps_get_hyp(ps, &score);
    printf("%s (%ld samples, %d score)\n", hyp, nsamp, score);
    TEST_ASSERT(nsamp > 0);
    TEST_ASSERT((0 == strcmp(hyp, AUSTEN_TEXT)
                 || 0 == strcmp(hyp, "<sil> " AUSTEN_TEXT " <sil>")));
    fclose(rawfh);

    return 0;
}

int
main(int argc, char *argv[])
{
    ps_decoder_t *ps;
    ps_alignment_t *al;
    ps_alignment_iter_t *itor;
    ps_seg_t *seg;
    ps_config_t *config;
    int i, sf, ef, last_ef;
    int *sfs, *efs;

    (void)argc;
    (void)argv;
    err_set_loglevel(ERR_INFO);
    TEST_ASSERT(config =
                ps_config_parse_json(
                    NULL,
                    "loglevel: INFO, bestpath: false,"
                    "hmm: \"" MODELDIR "/en-us/en-us\","
                    "dict: \"" MODELDIR "/en-us/cmudict-en-us.dict\","
                    "samprate: 16000"));
    TEST_ASSERT(ps = ps_init(config));
    /* Test alignment through the decoder/search API */
    TEST_EQUAL(0, ps_set_align_text(ps, AUSTEN_TEXT));
    do_decode(ps);
    TEST_EQUAL(0, strcmp(ps_get_hyp(ps, &i), AUSTEN_TEXT));
    printf("Word alignment:\n");
    i = 0; last_ef = -1;
    for (seg = ps_seg_iter(ps); seg; seg = ps_seg_next(seg)) {
        ps_seg_frames(seg, &sf, &ef);
        printf("%s %d %d\n", ps_seg_word(seg), sf, ef);
        TEST_ASSERT(sf == last_ef + 1);
        TEST_ASSERT(ef > sf);
        last_ef = ef;
        i++;
    }
    TEST_EQUAL(NULL, seg);

    /* Save start and end points for comparison */
    sfs = ckd_calloc(i, sizeof(*sfs));
    efs = ckd_calloc(i, sizeof(*efs));
    i = 0;
    for (seg = ps_seg_iter(ps); seg; seg = ps_seg_next(seg)) {
        ps_seg_frames(seg, &sfs[i], &efs[i]);
        i++;
    }

    /* Test second pass alignment.  Ensure that alignment and seg give
     * the same results and that phones have constraints propagated to
     * them. */
    printf("Converted to subword alignment constraints:\n");
    TEST_EQUAL(0, ps_set_alignment(ps, NULL));
    TEST_ASSERT(al = ps_get_alignment(ps));
    for (i = 0, seg = ps_seg_iter(ps), itor = ps_alignment_words(al); itor;
	 i++, seg = ps_seg_next(seg), itor = ps_alignment_iter_next(itor)) {
        int score, start, duration;
        ps_alignment_iter_t *pitor;

        ps_seg_frames(seg, &sf, &ef);
        TEST_ASSERT(seg);
	score = ps_alignment_iter_seg(itor, &start, &duration);
        printf("%s %d %d %s %d %d\n", ps_seg_word(seg), sf, ef,
               ps_alignment_iter_name(itor), start, duration);
        TEST_EQUAL(0, strcmp(ps_seg_word(seg), ps_alignment_iter_name(itor)));
        TEST_EQUAL(sf, sfs[i]);
        TEST_EQUAL(ef, efs[i]);
        TEST_EQUAL(0, score);
        TEST_EQUAL(start, sf);
        TEST_EQUAL(duration, ef - sf + 1);
        /* Durations are propagated down from words, each phone will
         * have the same duration as its parent, and these are used as
         * constraints to alignment. */
        for (pitor = ps_alignment_iter_children(itor); pitor;
             pitor = ps_alignment_iter_next(pitor)) {
            score = ps_alignment_iter_seg(pitor, &start, &duration);
            TEST_EQUAL(0, score);
            TEST_EQUAL(start, sf);
            TEST_EQUAL(duration, ef - sf + 1);
        }
    }

    do_decode(ps);
    TEST_ASSERT(al = ps_get_alignment(ps));
    printf("Subword alignment:\n");
    /* It should have durations assigned (and properly constrained). */
    for (i = 0, seg = ps_seg_iter(ps), itor = ps_alignment_words(al); itor;
	 i++, seg = ps_seg_next(seg), itor = ps_alignment_iter_next(itor)) {
        int score, start, duration;
        ps_alignment_iter_t *pitor;

        ps_seg_frames(seg, &sf, &ef);
        TEST_ASSERT(seg);
	score = ps_alignment_iter_seg(itor, &start, &duration);
        printf("%s %d %d %d %d %s %d %d %d\n", ps_seg_word(seg),
               sfs[i], efs[i], sf, ef,
               ps_alignment_iter_name(itor), start, duration, score);
        TEST_EQUAL(sf, sfs[i]);
        TEST_EQUAL(ef, efs[i]);
        TEST_ASSERT(score != 0);
        TEST_EQUAL(start, sf);
        TEST_EQUAL(duration, ef - sf + 1);

        /* Phone segmentations should be constrained by words */
        pitor = ps_alignment_iter_children(itor);
        score = ps_alignment_iter_seg(pitor, &start, &duration);
        /* First phone should be aligned with word */
        TEST_EQUAL(start, sf);
        while (pitor) {
            ps_alignment_iter_t *sitor;
            int state_start, state_duration;
            score = ps_alignment_iter_seg(pitor, &start, &duration);
            printf("%s %d %d %s %d %d %d\n", ps_seg_word(seg), sf, ef,
                   ps_alignment_iter_name(pitor), start, duration, score);
            /* State segmentations should be constrained by phones */
            sitor = ps_alignment_iter_children(pitor);
            score = ps_alignment_iter_seg(sitor, &state_start, &state_duration);
            /* First state should be aligned with phone */
            TEST_EQUAL(state_start, start);
            while (sitor) {
                score = ps_alignment_iter_seg(sitor, &state_start, &state_duration);
                printf("%s %d %d %s %d %d %d\n", ps_seg_word(seg), sf, ef,
                       ps_alignment_iter_name(sitor), state_start, state_duration,
                       score);
                sitor = ps_alignment_iter_next(sitor);
            }
            /* Last state should fill phone duration */
            TEST_EQUAL(state_start + state_duration, start + duration);
            pitor = ps_alignment_iter_next(pitor);
        }
        /* Last phone should fill word duration */
        TEST_EQUAL(start + duration - 1, ef);
    }

    /* Segmentations should all be contiguous */
    last_ef = 0;
    for (itor = ps_alignment_words(al); itor;
         itor = ps_alignment_iter_next(itor)) {
        int start, duration;
        (void)ps_alignment_iter_seg(itor, &start, &duration);
        TEST_EQUAL(start, last_ef);
        last_ef = start + duration;
    }
    last_ef = 0;
    for (itor = ps_alignment_phones(al); itor;
         itor = ps_alignment_iter_next(itor)) {
        int start, duration;
        (void)ps_alignment_iter_seg(itor, &start, &duration);
        TEST_EQUAL(start, last_ef);
        last_ef = start + duration;
    }
    last_ef = 0;
    for (itor = ps_alignment_states(al); itor;
         itor = ps_alignment_iter_next(itor)) {
        int start, duration;
        (void)ps_alignment_iter_seg(itor, &start, &duration);
        TEST_EQUAL(start, last_ef);
        last_ef = start + duration;
    }
    
    ckd_free(sfs);
    ckd_free(efs);
    ps_free(ps);
    ps_config_free(config);

    return 0;
}
