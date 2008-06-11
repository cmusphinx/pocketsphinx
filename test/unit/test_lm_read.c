#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>

#include "test_macros.h"

int
main(int argc, char *argv[])
{
	char const *hyp;
	char const *uttid;
	ps_decoder_t *ps;
	cmd_ln_t *config;
	ngram_model_t *lmset, *lm;
	FILE *rawfh;
	int32 score;

	/* First decode it with the crappy SWB language model. */
	TEST_ASSERT(config =
		    cmd_ln_init(NULL, ps_args(), TRUE,
				"-hmm", MODELDIR "/hmm/wsj1",
				"-lm", DATADIR "/wsj/wlist5o.nvp.lm.DMP",
				"-dict", DATADIR "/defective.dic",
				"-input_endian", "little",
				"-samprate", "16000", NULL));
	TEST_ASSERT(ps = ps_init(config));
	TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
	ps_decode_raw(ps, rawfh, "goforward", -1);
	hyp = ps_get_hyp(ps, &score, &uttid);
	printf("%s: %s (%d)\n", uttid, hyp, score);
	TEST_EQUAL(0, strcmp(hyp, "GO FORWARD TEN YEARS"));

	/* Now load the turtle language model. */
	lm = ngram_model_read(config, 
			      MODELDIR "/lm/turtle/turtle.lm",
			      NGRAM_AUTO, ps_get_logmath(ps));
	TEST_ASSERT(lm);
	lmset = ps_get_lmset(ps);
	TEST_ASSERT(lmset);
	ngram_model_set_add(lmset, lm, "turtle", 1.0, TRUE);
	ngram_model_set_select(lmset, "turtle");
	ps_update_lmset(ps, lmset);
	clearerr(rawfh);
	fseek(rawfh, 0, SEEK_SET);
	TEST_ASSERT(ps_decode_raw(ps, rawfh, "goforward", -1));
	hyp = ps_get_hyp(ps, &score, &uttid);
	printf("%s: %s (%d)\n", uttid, hyp, score);

	/* Oops!  It's still not correct, because METERS isn't in the
	 * dictionary that we originally loaded. */
	TEST_EQUAL(0, strcmp(hyp, "GO FORWARD TEN DEGREES"));
	/* So let's add it to the dictionary. */
	ps_add_word(ps, "METERS", "M IY T ER Z", TRUE);
	/* And try again. */
	clearerr(rawfh);
	fseek(rawfh, 0, SEEK_SET);
	TEST_ASSERT(ps_decode_raw(ps, rawfh, "goforward", -1));
	hyp = ps_get_hyp(ps, &score, &uttid);
	printf("%s: %s (%d)\n", uttid, hyp, score);
	/* Bingo! */
	TEST_EQUAL(0, strcmp(hyp, "GO FORWARD TEN METERS"));

	ps_free(ps);

	return 0;
}
