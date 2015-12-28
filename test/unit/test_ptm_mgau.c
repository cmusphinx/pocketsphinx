#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "pocketsphinx_internal.h"
#include "ptm_mgau.h"
#include "ms_mgau.h"
#include "test_macros.h"

static const mfcc_t prior[13] = {
	FLOAT2MFCC(37.03),
	FLOAT2MFCC(-1.01),
	FLOAT2MFCC(0.53),
	FLOAT2MFCC(0.49),
	FLOAT2MFCC(-0.60),
	FLOAT2MFCC(0.14),
	FLOAT2MFCC(-0.05),
	FLOAT2MFCC(0.25),
	FLOAT2MFCC(0.37),
	FLOAT2MFCC(0.58),
	FLOAT2MFCC(0.13),
	FLOAT2MFCC(-0.16),
	FLOAT2MFCC(0.17)
};

void
run_acmod_test(acmod_t *acmod)
{
	FILE *rawfh;
	int16 *buf;
	int16 const *bptr;
	size_t nread, nsamps;
	int nfr;
	int frame_counter;

	cmn_prior_set(acmod->fcb->cmn_struct, prior);
	nsamps = 2048;
	frame_counter = 0;
	buf = ckd_calloc(nsamps, sizeof(*buf));
	TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
	TEST_EQUAL(0, acmod_start_utt(acmod));
	printf("Incremental(2048):\n");
	while (!feof(rawfh)) {
		nread = fread(buf, sizeof(*buf), nsamps, rawfh);
		bptr = buf;
		while ((nfr = acmod_process_raw(acmod, &bptr, &nread, FALSE)) > 0) {
			int16 best_score;
			int frame_idx = -1, best_senid;
			while (acmod->n_feat_frame > 0) {
				acmod_score(acmod, &frame_idx);
				acmod_advance(acmod);
				best_score = acmod_best_score(acmod, &best_senid);
				printf("Frame %d best senone %d score %d\n",
				       frame_idx, best_senid, best_score);
				TEST_EQUAL(frame_counter, frame_idx);
				++frame_counter;
				frame_idx = -1;
			}
		}
	}
	TEST_EQUAL(0, acmod_end_utt(acmod));
	nread = 0;
	{
		int16 best_score;
		int frame_idx = -1, best_senid;
		while (acmod->n_feat_frame > 0) {
			acmod_score(acmod, &frame_idx);
			acmod_advance(acmod);
			best_score = acmod_best_score(acmod, &best_senid);
			printf("Frame %d best senone %d score %d\n",
			       frame_idx, best_senid, best_score);
			TEST_EQUAL(frame_counter, frame_idx);
			++frame_counter;
			frame_idx = -1;
		}
	}
	fclose(rawfh);
}

int
main(int argc, char *argv[])
{
	logmath_t *lmath;
	cmd_ln_t *config;
	acmod_t *acmod;
	ps_mgau_t *ps;
	ptm_mgau_t *s;
	int i, lastcb;

	lmath = logmath_init(1.0001, 0, 0);
	config = cmd_ln_init(NULL, ps_args(), TRUE,
	     "-compallsen", "yes",
	     "-input_endian", "little",
	     NULL);
	cmd_ln_parse_file_r(config, ps_args(), MODELDIR "/en-us/en-us/feat.params", FALSE);

	cmd_ln_set_str_extra_r(config, "_mdef", MODELDIR "/en-us/en-us/mdef");
	cmd_ln_set_str_extra_r(config, "_mean", MODELDIR "/en-us/en-us/means");
	cmd_ln_set_str_extra_r(config, "_var", MODELDIR "/en-us/en-us/variances");
	cmd_ln_set_str_extra_r(config, "_tmat", MODELDIR "/en-us/en-us/transition_matrices");
	cmd_ln_set_str_extra_r(config, "_sendump", MODELDIR "/en-us/en-us/sendump");
	cmd_ln_set_str_extra_r(config, "_mixw", NULL);
	cmd_ln_set_str_extra_r(config, "_lda", NULL);
	cmd_ln_set_str_extra_r(config, "_senmgau", NULL);	
	
	err_set_debug_level(3);
	TEST_ASSERT(config);
	TEST_ASSERT((acmod = acmod_init(config, lmath, NULL, NULL)));
	TEST_ASSERT((ps = acmod->mgau));
	TEST_EQUAL(0, strcmp(ps->vt->name, "ptm"));
	s = (ptm_mgau_t *)ps;
	E_DEBUG(2,("PTM model loaded: %d codebooks, %d senones, %d frames of history\n",
		   s->g->n_mgau, s->n_sen, s->n_fast_hist));
	E_DEBUG(2,("Senone to codebook mappings:\n"));
	lastcb = s->sen2cb[0];
	E_DEBUG(2,("\t%d: 0", lastcb));
	for (i = 0; i < s->n_sen; ++i) {
		if (s->sen2cb[i] != lastcb) {
			lastcb = s->sen2cb[i];
			E_DEBUGCONT(2,("-%d\n", i-1));
			E_DEBUGCONT(2,("\t%d: %d", lastcb, i));
		}
	}
	E_INFOCONT("-%d\n", i-1);
	run_acmod_test(acmod);

#if 0
	/* Replace it with ms_mgau. */
	ptm_mgau_free(ps);
	cmd_ln_set_str_r(config,
			 "-mixw",
			 MODELDIR "/en-us/en-us/mixture_weights");
	TEST_ASSERT((acmod->mgau = ms_mgau_init(acmod, lmath, acmod->mdef)));
	run_acmod_test(acmod);
	cmd_ln_free_r(config);
#endif

	return 0;
}
