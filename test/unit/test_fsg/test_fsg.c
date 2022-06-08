#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "pocketsphinx_internal.h"
#include "fsg_search_internal.h"
#include "ps_lattice_internal.h"
#include "test_macros.h"

int
main(int argc, char *argv[])
{
    ps_decoder_t *ps;
    cmd_ln_t *config;
    ps_lattice_t *dag;
    const char *hyp;
    ps_seg_t *seg;
    int32 score, prob;
    FILE *rawfh;

    TEST_ASSERT(config =
            cmd_ln_init(NULL, ps_args(), TRUE,
                "-hmm", MODELDIR "/en-us/en-us",
                "-fsg", DATADIR "/goforward.fsg",
                "-dict", DATADIR "/turtle.dic",
                "-bestpath", "no",
                "-samprate", "16000", NULL));
    TEST_ASSERT(ps = ps_init(config));

    TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
    ps_decode_raw(ps, rawfh, -1);
    hyp = ps_get_hyp(ps, &score);
    prob = ps_get_prob(ps);
    printf("%s (%d, %d)\n", hyp, score, prob);
    TEST_EQUAL(0, strcmp("go forward ten meters", hyp));


    for (seg = ps_seg_iter(ps); seg;
         seg = ps_seg_next(seg)) {
        char const *word;
        int sf, ef;
        int32 post, lscr, ascr, lback;

        word = ps_seg_word(seg);
        ps_seg_frames(seg, &sf, &ef);
        if (sf == ef)
            continue;
        post = ps_seg_prob(seg, &ascr, &lscr, &lback);
        printf("%s (%d:%d) P(w|o) = %f ascr = %d lscr = %d lback = %d\n", word, sf, ef,
               logmath_exp(ps_get_logmath(ps), post), ascr, lscr, lback);
    }

    /* Now get the DAG and play with it. */
    dag = ps_get_lattice(ps);
    ps_lattice_write(dag, "test_fsg.lat");
    printf("BESTPATH: %s\n",
           ps_lattice_hyp(dag, ps_lattice_bestpath(dag, NULL, 1.0, 15.0)));
    ps_lattice_posterior(dag, NULL, 15.0);
    ps_free(ps);
    cmd_ln_free_r(config);

    return 0;
}
