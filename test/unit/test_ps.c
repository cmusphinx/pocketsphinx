#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>

#include "pocketsphinx_internal.h"

#include "test_macros.h"

int
ps_decoder_test(cmd_ln_t *config, char const *sname, char const *expected)
{
    ps_decoder_t *ps;
    mfcc_t **cepbuf;
    float32 **floatbuf;
    FILE *rawfh;
    int16 *buf;
    int16 const *bptr;
    size_t nread;
    size_t nsamps;
    int32 nfr, i, score, prob;
    char const *hyp;
    double n_speech, n_cpu, n_wall;
    ps_seg_t *seg;
    char *prev_cmn;

    TEST_ASSERT(ps = ps_init(config));
    prev_cmn = ckd_salloc(ps_get_cmn(ps, FALSE));
    printf("CMN: %s\n", prev_cmn);
    err_set_loglevel(ERR_INFO);
    /* Test it first with pocketsphinx_decode_raw() */
    TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
    ps_decode_raw(ps, rawfh, -1);
    hyp = ps_get_hyp(ps, &score);
    prob = ps_get_prob(ps);
    printf("%s: %s (%d, %d)\n", sname, hyp, score, prob);
    TEST_EQUAL(0, strcmp(hyp, expected));
    TEST_ASSERT(prob <= 0);
    ps_get_utt_time(ps, &n_speech, &n_cpu, &n_wall);
    printf("%.2f seconds speech, %.2f seconds CPU, %.2f seconds wall\n",
           n_speech, n_cpu, n_wall);
    printf("%.2f xRT (CPU), %.2f xRT (elapsed)\n",
           n_cpu / n_speech, n_wall / n_speech);
    TEST_ASSERT(0 != strcmp(prev_cmn, ps_get_cmn(ps, FALSE)));
    ckd_free(prev_cmn);
    prev_cmn = ckd_salloc(ps_get_cmn(ps, FALSE));
    printf("CMN: %s\n", prev_cmn);

    /* Test it with ps_process_raw() */
    clearerr(rawfh);
    fseek(rawfh, 0, SEEK_END);
    nsamps = ftell(rawfh) / sizeof(*buf);
    fseek(rawfh, 0, SEEK_SET);
    ps_start_stream(ps);
    TEST_EQUAL(0, ps_start_utt(ps));
    nsamps = 2048;
    buf = ckd_calloc(nsamps, sizeof(*buf));
    i = 0;
    while (!feof(rawfh)) {
        nread = fread(buf, sizeof(*buf), nsamps, rawfh);
        ps_process_raw(ps, buf, nread, FALSE, FALSE);
        if (i++ == 1) {
            /* Test updating CMN while decoding */
            TEST_ASSERT(0 != strcmp(prev_cmn, ps_get_cmn(ps, TRUE)));
            ckd_free(prev_cmn);
            prev_cmn = ckd_salloc(ps_get_cmn(ps, FALSE));
            printf("CMN: %s\n", prev_cmn);
        }
    }
    TEST_EQUAL(0, ps_end_utt(ps));
    hyp = ps_get_hyp(ps, &score);
    prob = ps_get_prob(ps);
    printf("%s: %s (%d, %d)\n", sname, hyp, score, prob);
    TEST_EQUAL(0, strcmp(hyp, expected));
    ps_get_utt_time(ps, &n_speech, &n_cpu, &n_wall);
    printf("%.2f seconds speech, %.2f seconds CPU, %.2f seconds wall\n",
           n_speech, n_cpu, n_wall);
    printf("%.2f xRT (CPU), %.2f xRT (elapsed)\n",
           n_cpu / n_speech, n_wall / n_speech);

    /* Now read the whole file and produce an MFCC buffer. */
    ps_start_stream(ps);
    clearerr(rawfh);
    fseek(rawfh, 0, SEEK_END);
    nsamps = ftell(rawfh) / sizeof(*buf);
    fseek(rawfh, 0, SEEK_SET);
    bptr = buf = ckd_realloc(buf, nsamps * sizeof(*buf));
    TEST_EQUAL(nsamps, fread(buf, sizeof(*buf), nsamps, rawfh));
    fe_process_frames(ps->acmod->fe, &bptr, &nsamps, NULL, &nfr);
    cepbuf = ckd_calloc_2d(nfr + 1,
                   fe_get_output_size(ps->acmod->fe),
                   sizeof(**cepbuf));
    fe_start_utt(ps->acmod->fe);
    fe_process_frames(ps->acmod->fe, &bptr, &nsamps, cepbuf, &nfr);
    fe_end_utt(ps->acmod->fe, cepbuf[nfr], &i);

    /* Decode it with process_cep() */
#ifdef FIXED_POINT
    floatbuf = ckd_calloc_2d(nfr + 1,
                             fe_get_output_size(ps->acmod->fe),
                             sizeof(**floatbuf));
    fe_mfcc_to_float(ps->acmod->fe, cepbuf, floatbuf, nfr + 1);
#else
    floatbuf = cepbuf;
#endif

    TEST_EQUAL(0, ps_start_utt(ps));
    for (i = 0; i < nfr; ++i) {
        ps_process_cep(ps, floatbuf + i, 1, FALSE, FALSE);
        if (i == nfr - 5) {
            /* Test updating CMN while decoding */
            TEST_ASSERT(0 != strcmp(prev_cmn, ps_get_cmn(ps, TRUE)));
            ckd_free(prev_cmn);
            prev_cmn = ckd_salloc(ps_get_cmn(ps, FALSE));
            printf("CMN: %s\n", prev_cmn);
        }
    }
    TEST_EQUAL(0, ps_end_utt(ps));
    hyp = ps_get_hyp(ps, &score);
    prob = ps_get_prob(ps);
    printf("%s: %s (%d, %d)\n", sname, hyp, score, prob);
    TEST_EQUAL(0, strcmp(hyp, expected));
    TEST_ASSERT(prob <= 0);
    for (seg = ps_seg_iter(ps); seg;
         seg = ps_seg_next(seg)) {
        char const *word;
        int sf, ef;
        int32 post, lscr, ascr, lback;

        word = ps_seg_word(seg);
        ps_seg_frames(seg, &sf, &ef);
        post = ps_seg_prob(seg, &ascr, &lscr, &lback);
        printf("%s (%d:%d) P(w|o) = %f ascr = %d lscr = %d lback = %d post=%d\n", word, sf, ef,
               logmath_exp(ps_get_logmath(ps), post), ascr, lscr, lback, post);
    }

    ps_get_utt_time(ps, &n_speech, &n_cpu, &n_wall);
    printf("%.2f seconds speech, %.2f seconds CPU, %.2f seconds wall\n",
           n_speech, n_cpu, n_wall);
    printf("%.2f xRT (CPU), %.2f xRT (elapsed)\n",
           n_cpu / n_speech, n_wall / n_speech);
    ps_get_all_time(ps, &n_speech, &n_cpu, &n_wall);
    printf("TOTAL: %.2f seconds speech, %.2f seconds CPU, %.2f seconds wall\n",
           n_speech, n_cpu, n_wall);
    printf("TOTAL: %.2f xRT (CPU), %.2f xRT (elapsed)\n",
           n_cpu / n_speech, n_wall / n_speech);
    TEST_ASSERT(0 != strcmp(prev_cmn, ps_get_cmn(ps, FALSE)));
    ckd_free(prev_cmn);
    prev_cmn = ckd_salloc(ps_get_cmn(ps, FALSE));
    printf("CMN: %s\n", prev_cmn);

    fclose(rawfh);
    ps_free(ps);
    ps_config_free(config);
#ifdef FIXED_POINT
    ckd_free_2d(floatbuf);
#endif
    ckd_free_2d(cepbuf);
    ckd_free(buf);
    ckd_free(prev_cmn);

    return 0;
}
