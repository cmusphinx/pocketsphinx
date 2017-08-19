#include <pocketsphinx.h>

#include "ps_alignment.h"
#include "state_align_search.h"
#include "pocketsphinx_internal.h"

#include "test_macros.h"

static int
do_search(ps_search_t *search, acmod_t *acmod)
{
    FILE *rawfh;
    int16 buf[2048];
    size_t nread;
    int16 const *bptr;
    int nfr;

    TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
    TEST_EQUAL(0, acmod_start_utt(acmod));
    ps_search_start(search);
    while (!feof(rawfh)) {
        nread = fread(buf, sizeof(*buf), 2048, rawfh);
        bptr = buf;
        while ((nfr = acmod_process_raw(acmod, &bptr, &nread, FALSE)) > 0) {
            while (acmod->n_feat_frame > 0) {
                ps_search_step(search, acmod->output_frame);
                acmod_advance(acmod);
            }
        }
    }
    TEST_ASSERT(acmod_end_utt(acmod) >= 0);
    fclose(rawfh);
    return ps_search_finish(search);
}


int
main(int argc, char *argv[])
{
    ps_decoder_t *ps;
    dict_t *dict;
    dict2pid_t *d2p;
    acmod_t *acmod;
    ps_alignment_t *al;
    ps_alignment_iter_t *itor;
    ps_search_t *search;
    cmd_ln_t *config;
    int i;

    /* Test diphones synthetic AM */

    config = cmd_ln_init(NULL, ps_args(), FALSE,
                 "-hmm", MODELDIR "/en-us/en-us",
                 "-dict", MODELDIR "/en-us/cmudict-en-us.dict",
                 "-samprate", "16000",
                 "-diphones", "synthetic", NULL);
    TEST_ASSERT(ps = ps_init(config));
    dict = ps->dict;
    d2p = ps->d2p;
    acmod = ps->acmod;

    al = ps_alignment_init(d2p);
    TEST_EQUAL(1, ps_alignment_add_word(al, dict_wordid(dict, "go"), 0));
    TEST_EQUAL(2, ps_alignment_add_word(al, dict_wordid(dict, "forward"), 0));
    TEST_EQUAL(3, ps_alignment_add_word(al, dict_wordid(dict, "ten"), 0));
    TEST_EQUAL(4, ps_alignment_add_word(al, dict_wordid(dict, "meters"), 0));
    TEST_EQUAL(0, ps_alignment_populate(al));

    TEST_ASSERT(search = state_align_search_init("state_align", config, acmod, al));

    do_search(search, acmod);

    itor = ps_alignment_phones(al);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 0);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 53);
    TEST_EQUAL(ps_alignment_iter_get(itor)->score, -31);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 53);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 11);
    TEST_EQUAL(ps_alignment_iter_get(itor)->score, -69);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 64);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 15);
    TEST_EQUAL(ps_alignment_iter_get(itor)->score, -73);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 79);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 6);
    TEST_EQUAL(ps_alignment_iter_get(itor)->score, -56);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 85);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 9);
    TEST_EQUAL(ps_alignment_iter_get(itor)->score, -162);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 94);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 7);
    TEST_EQUAL(ps_alignment_iter_get(itor)->score, -195);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 101);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 11);
    TEST_EQUAL(ps_alignment_iter_get(itor)->score, -58);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 112);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 8);
    TEST_EQUAL(ps_alignment_iter_get(itor)->score, -100);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 120);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 11);
    TEST_EQUAL(ps_alignment_iter_get(itor)->score, -78);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 131);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 9);
    TEST_EQUAL(ps_alignment_iter_get(itor)->score, -74);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 140);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 15);
    TEST_EQUAL(ps_alignment_iter_get(itor)->score, -169);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 155);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 3);
    TEST_EQUAL(ps_alignment_iter_get(itor)->score, -75);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 158);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 13);
    TEST_EQUAL(ps_alignment_iter_get(itor)->score, -61);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 171);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 3);
    TEST_EQUAL(ps_alignment_iter_get(itor)->score, -149);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 174);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 16);
    TEST_EQUAL(ps_alignment_iter_get(itor)->score, -70);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 190);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 71);
    TEST_EQUAL(ps_alignment_iter_get(itor)->score, -2210);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(itor, NULL);

    ps_search_free(search);
    ps_alignment_free(al);

    /* Test bad alignment */

    al = ps_alignment_init(d2p);
    TEST_EQUAL(1, ps_alignment_add_word(al, dict_wordid(dict, "<s>"), 0));
    for (i = 0; i < 20; i++) {
        TEST_EQUAL(i + 2, ps_alignment_add_word(al, dict_wordid(dict, "hello"), 0));
    }
    TEST_EQUAL(22, ps_alignment_add_word(al, dict_wordid(dict, "</s>"), 0));
    TEST_EQUAL(0, ps_alignment_populate(al));
    TEST_ASSERT(search = state_align_search_init("state_align", config, acmod, al));
    E_INFO("Error here is expected, testing bad alignment\n");
    TEST_EQUAL(-1, do_search(search, acmod));

    ps_search_free(search);
    ps_alignment_free(al);

    ps_free(ps);
    cmd_ln_free_r(config);

    /* Test diphones trained AM */

    config = cmd_ln_init(NULL, ps_args(), FALSE,
                 "-hmm", MODELDIR "/en-us/en-us-diphones",
                 "-dict", MODELDIR "/en-us/cmudict-en-us-diphones.dict",
                 "-samprate", "16000", NULL);
    TEST_ASSERT(ps = ps_init(config));
    dict = ps->dict;
    d2p = ps->d2p;
    acmod = ps->acmod;

    al = ps_alignment_init(d2p);
    TEST_EQUAL(1, ps_alignment_add_word(al, dict_wordid(dict, "go"), 0));
    TEST_EQUAL(2, ps_alignment_add_word(al, dict_wordid(dict, "forward"), 0));
    TEST_EQUAL(3, ps_alignment_add_word(al, dict_wordid(dict, "ten"), 0));
    TEST_EQUAL(4, ps_alignment_add_word(al, dict_wordid(dict, "meters"), 0));
    TEST_EQUAL(0, ps_alignment_populate(al));

    TEST_ASSERT(search = state_align_search_init("state_align", config, acmod, al));

    do_search(search, acmod);

    itor = ps_alignment_phones(al);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 0);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 56);
    TEST_EQUAL(ps_alignment_iter_get(itor)->score, -36);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 56);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 7);
    TEST_EQUAL(ps_alignment_iter_get(itor)->score, -85);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 63);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 16);
    TEST_EQUAL(ps_alignment_iter_get(itor)->score, -75);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 79);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 13);
    TEST_EQUAL(ps_alignment_iter_get(itor)->score, -162);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 92);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 4);
    TEST_EQUAL(ps_alignment_iter_get(itor)->score, -27);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 96);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 7);
    TEST_EQUAL(ps_alignment_iter_get(itor)->score, -93);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 103);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 5);
    TEST_EQUAL(ps_alignment_iter_get(itor)->score, -61);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 108);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 13);
    TEST_EQUAL(ps_alignment_iter_get(itor)->score, -245);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 121);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 11);
    TEST_EQUAL(ps_alignment_iter_get(itor)->score, -75);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 132);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 21);
    TEST_EQUAL(ps_alignment_iter_get(itor)->score, -523);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 153);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 16);
    TEST_EQUAL(ps_alignment_iter_get(itor)->score, -100);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 169);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 4);
    TEST_EQUAL(ps_alignment_iter_get(itor)->score, -148);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 173);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 6);
    TEST_EQUAL(ps_alignment_iter_get(itor)->score, -39);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 179);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 6);
    TEST_EQUAL(ps_alignment_iter_get(itor)->score, -55);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(ps_alignment_iter_get(itor)->start, 185);
    TEST_EQUAL(ps_alignment_iter_get(itor)->duration, 76);
    TEST_EQUAL(ps_alignment_iter_get(itor)->score, -285);
    itor = ps_alignment_iter_next(itor);
    TEST_EQUAL(itor, NULL);

    ps_search_free(search);
    ps_alignment_free(al);

    /* Test bad alignment */

    al = ps_alignment_init(d2p);
    TEST_EQUAL(1, ps_alignment_add_word(al, dict_wordid(dict, "<s>"), 0));
    for (i = 0; i < 20; i++) {
        TEST_EQUAL(i + 2, ps_alignment_add_word(al, dict_wordid(dict, "hello"), 0));
    }
    TEST_EQUAL(22, ps_alignment_add_word(al, dict_wordid(dict, "</s>"), 0));
    TEST_EQUAL(0, ps_alignment_populate(al));
    TEST_ASSERT(search = state_align_search_init("state_align", config, acmod, al));
    E_INFO("Error here is expected, testing bad alignment\n");
    TEST_EQUAL(-1, do_search(search, acmod));

    ps_search_free(search);
    ps_alignment_free(al);

    ps_free(ps);
    cmd_ln_free_r(config);

    return 0;
}
