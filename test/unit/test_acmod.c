#include <stdio.h>
#include <string.h>
#include <pocketsphinx.h>
#include <logmath.h>

#include "acmod.h"
#include "test_macros.h"

int
main(int argc, char *argv[])
{
	acmod_t *acmod;
	logmath_t *lmath;
	cmd_ln_t *config;

	lmath = logmath_init(1.0001, 0, 0);
	config = cmd_ln_init(NULL, pocketsphinx_args(), TRUE,
			     "-featparams", MODELDIR "/hmm/wsj1/feat.params",
			     "-mdef", MODELDIR "/hmm/wsj1/mdef",
			     "-mean", MODELDIR "/hmm/wsj1/means",
			     "-var", MODELDIR "/hmm/wsj1/variances",
			     "-tmat", MODELDIR "/hmm/wsj1/transition_matrices",
			     "-sendump", MODELDIR "/hmm/wsj1/sendump",
			     "-tmatfloor", "0.0001",
			     "-mixwfloor", "0.001",
			     "-varfloor", "0.0001",
			     "-mmap", "no",
			     "-topn", "4",
			     "-dsratio", "1",
			     "-samprate", "16000", NULL);
	TEST_ASSERT(config);
	TEST_ASSERT(acmod = acmod_init(config, lmath, NULL, NULL));

	acmod_free(acmod);
	return 0;
}
