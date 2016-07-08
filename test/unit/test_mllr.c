#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>

#include "pocketsphinx_internal.h"
#include "test_macros.h"
#include "test_ps.c"

int
main(int argc, char *argv[])
{
	cmd_ln_t *config;
	ps_decoder_t *ps;
	FILE *rawfh;
	char const *hyp;
	int32 score;

	TEST_ASSERT(config =
		    cmd_ln_init(NULL, ps_args(), TRUE,
				"-hmm", DATADIR "/an4_ci_cont",
				"-lm", DATADIR "/turtle.lm.bin",
				"-dict", DATADIR "/turtle.dic",
				"-mllr", DATADIR "/mllr_matrices",
				"-samprate", "16000", NULL));

	TEST_ASSERT(ps = ps_init(config));
	TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
	ps_decode_raw(ps, rawfh, -1);
	fclose(rawfh);
	hyp = ps_get_hyp(ps, &score);
	printf("FWDFLAT: %s (%d)\n", hyp, score);
	ps_free(ps);
	cmd_ln_free_r(config);
	return 0;
}
