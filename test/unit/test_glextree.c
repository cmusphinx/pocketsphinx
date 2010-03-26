#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "pocketsphinx_internal.h"
#include "glextree.h"
#include "test_macros.h"

int
main(int argc, char *argv[])
{
	dict_t *dict;
	dict2pid_t *d2p;
	bin_mdef_t *mdef;
	glextree_t *tree;
	hmm_context_t *ctx;
	logmath_t *lmath;
	tmat_t *tmat;

	TEST_ASSERT(mdef = bin_mdef_read(NULL, MODELDIR "/hmm/en_US/hub4wsj_sc_8k/mdef"));
	TEST_ASSERT(dict = dict_init(cmd_ln_init(NULL, NULL, FALSE,
						 "-dict", DATADIR "/turtle.dic",
						 "-dictcase", "no", NULL),
				       mdef));
	TEST_ASSERT(d2p = dict2pid_build(mdef, dict));
	lmath = logmath_init(1.0001, 0, TRUE);
	tmat = tmat_init(MODELDIR "/hmm/en_US/hub4wsj_sc_8k/transition_matrices",
			 lmath, 1e-5, TRUE);
	ctx = hmm_context_init(bin_mdef_n_emit_state(mdef), tmat->tp, NULL, mdef->sseq);
	TEST_ASSERT(tree = glextree_build(ctx, dict, d2p, NULL, NULL));

	dict_free(dict);
	dict2pid_free(d2p);
	bin_mdef_free(mdef);
	tmat_free(tmat);
	hmm_context_free(ctx);
	glextree_free(tree);
	return 0;
}
