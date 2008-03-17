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
	FILE *rawfh;
	int16 buf[2048];
	size_t nread;

	TEST_ASSERT(config =
		    cmd_ln_init(NULL, pocketsphinx_args(), TRUE,
				"-hmm", MODELDIR "/hmm/wsj1",
				"-lm", MODELDIR "/lm/swb/swb.lm.DMP",
				"-dict", MODELDIR "/lm/swb/swb.dic",
				"-cepext", "",
				"-fwdtree", "yes",
				"-fwdflat", "no",
				"-bestpath", "no",
				"-samprate", "16000", NULL));
	TEST_ASSERT(ps = pocketsphinx_init(config));
	/* HACK!  Need a way to do this in the API. */
	cmn_prior_set(ps->acmod->fcb->cmn_struct, prior);

	TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
	TEST_EQUAL(0, pocketsphinx_start_utt(ps));
	while (!feof(rawfh)) {
		nread = fread(buf, sizeof(*buf), 2048, rawfh);
		pocketsphinx_process_raw(ps, buf, nread, FALSE, FALSE);
	}
	TEST_EQUAL(0, pocketsphinx_end_utt(ps));
	fclose(rawfh);
	pocketsphinx_free(ps);

	return 0;
}
