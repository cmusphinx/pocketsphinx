#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>

#include "pocketsphinx_internal.h"
#include "test_macros.h"
#include "ps_test.c"

int
main(int argc, char *argv[])
{
	ps_decoder_t *ps;
	ps_lattice_t *dag;
	cmd_ln_t *config;
	FILE *rawfh;
	char const *hyp;
	char const *uttid;
	int32 score;

	TEST_ASSERT(config =
		    cmd_ln_init(NULL, ps_args(), TRUE,
				"-hmm", MODELDIR "/hmm/wsj1",
				"-lm", DATADIR "/wsj/wlist5o.nvp.lm.DMP",
				"-dict", MODELDIR "/lm/cmudict.0.6d",
				"-fwdtree", "yes",
				"-fwdflat", "yes",
				"-bestpath", "no",
				"-input_endian", "little",
				"-samprate", "16000", NULL));
	TEST_ASSERT(ps = ps_init(config));
	TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
	ps_decode_raw(ps, rawfh, "goforward", -1);
	fclose(rawfh);
	hyp = ps_get_hyp(ps, &score, &uttid);
	printf("FWDFLAT (%s): %s (%d)\n", uttid, hyp, score);
	TEST_ASSERT(dag = ps_get_lattice(ps));
	ps_lattice_bestpath(dag, ps_get_lmset(ps), 1.0, 1.0/15.0);
	score = ps_lattice_posterior(dag, ps_get_lmset(ps), 1.0/15.0);

	/* Test node iterators. */
	{
		ps_latnode_iter_t *itor;
		TEST_ASSERT(itor = ps_latnode_iter(dag));
		while ((itor = ps_latnode_iter_next(itor))) {
			int16 sf, fef, lef;
			ps_latnode_t *node;
			float64 post;

			node = ps_latnode_iter_node(itor);
			sf = ps_latnode_times(node, &fef, &lef);
			post = logmath_exp(ps_lattice_get_logmath(dag),
					   ps_latnode_prob(dag, node, NULL));
			printf("%s %d -> (%d,%d) %f\n",
			       ps_latnode_word(dag, node), sf,
			       fef, lef, post);
		}
	}

	TEST_EQUAL(0, ps_lattice_write(dag, "goforward.lat"));
	ps_free(ps);
	return 0;
}
