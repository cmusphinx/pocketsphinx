#include <ngram_model.h>
#include <logmath.h>
#include <strfuncs.h>

#include "test_macros.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

void
run_tests(logmath_t *lmath, ngram_model_t *model)
{
	int32 score;

	ngram_model_add_word(model, "foobie", 1.0);
	score = ngram_score(model, "foobie", NULL);
	TEST_EQUAL_LOG(score, logmath_log(lmath, 1.0/401)); /* #unigrams */

	ngram_model_add_word(model, "quux", 0.5);
	score = ngram_score(model, "quux", NULL);
	TEST_EQUAL_LOG(score, logmath_log(lmath, 0.5/402)); /* #unigrams */
}

int
main(int argc, char *argv[])
{
	logmath_t *lmath;
	ngram_model_t *model;

	lmath = logmath_init(1.0001, 0, 0);

	model = ngram_model_read(NULL, LMDIR "/100.lm.dmp", NGRAM_BIN, lmath);
	run_tests(lmath, model);
	ngram_model_free(model);

	model = ngram_model_read(NULL, LMDIR "/100.lm.gz", NGRAM_ARPA, lmath);
	run_tests(lmath, model);
	ngram_model_free(model);

	logmath_free(lmath);
	return 0;
}
