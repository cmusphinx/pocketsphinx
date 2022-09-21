/* -*- c-basic-offset: 4 -*- */
#include <pocketsphinx.h>

#include "test_macros.h"
#include "pocketsphinx_internal.h"

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
    TEST_ASSERT((0 == strcmp(hyp, "go forward ten meters")
                 || 0 == strcmp(hyp, "<sil> go forward ten meters <sil>")));
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

    (void)argc;
    (void)argv;
    err_set_loglevel(ERR_INFO);
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

    /* Test second pass alignment */
    TEST_EQUAL(0, ps_set_alignment(ps, NULL));
    TEST_ASSERT(al = ps_get_alignment(ps));
    /* It should have no durations assigned. */
    for (itor = ps_alignment_phones(al); itor;
	 itor = ps_alignment_iter_next(itor)) {
	ps_alignment_entry_t *ent = ps_alignment_iter_get(itor);
        TEST_EQUAL(0, ent->start);
        TEST_EQUAL(0, ent->duration);
    }
    do_decode(ps);
    TEST_ASSERT(al = ps_get_alignment(ps));
    /* It should have durations assigned. */
    for (itor = ps_alignment_phones(al); itor;
	 itor = ps_alignment_iter_next(itor)) {
	ps_alignment_entry_t *ent = ps_alignment_iter_get(itor);

	printf("%s %d %d\n",
	       bin_mdef_ciphone_str(ps->acmod->mdef, ent->id.pid.cipid),
	       ent->start, ent->duration);
        TEST_ASSERT(0 != ent->duration);
    }
    
    ps_free(ps);
    ps_config_free(config);

    return 0;
}
