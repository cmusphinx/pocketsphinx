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
	FILE *rawfh;
	int16 *buf;
	size_t nread, nsamps;

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

	nsamps = 2048;
	buf = ckd_calloc(nsamps, sizeof(*buf));
	TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
	while (!feof(rawfh)) {
		int16 *bptr = buf;
		nread = fread(buf, sizeof(*buf), nsamps, rawfh);
		while (acmod_process_raw(acmod, &bptr, &nread, FALSE) > 0) {
			ascr_t const *senscr;
			int frame_idx, best_score, best_senid;
			while ((senscr = acmod_score(acmod, &frame_idx,
						     &best_score, &best_senid))) {
				/* Here we would do searching. */
			}
		}
	}
	fclose(rawfh);
	ckd_free(buf);
	acmod_free(acmod);
	return 0;
}
