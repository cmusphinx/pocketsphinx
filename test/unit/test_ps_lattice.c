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
	int32 score, n;

	TEST_ASSERT(config =
		    cmd_ln_init(NULL, ps_args(), TRUE,
				"-hmm", MODELDIR "/hmm/wsj1",
				"-lm", DATADIR "/wsj/wlist5o.nvp.lm.DMP",
				"-dict", MODELDIR "/lm/cmudict.0.6d",
				"-fwdtree", "yes",
				"-fwdflat", "yes",
				"-bestpath", "yes",
				"-input_endian", "little",
				"-samprate", "16000", NULL));
	TEST_ASSERT(ps = ps_init(config));
	TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
	ps_decode_raw(ps, rawfh, "goforward", -1);
	fclose(rawfh);
	hyp = ps_get_hyp(ps, &score, &uttid);
	printf("BESTPATH (%s): %s (%d)\n", uttid, hyp, score);
	TEST_ASSERT(dag = ps_get_lattice(ps));
	/* Now do some useful things with it. */

	TEST_EQUAL(0, ps_lattice_write(dag, "goforward.lat"));
	ps_free(ps);
	return 0;
}
