#include <stdio.h>
#include <string.h>
#include <pocketsphinx.h>

#include <sphinxbase/logmath.h>

#include "acmod.h"
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
    int bestsen1[270];

    lmath = logmath_init(1.0001, 0, 0);
    config = cmd_ln_init(NULL, ps_args(), TRUE,
                 "-compallsen", "true",
                 "-cmn", "live",
                 "-tmatfloor", "0.0001",
                 "-mixwfloor", "0.001",
                 "-varfloor", "0.0001",
                 "-mmap", "no",
                 "-topn", "4",
                 "-ds", "1",
                 "-samprate", "16000", NULL);
    TEST_ASSERT(config);
    cmd_ln_parse_file_r(config, ps_args(), MODELDIR "/en-us/en-us/feat.params", FALSE);

    cmd_ln_set_str_extra_r(config, "_mdef", MODELDIR "/en-us/en-us/mdef");
    cmd_ln_set_str_extra_r(config, "_mean", MODELDIR "/en-us/en-us/means");
    cmd_ln_set_str_extra_r(config, "_var", MODELDIR "/en-us/en-us/variances");
    cmd_ln_set_str_extra_r(config, "_tmat", MODELDIR "/en-us/en-us/transition_matrices");
    cmd_ln_set_str_extra_r(config, "_sendump", MODELDIR "/en-us/en-us/sendump");
    cmd_ln_set_str_extra_r(config, "_mixw", NULL);
    cmd_ln_set_str_extra_r(config, "_lda", NULL);
    cmd_ln_set_str_extra_r(config, "_senmgau", NULL);

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
                if (frame_counter < 190)
                    bestsen1[frame_counter] = best_senid;
                ++frame_counter;
                frame_idx = -1;
            }
        }
    }
    TEST_EQUAL(0, acmod_end_utt(acmod));
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
            if (frame_counter < 190)
                bestsen1[frame_counter] = best_senid;
            TEST_EQUAL(frame_counter, frame_idx);
            ++frame_counter;
            frame_idx = -1;
        }
    }

    printf("Rewound (MFCC):\n");
    TEST_EQUAL(0, acmod_rewind(acmod));
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
            if (frame_counter < 190)
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
    cmd_ln_free_r(config);
    return 0;
}
