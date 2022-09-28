#include <stdio.h>
#include <string.h>
#include <pocketsphinx.h>

#include <pocketsphinx/logmath.h>
#include <pocketsphinx/err.h>

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

static void
assert_cmninit(cmn_t *cmn)
{
    int i;
    for (i = 0; i < cmn->veclen; ++i) {
        TEST_EQUAL_MFCC(cmn->cmn_mean[i], cmninit[i]);
        TEST_EQUAL_MFCC(cmn->sum[i], cmninit[i] * CMN_WIN);
    }
}

int
main(int argc, char *argv[])
{
    acmod_t *acmod;
    logmath_t *lmath;
    cmd_ln_t *config;
    FILE *rawfh;
    int16 *buf;
    int16 const *bptr;
    mfcc_t **cepbuf, **cptr;
    size_t nread, nsamps;
    int nfr;
    int frame_counter;
    int bestsen1[270];

    (void)argc;
    (void)argv;
    err_set_loglevel(ERR_INFO);
    lmath = logmath_init(1.0001, 0, 0);
    config = ps_config_parse_json(
        NULL,
        "compallsen: true, cmn: live, tmatfloor: 0.0001,"
        "mixwfloor: 0.001, varfloor: 0.0001,"
        "mmap: false, topn: 4, ds: 1, samprate: 16000");
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

    /* Ensure that -cmninit does what it should */
    ps_config_set_str(config, "cmninit",
                     "41.00,-5.29,-0.12,5.09,2.48,-4.07,-1.37,-1.78,-5.08,-2.05,-6.45,-1.42,1.17");
    TEST_ASSERT(acmod = acmod_init(config, lmath, NULL, NULL));
    assert_cmninit(acmod->fcb->cmn_struct);
    /* Ensure that the two different ways of setting CMN do the right thing. */
    cmn_live_set(acmod->fcb->cmn_struct, cmninit);
    assert_cmninit(acmod->fcb->cmn_struct);
    cmn_set_repr(acmod->fcb->cmn_struct, 
                 "41.00,-5.29,-0.12,5.09,2.48,-4.07,-1.37,-1.78,-5.08,-2.05,-6.45,-1.42,1.17");
    assert_cmninit(acmod->fcb->cmn_struct);

    nsamps = 2048;
    frame_counter = 0;
    buf = ckd_calloc(nsamps, sizeof(*buf));
    TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
    TEST_EQUAL(0, acmod_start_utt(acmod));
    E_INFO("Incremental(2048):\n");
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
                E_INFO("Frame %d best senone %d score %d\n",
                       frame_idx, best_senid, best_score);
                TEST_EQUAL(frame_counter, frame_idx);
                if (frame_counter < 190)
                    bestsen1[frame_counter] = best_score;
                ++frame_counter;
                frame_idx = -1;
            }
        }
    }
    TEST_EQUAL(0, acmod_end_utt(acmod));
    /* Make sure -cmninit was updated. */
    TEST_ASSERT(ps_config_str(config, "cmninit") != NULL);
    E_INFO("New -cmninit: %s\n", ps_config_str(config, "cmninit"));
    nread = 0;
    {
        int16 best_score;
        int frame_idx = -1, best_senid;
        while (acmod->n_feat_frame > 0) {
            acmod_score(acmod, &frame_idx);
            acmod_advance(acmod);
            best_score = acmod_best_score(acmod, &best_senid);
            E_INFO("Frame %d best senone %d score %d\n",
                   frame_idx, best_senid, best_score);
            if (frame_counter < 190)
                bestsen1[frame_counter] = best_score;
            TEST_EQUAL(frame_counter, frame_idx);
            ++frame_counter;
            frame_idx = -1;
        }
    }

    /* Now try to process the whole thing at once. */
    E_INFO("Whole utterance:\n");
    cmn_live_set(acmod->fcb->cmn_struct, cmninit);
    nsamps = ftell(rawfh) / sizeof(*buf);
    clearerr(rawfh);
    fseek(rawfh, 0, SEEK_SET);
    buf = ckd_realloc(buf, nsamps * sizeof(*buf));
    TEST_EQUAL(nsamps, fread(buf, sizeof(*buf), nsamps, rawfh));
    bptr = buf;
    TEST_EQUAL(0, acmod_start_utt(acmod));
    acmod_process_raw(acmod, &bptr, &nsamps, TRUE);
    TEST_EQUAL(0, acmod_end_utt(acmod));
    /* Make sure -cmninit was updated. */
    TEST_ASSERT(ps_config_str(config, "cmninit") != NULL);
    E_INFO("New -cmninit: %s\n", ps_config_str(config, "cmninit"));
    {
        int16 best_score;
        int frame_idx = -1, best_senid;
        frame_counter = 0;
        while (acmod->n_feat_frame > 0) {
            acmod_score(acmod, &frame_idx);
            acmod_advance(acmod);
            best_score = acmod_best_score(acmod, &best_senid);
            E_INFO("Frame %d best senone %d score %d\n",
               frame_idx, best_senid, best_score);
            if (frame_counter < 190)
                TEST_EQUAL_LOG(best_score, bestsen1[frame_counter]);
            TEST_EQUAL(frame_counter, frame_idx);
            ++frame_counter;
            frame_idx = -1;
        }
    }

    /* Now process MFCCs and make sure we get the same results. */
    cepbuf = ckd_calloc_2d(frame_counter,
                   fe_get_output_size(acmod->fe),
                   sizeof(**cepbuf));
    fe_start_utt(acmod->fe);
    nsamps = ftell(rawfh) / sizeof(*buf);
    bptr = buf;
    nfr = frame_counter;
    fe_process_frames(acmod->fe, &bptr, &nsamps, cepbuf, &nfr);
    fe_end_utt(acmod->fe, cepbuf[frame_counter-1], &nfr);

    E_INFO("Incremental(MFCC):\n");
    cmn_live_set(acmod->fcb->cmn_struct, cmninit);
    TEST_EQUAL(0, acmod_start_utt(acmod));
    cptr = cepbuf;
    nfr = frame_counter;
    frame_counter = 0;
    while ((acmod_process_cep(acmod, &cptr, &nfr, FALSE)) > 0) {
        int16 best_score;
        int frame_idx = -1, best_senid;
        while (acmod->n_feat_frame > 0) {
            acmod_score(acmod, &frame_idx);
            acmod_advance(acmod);
            best_score = acmod_best_score(acmod, &best_senid);
            E_INFO("Frame %d best senone %d score %d\n",
                   frame_idx, best_senid, best_score);
            TEST_EQUAL(frame_counter, frame_idx);
            if (frame_counter < 190)
                TEST_EQUAL_LOG(best_score, bestsen1[frame_counter]);
            ++frame_counter;
            frame_idx = -1;
        }
    }
    TEST_EQUAL(0, acmod_end_utt(acmod));
    /* Make sure -cmninit was updated. */
    TEST_ASSERT(ps_config_str(config, "cmninit") != NULL);
    E_INFO("New -cmninit: %s\n", ps_config_str(config, "cmninit"));
    nfr = 0;
    acmod_process_cep(acmod, &cptr, &nfr, FALSE);
    {
        int16 best_score;
        int frame_idx = -1, best_senid;
        while (acmod->n_feat_frame > 0) {
            acmod_score(acmod, &frame_idx);
            acmod_advance(acmod);
            best_score = acmod_best_score(acmod, &best_senid);
            E_INFO("Frame %d best senone %d score %d\n",
                   frame_idx, best_senid, best_score);
            TEST_EQUAL(frame_counter, frame_idx);
            if (frame_counter < 190)
                TEST_EQUAL_LOG(best_score, bestsen1[frame_counter]);
            ++frame_counter;
            frame_idx = -1;
        }
    }

    /* Note that we have to process the whole thing again because
     * !#@$@ s2mfc2feat modifies its argument (not for long) */
    fe_start_utt(acmod->fe);
    nsamps = ftell(rawfh) / sizeof(*buf);
    bptr = buf;
    nfr = frame_counter;
    fe_process_frames(acmod->fe, &bptr, &nsamps, cepbuf, &nfr);
    fe_end_utt(acmod->fe, cepbuf[frame_counter-1], &nfr);

    E_INFO("Whole utterance (MFCC):\n");
    cmn_live_set(acmod->fcb->cmn_struct, cmninit);
    TEST_EQUAL(0, acmod_start_utt(acmod));
    cptr = cepbuf;
    nfr = frame_counter;
    acmod_process_cep(acmod, &cptr, &nfr, TRUE);
    TEST_EQUAL(0, acmod_end_utt(acmod));
    /* Make sure -cmninit was updated. */
    TEST_ASSERT(ps_config_str(config, "cmninit") != NULL);
    E_INFO("New -cmninit: %s\n", ps_config_str(config, "cmninit"));
    {
        int16 best_score;
        int frame_idx = -1, best_senid;
        frame_counter = 0;
        while (acmod->n_feat_frame > 0) {
            acmod_score(acmod, &frame_idx);
            acmod_advance(acmod);
            best_score = acmod_best_score(acmod, &best_senid);
            E_INFO("Frame %d best senone %d score %d\n",
                   frame_idx, best_senid, best_score);
            if (frame_counter < 190)
                TEST_EQUAL_LOG(best_score, bestsen1[frame_counter]);
            TEST_EQUAL(frame_counter, frame_idx);
            ++frame_counter;
            frame_idx = -1;
        }
    }

    E_INFO("Rewound (MFCC):\n");
    TEST_EQUAL(0, acmod_rewind(acmod));
    {
        int16 best_score;
        int frame_idx = -1, best_senid;
        frame_counter = 0;
        while (acmod->n_feat_frame > 0) {
            acmod_score(acmod, &frame_idx);
            acmod_advance(acmod);
            best_score = acmod_best_score(acmod, &best_senid);
            E_INFO("Frame %d best senone %d score %d\n",
                   frame_idx, best_senid, best_score);
            if (frame_counter < 190)
                TEST_EQUAL_LOG(best_score, bestsen1[frame_counter]);
            TEST_EQUAL(frame_counter, frame_idx);
            ++frame_counter;
            frame_idx = -1;
        }
    }

    /* Clean up, go home. */
    ckd_free_2d(cepbuf);
    fclose(rawfh);
    ckd_free(buf);
    acmod_free(acmod);
    logmath_free(lmath);
    ps_config_free(config);
    return 0;
}
