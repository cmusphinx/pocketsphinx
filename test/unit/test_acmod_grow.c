#include <stdio.h>
#include <string.h>
#include <pocketsphinx.h>

#include <pocketsphinx/logmath.h>

#include "acmod.h"
#include "ptm_mgau.h"
#include "test_macros.h"

static const mfcc_t cmninit[13] = {
	FLOAT2MFCC(41.00),
	FLOAT2MFCC(-5.29),
	FLOAT2MFCC(-0.12),
	FLOAT2MFCC(5.09),
	FLOAT2MFCC(2.48),
	FLOAT2MFCC(-4.07),
	FLOAT2MFCC(-1.37),
	FLOAT2MFCC(-1.78),
	FLOAT2MFCC(-5.08),
	FLOAT2MFCC(-2.05),
	FLOAT2MFCC(-6.45),
	FLOAT2MFCC(-1.42),
	FLOAT2MFCC(1.17)
};

#define NUM_BEST_SEN 270

int
main(int argc, char *argv[])
{
    acmod_t *acmod;
    logmath_t *lmath;
    cmd_ln_t *config;
    FILE *rawfh;
    int16 *buf;
    int16 const *bptr;
    size_t nread, nsamps;
    int nfr;
    int frame_counter;
    int bestsen1[NUM_BEST_SEN];

    (void)argc;
    (void)argv;
    lmath = logmath_init(1.0001, 0, 0);
    config = ps_config_parse_json(
        NULL,
        "compallsen: true,"
        "cmn: live,"
        "tmatfloor: 0.0001,"
        "mixwfloor: 0.001,"
        "varfloor: 0.0001,"
        "mmap: no,"
        "topn: 4,"
        "ds: 1,"
        "samprate: 16000");
    TEST_ASSERT(config);
    cmd_ln_parse_file_r(config, ps_args(), MODELDIR "/en-us/en-us/feat.params", FALSE);

    cmd_ln_set_str_extra_r(config, "mdef", MODELDIR "/en-us/en-us/mdef");
    cmd_ln_set_str_extra_r(config, "mean", MODELDIR "/en-us/en-us/means");
    cmd_ln_set_str_extra_r(config, "var", MODELDIR "/en-us/en-us/variances");
    cmd_ln_set_str_extra_r(config, "tmat", MODELDIR "/en-us/en-us/transition_matrices");
    cmd_ln_set_str_extra_r(config, "sendump", MODELDIR "/en-us/en-us/sendump");
    cmd_ln_set_str_extra_r(config, "mixw", NULL);
    cmd_ln_set_str_extra_r(config, "lda", NULL);
    cmd_ln_set_str_extra_r(config, "senmgau", NULL);

    TEST_ASSERT(acmod = acmod_init(config, lmath, NULL, NULL));
    cmn_live_set(acmod->fcb->cmn_struct, cmninit);

    nsamps = 2048;
    frame_counter = 0;
    buf = ckd_calloc(nsamps, sizeof(*buf));
    TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
    TEST_EQUAL(FALSE, acmod_set_grow(acmod, TRUE));
    TEST_EQUAL(0, acmod_start_utt(acmod));
    printf("Incremental(2048):\n");
    while (!feof(rawfh)) {
        nread = fread(buf, sizeof(*buf), nsamps, rawfh);
        bptr = buf;
        while ((nfr = acmod_process_raw(acmod, &bptr, &nread, FALSE)) > 0 || nread > 0) {
            int16 best_score;
            int frame_idx = -1, best_senid;
            while (acmod->n_feat_frame > 0) {
                acmod_score(acmod, &frame_idx);
                acmod_advance(acmod);
                best_score = acmod_best_score(acmod, &best_senid);
                printf("Frame %d best senone %d score %d\n",
                       frame_idx, best_senid, best_score);
                TEST_EQUAL(frame_counter, frame_idx);
                if (frame_counter < NUM_BEST_SEN)
                    bestsen1[frame_counter] = best_senid;
                ++frame_counter;
                frame_idx = -1;
            }
        }
    }
    /* Match pocketsphinx-0.8 as we do not remove silence anymore */
    TEST_EQUAL(1, acmod_end_utt(acmod));
    nread = 0;
    acmod_process_raw(acmod, NULL, &nread, FALSE);
    {
        int16 best_score;
        int frame_idx = -1, best_senid;
        while (acmod->n_feat_frame > 0) {
            acmod_score(acmod, &frame_idx);
            acmod_advance(acmod);
            best_score = acmod_best_score(acmod, &best_senid);
            printf("Frame %d best senone %d score %d\n",
                   frame_idx, best_senid, best_score);
            if (frame_counter < NUM_BEST_SEN)
                bestsen1[frame_counter] = best_senid;
            TEST_EQUAL(frame_counter, frame_idx);
            ++frame_counter;
            frame_idx = -1;
        }
    }

    printf("Rewound (MFCC):\n");
    TEST_EQUAL(0, acmod_rewind(acmod));
    /* Clear fast history buffers to ensure repeatability */
    ptm_mgau_reset_fast_hist(acmod->mgau);
    {
        int16 best_score;
        int frame_idx = -1, best_senid;
        frame_counter = 0;
        while (acmod->n_feat_frame > 0) {
            acmod_score(acmod, &frame_idx);
            acmod_advance(acmod);
            best_score = acmod_best_score(acmod, &best_senid);
            printf("Frame %d best senone %d score %d\n",
                   frame_idx, best_senid, best_score);
            if (frame_counter < NUM_BEST_SEN)
                TEST_EQUAL(best_senid, bestsen1[frame_counter]);
            TEST_EQUAL(frame_counter, frame_idx);
            ++frame_counter;
            frame_idx = -1;
        }
    }

    /* Clean up, go home. */
    fclose(rawfh);
    ckd_free(buf);
    acmod_free(acmod);
    logmath_free(lmath);
    ps_config_free(config);
    return 0;
}
