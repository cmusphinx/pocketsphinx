#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <jsgf.h>
#include <fsg_model.h>

#include "pocketsphinx_internal.h"
#include "fsg_search_internal.h"
#include "test_macros.h"

int
main(int argc, char *argv[])
{
	ps_decoder_t *ps;
	cmd_ln_t *config;
	acmod_t *acmod;
	fsg_search_t *fsgs;
	jsgf_t *jsgf;
	jsgf_rule_t *rule;
	fsg_model_t *fsg;
	ps_seg_t *seg;
	ps_lattice_t *dag;
	int32 score;
	clock_t c;
	int i;

	TEST_ASSERT(config =
		    cmd_ln_init(NULL, ps_args(), TRUE,
				"-hmm", MODELDIR "/hmm/en_US/hub4wsj_sc_8k",
				"-dict", MODELDIR "/lm/en/turtle.dic",
				"-input_endian", "little",
				"-samprate", "16000", NULL));
	TEST_ASSERT(ps = ps_init(config));
        fsgs = (fsg_search_t *)fsg_search_init(config, ps->acmod, ps->dict, ps->d2p);
	acmod = ps->acmod;

	jsgf = jsgf_parse_file(DATADIR "/goforward.gram", NULL);
	TEST_ASSERT(jsgf);
	rule = jsgf_get_rule(jsgf, "<goforward.move2>");
	TEST_ASSERT(rule);
	fsg = jsgf_build_fsg(jsgf, rule, ps->lmath, 7.5);
	TEST_ASSERT(fsg);
	fsg_model_write(fsg, stdout);
	TEST_ASSERT(fsg_set_add(fsgs, "<goforward.move2>", fsg));
	TEST_ASSERT(fsg_set_select(fsgs, "<goforward.move2>"));
	fsg_search_reinit(ps_search_base(fsgs), ps->dict, ps->d2p);

	setbuf(stdout, NULL);
	c = clock();
	for (i = 0; i < 5; ++i) {
		FILE *rawfh;
		int16 buf[2048];
		size_t nread;
		int16 const *bptr;
		char const *hyp;
		int nfr;

		TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
		TEST_EQUAL(0, acmod_start_utt(acmod));
		fsg_search_start(ps_search_base(fsgs));
		while (!feof(rawfh)) {
			nread = fread(buf, sizeof(*buf), 2048, rawfh);
			bptr = buf;
			while ((nfr = acmod_process_raw(acmod, &bptr, &nread, FALSE)) > 0) {
				while (acmod->n_feat_frame > 0) {
					fsg_search_step(ps_search_base(fsgs),
							acmod->output_frame);
					acmod_advance(acmod);
				}
			}
		}
		fsg_search_finish(ps_search_base(fsgs));
		hyp = fsg_search_hyp(ps_search_base(fsgs), &score);
		printf("FSG: %s (%d)\n", hyp, score);

		TEST_ASSERT(acmod_end_utt(acmod) >= 0);
		fclose(rawfh);
	}
	TEST_EQUAL(0, strcmp("go forward ten meters",
			     fsg_search_hyp(ps_search_base(fsgs), &score)));
	ps->search = fsgs;
	for (seg = ps_seg_iter(ps, &score); seg;
	     seg = ps_seg_next(seg)) {
		char const *word;
		int sf, ef;

		word = ps_seg_word(seg);
		ps_seg_frames(seg, &sf, &ef);
		printf("%s %d %d\n", word, sf, ef);
	}
	c = clock() - c;
	printf("5 * fsg search in %.2f sec\n", (double)c / CLOCKS_PER_SEC);

	dag = ps_get_lattice(ps);
	ps_lattice_write(dag, "test_jsgf.lat");
	ps_free(ps);
	jsgf_grammar_free(jsgf);
	fsg_search_free(ps_search_base(fsgs));

	return 0;
}
