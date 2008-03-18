#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>

#include "pocketsphinx_internal.h"

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

int
main(int argc, char *argv[])
{
	pocketsphinx_t *ps;
	cmd_ln_t *config;
	mfcc_t **cepbuf;
	FILE *rawfh;
	int16 *buf;
	int16 const *bptr;
	size_t nread;
	size_t nsamps;
	int32 nfr, i, score;
	char const *hyp;
	double n_speech, n_cpu, n_wall;

	TEST_ASSERT(config =
		    cmd_ln_init(NULL, pocketsphinx_args(), TRUE,
				"-hmm", MODELDIR "/hmm/wsj1",
				"-lm", MODELDIR "/lm/swb/swb.lm.DMP",
				"-dict", MODELDIR "/lm/swb/swb.dic",
				"-cepext", "",
				"-fwdtree", "yes",
				"-fwdflat", "no",
				"-bestpath", "no",
				"-input_endian", "little",
				"-samprate", "16000", NULL));
	TEST_ASSERT(ps = pocketsphinx_init(config));
	/* HACK!  Need a way to do this in the API. */
	cmn_prior_set(ps->acmod->fcb->cmn_struct, prior);

	TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
	TEST_EQUAL(0, pocketsphinx_start_utt(ps));
	nsamps = 2048;
	buf = ckd_calloc(nsamps, sizeof(*buf));
	while (!feof(rawfh)) {
		nread = fread(buf, sizeof(*buf), nsamps, rawfh);
		pocketsphinx_process_raw(ps, buf, nread, FALSE, FALSE);
	}
	TEST_EQUAL(0, pocketsphinx_end_utt(ps));
	hyp = pocketsphinx_get_hyp(ps, &score);
	printf("FWDTREE: %s (%d)\n", hyp, score);
	TEST_EQUAL(0, strcmp(hyp, "GO FOR WORDS TEN YEARS"));
	pocketsphinx_get_utt_time(ps, &n_speech, &n_cpu, &n_wall);
	printf("%.2f seconds speech, %.2f seconds CPU, %.2f seconds wall\n",
	       n_speech, n_cpu, n_wall);
	printf("%.2f xRT (CPU), %.2f xRT (elapsed)\n",
	       n_cpu / n_speech, n_wall / n_speech);

	/* Now read the whole file and produce an MFCC buffer. */
	clearerr(rawfh);
	fseek(rawfh, 0, SEEK_END);
	nsamps = ftell(rawfh) / sizeof(*buf);
	fseek(rawfh, 0, SEEK_SET);
	bptr = buf = ckd_realloc(buf, nsamps * sizeof(*buf));
	TEST_EQUAL(nsamps, fread(buf, sizeof(*buf), nsamps, rawfh));
	fe_process_frames(ps->acmod->fe, &bptr, &nsamps, NULL, &nfr);
	cepbuf = ckd_calloc_2d(nfr + 1,
			       fe_get_output_size(ps->acmod->fe),
			       sizeof(**cepbuf));
	fe_start_utt(ps->acmod->fe);
	fe_process_frames(ps->acmod->fe, &bptr, &nsamps, cepbuf, &nfr);
	fe_end_utt(ps->acmod->fe, cepbuf[nfr], &i);

	/* Decode it with process_cep() */
	TEST_EQUAL(0, pocketsphinx_start_utt(ps));
	cmn_prior_set(ps->acmod->fcb->cmn_struct, prior);
	for (i = 0; i < nfr; ++i) {
		pocketsphinx_process_cep(ps, cepbuf + i, 1, FALSE, FALSE);
	}
	TEST_EQUAL(0, pocketsphinx_end_utt(ps));
	hyp = pocketsphinx_get_hyp(ps, &score);
	printf("FWDTREE: %s (%d)\n", hyp, score);
	TEST_EQUAL(0, strcmp(hyp, "GO FOR WORDS TEN YEARS"));
	pocketsphinx_get_utt_time(ps, &n_speech, &n_cpu, &n_wall);
	printf("%.2f seconds speech, %.2f seconds CPU, %.2f seconds wall\n",
	       n_speech, n_cpu, n_wall);
	printf("%.2f xRT (CPU), %.2f xRT (elapsed)\n",
	       n_cpu / n_speech, n_wall / n_speech);
	pocketsphinx_get_all_time(ps, &n_speech, &n_cpu, &n_wall);
	printf("TOTAL: %.2f seconds speech, %.2f seconds CPU, %.2f seconds wall\n",
	       n_speech, n_cpu, n_wall);
	printf("TOTAL: %.2f xRT (CPU), %.2f xRT (elapsed)\n",
	       n_cpu / n_speech, n_wall / n_speech);

	fclose(rawfh);
	pocketsphinx_free(ps);
	ckd_free_2d(cepbuf);
	ckd_free(buf);

	return 0;
}
