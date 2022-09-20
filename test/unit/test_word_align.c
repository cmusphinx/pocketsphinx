/* -*- c-basic-offset: 4 -*- */
#include <pocketsphinx.h>

#include "test_macros.h"

static int
do_decode(ps_decoder_t *ps)
{
    FILE *rawfh;
    const char *hyp;
    long nsamp;
    int score;
    
    TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
    nsamp = ps_decode_raw(ps, rawfh, -1);
    hyp = ps_get_hyp(ps, &score);
    printf("%s (%ld samples, %d score)\n", hyp, nsamp, score);
    TEST_ASSERT(nsamp > 0);
    TEST_EQUAL(0, strcmp(hyp, "go forward ten meters"));
    fclose(rawfh);

    return 0;
}

int
main(int argc, char *argv[])
{
    ps_decoder_t *ps;
    ps_seg_t *seg;
    ps_config_t *config;
    int i, sf, ef, last_ef;

    (void)argc;
    (void)argv;
    TEST_ASSERT(config =
                ps_config_parse_json(
                    NULL,
                    "hmm: \"" MODELDIR "/en-us/en-us\","
                    "dict: \"" MODELDIR "/en-us/cmudict-en-us.dict\","
                    "samprate: 16000"));
    TEST_ASSERT(ps = ps_init(config));
    /* Test alignment through the decoder/search API */
    TEST_EQUAL(0, ps_set_align_text(ps, "go forward ten meters"));
    do_decode(ps);
    TEST_EQUAL(0, strcmp(ps_get_hyp(ps, &i), "go forward ten meters"));
    seg = ps_seg_iter(ps);
    ps_seg_frames(seg, &sf, &ef);
    printf("%s %d %d\n", ps_seg_word(seg), sf, ef);
    TEST_EQUAL(0, strcmp("<sil>", ps_seg_word(seg)));
    TEST_ASSERT(ef > sf);
    last_ef = ef;
    seg = ps_seg_next(seg);
    ps_seg_frames(seg, &sf, &ef);
    printf("%s %d %d\n", ps_seg_word(seg), sf, ef);
    TEST_EQUAL(0, strcmp("go", ps_seg_word(seg)));
    TEST_ASSERT(sf > last_ef);
    TEST_ASSERT(ef > sf);
    last_ef = ef;
    seg = ps_seg_next(seg);
    ps_seg_frames(seg, &sf, &ef);
    printf("%s %d %d\n", ps_seg_word(seg), sf, ef);
    TEST_EQUAL(0, strcmp("forward", ps_seg_word(seg)));
    TEST_ASSERT(sf > last_ef);
    TEST_ASSERT(ef > sf);
    last_ef = ef;
    seg = ps_seg_next(seg);
    ps_seg_frames(seg, &sf, &ef);
    printf("%s %d %d\n", ps_seg_word(seg), sf, ef);
    TEST_EQUAL(0, strcmp("ten", ps_seg_word(seg)));
    TEST_ASSERT(sf > last_ef);
    TEST_ASSERT(ef > sf);
    last_ef = ef;
    seg = ps_seg_next(seg);
    ps_seg_frames(seg, &sf, &ef);
    printf("%s %d %d\n", ps_seg_word(seg), sf, ef);
    TEST_EQUAL(0, strcmp("meters", ps_seg_word(seg)));
    TEST_ASSERT(sf > last_ef);
    TEST_ASSERT(ef > sf);
    last_ef = ef;
    seg = ps_seg_next(seg);
    ps_seg_frames(seg, &sf, &ef);
    printf("%s %d %d\n", ps_seg_word(seg), sf, ef);
    TEST_EQUAL(0, strcmp("<sil>", ps_seg_word(seg)));
    TEST_ASSERT(sf > last_ef);
    TEST_ASSERT(ef > sf);
    last_ef = ef;
    seg = ps_seg_next(seg);
    TEST_EQUAL(NULL, seg);
    ps_free(ps);
    ps_config_free(config);

    return 0;
}
