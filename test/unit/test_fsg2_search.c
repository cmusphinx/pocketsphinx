#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "pocketsphinx_internal.h"
#include "fsg2_search.h"
#include "test_macros.h"

int
main(int argc, char *argv[])
{
	ps_decoder_t *ps;
	cmd_ln_t *config;
	acmod_t *acmod;
	fsg2_search_t *fsgs;
	int32 score;

	TEST_ASSERT(config =
		    cmd_ln_init(NULL, ps_args(), TRUE,
				"-hmm", MODELDIR "/hmm/en_US/hub4wsj_sc_8k",
				"-fsg", DATADIR "/goforward.fsg",
				"-dict", MODELDIR "/lm/en_US/cmu07a.dic",
				"-bestpath", "no",
				"-input_endian", "little",
				"-samprate", "16000", NULL));
	TEST_ASSERT(ps = ps_init(config));

	acmod = ps->acmod;
	fsgs = (fsg2_search_t *)fsg2_search_init(config, acmod, ps->dict, ps->d2p);

	setbuf(stdout, NULL);
	{
		FILE *rawfh;
		int16 buf[2048];
		size_t nread;
		int16 const *bptr;
		char const *hyp;
		int nfr;

		TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
		TEST_EQUAL(0, acmod_start_utt(acmod));
		fsg2_search_start(ps_search_base(fsgs));
		while (!feof(rawfh)) {
			nread = fread(buf, sizeof(*buf), 2048, rawfh);
			bptr = buf;
			while ((nfr = acmod_process_raw(acmod, &bptr, &nread, FALSE)) > 0) {
				while (acmod->n_feat_frame > 0) {
					fsg2_search_step(ps_search_base(fsgs),
							 acmod->output_frame);
					acmod_advance(acmod);
				}
			}
		}
		fsg2_search_finish(ps_search_base(fsgs));
		hyp = fsg2_search_hyp(ps_search_base(fsgs), &score);
		printf("FSG: %s (%d)\n", hyp, score);

		TEST_ASSERT(acmod_end_utt(acmod) >= 0);
		fclose(rawfh);
	}
	TEST_EQUAL(0, strcmp("go forward ten meters",
			     fsg2_search_hyp(ps_search_base(fsgs), &score)));
	ps_free(ps);

	return 0;
}
