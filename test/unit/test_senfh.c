#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "pocketsphinx_internal.h"
#include "ngram_search_fwdtree.h"
#include "test_macros.h"

int
main(int argc, char *argv[])
{
    ps_decoder_t *ps;
    cmd_ln_t *config;
    acmod_t *acmod;
    ngram_search_t *ngs;

    TEST_ASSERT(config =
            cmd_ln_init(NULL, ps_args(), TRUE,
                "-hmm", MODELDIR "/en-us/en-us",
                "-lm", MODELDIR "/en-us/en-us.lm.bin",
                "-dict", MODELDIR "/en-us/cmudict-en-us.dict",
                "-fwdtree", "yes",
                "-fwdflat", "no",
                "-bestpath", "no",
                "-samprate", "16000", NULL));
    TEST_ASSERT(ps = ps_init(config));

    ngs = (ngram_search_t *)ps->search;
    acmod = ps->acmod;

    setbuf(stdout, NULL);
    {
        FILE *rawfh, *senfh;
        int16 buf[2048];
        size_t nread;
        int16 const *bptr;
        int nfr;

        TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
        TEST_EQUAL(0, acmod_start_utt(acmod));
        TEST_ASSERT(senfh = fopen("goforward.sen", "wb"));
        TEST_EQUAL(0, acmod_set_senfh(acmod, senfh));
        ngram_fwdtree_start(ngs);
        while (!feof(rawfh)) {
            nread = fread(buf, sizeof(*buf), 2048, rawfh);
            bptr = buf;
            while ((nfr = acmod_process_raw(acmod, &bptr, &nread, FALSE)) > 0) {
                while (acmod->n_feat_frame > 0) {
                    ngram_fwdtree_search(ngs, acmod->output_frame);
                    acmod_advance(acmod);
                }
            }
        }
        ngram_fwdtree_finish(ngs);
        printf("%s\n",
               ngram_search_bp_hyp(ngs, ngram_search_find_exit(ngs, -1, NULL)));

        TEST_ASSERT(acmod_end_utt(acmod) >= 0);
        fclose(rawfh);
        TEST_EQUAL(0, strcmp("go forward ten meters",
                     ngram_search_bp_hyp(ngs,
                             ngram_search_find_exit(ngs, -1, NULL))));

        TEST_EQUAL(0, acmod_set_senfh(acmod, NULL));
        TEST_EQUAL(0, acmod_start_utt(acmod));
        TEST_ASSERT(senfh = fopen("goforward.sen", "rb"));
        TEST_EQUAL(0, acmod_set_insenfh(acmod, senfh));
        ngram_fwdtree_start(ngs);
        while ((nfr = acmod_read_scores(acmod)) > 0) {
            while (acmod->n_feat_frame > 0) {
                ngram_fwdtree_search(ngs, acmod->output_frame);
                acmod_advance(acmod);
            }
        }
        ngram_fwdtree_finish(ngs);
        printf("%s\n",
               ngram_search_bp_hyp(ngs, ngram_search_find_exit(ngs, -1, NULL)));

        TEST_ASSERT(acmod_end_utt(acmod) >= 0);
        TEST_EQUAL(0, strcmp("go forward ten meters",
                     ngram_search_bp_hyp(ngs,
                             ngram_search_find_exit(ngs, -1, NULL))));
        fclose(senfh);
    }
    ps_free(ps);
    cmd_ln_free_r(config);

    return 0;
}
