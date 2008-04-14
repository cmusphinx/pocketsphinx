#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <jsgf.h>
#include <fsg_model.h>

#include "pocketsphinx_internal.h"
#include "fsg_search2.h"
#include "test_macros.h"

int
main(int argc, char *argv[])
{
	pocketsphinx_t *ps;
	cmd_ln_t *config;
	acmod_t *acmod;
	fsg_search2_t *fsgs;
	jsgf_t *jsgf;
	jsgf_rule_t *rule;
	fsg_model_t *fsg;
	clock_t c;
	int i;

	TEST_ASSERT(config =
		    cmd_ln_init(NULL, pocketsphinx_args(), TRUE,
				"-hmm", MODELDIR "/hmm/wsj1",
				"-dict", MODELDIR "/lm/turtle/turtle.dic",
				"-input_endian", "little",
				"-samprate", "16000", NULL));
	TEST_ASSERT(ps = pocketsphinx_init(config));
        fsgs = (fsg_search2_t *)fsg_search2_init(config, ps->acmod, ps->dict);
	acmod = ps->acmod;

	jsgf = jsgf_parse_file(DATADIR "/goforward.gram", NULL);
	TEST_ASSERT(jsgf);
	rule = jsgf_get_rule(jsgf, "<goforward.move>");
	TEST_ASSERT(rule);
	fsg = jsgf_build_fsg(jsgf, rule, ps->lmath, 7.5);
	TEST_ASSERT(fsg);
	TEST_ASSERT(fsg_search2_add(fsgs, "<goforward.move>", fsg));
	TEST_ASSERT(fsg_search2_select(fsgs, "<goforward.move>"));

	setbuf(stdout, NULL);
	c = clock();
	for (i = 0; i < 5; ++i) {
		FILE *rawfh;
		int16 buf[2048];
		size_t nread;
		int16 const *bptr;
		char const *hyp;
		int32 score;
		int nfr;

		TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
		TEST_EQUAL(0, acmod_start_utt(acmod));
		fsg_search2_start(ps_search_base(fsgs));
		while (!feof(rawfh)) {
			nread = fread(buf, sizeof(*buf), 2048, rawfh);
			bptr = buf;
			while ((nfr = acmod_process_raw(acmod, &bptr, &nread, FALSE)) > 0) {
				while (fsg_search2_step(ps_search_base(fsgs))) {
				}
			}
		}
		fsg_search2_finish(ps_search_base(fsgs));
		hyp = fsg_search2_hyp(ps_search_base(fsgs), &score);
		printf("FSG: %s (%d)\n", hyp, score);

		TEST_ASSERT(acmod_end_utt(acmod) >= 0);
		fclose(rawfh);
	}
	c = clock() - c;
	printf("5 * fsg search in %.2f sec\n", (double)c / CLOCKS_PER_SEC);
	pocketsphinx_free(ps);
	jsgf_grammar_free(jsgf);

	return 0;
}
