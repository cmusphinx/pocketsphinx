#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "pocketsphinx_internal.h"
#include "ngram_search_fwdtree.h"
#include "test_macros.h"

int
main(int argc, char *argv[])
{
	pocketsphinx_t *ps;
	cmd_ln_t *config;
	acmod_t *acmod;
	ngram_search_t *ngs;
	clock_t c;
	int i;

	TEST_ASSERT(config =
		    cmd_ln_init(NULL, pocketsphinx_args(), TRUE,
				"-hmm", MODELDIR "/hmm/wsj1",
				"-lm", MODELDIR "/lm/swb/swb.lm.DMP",
				"-dict", MODELDIR "/lm/swb/swb.dic",
				"-fwdtree", "yes",
				"-fwdflat", "no",
				"-bestpath", "no",
				"-samprate", "16000", NULL));
	TEST_ASSERT(ps = pocketsphinx_init(config));

	ngs = ps->ngs;
	acmod = ps->acmod;

	setbuf(stdout, NULL);
	c = clock();
	for (i = 0; i < 5; ++i) {
		FILE *rawfh;
		int16 buf[2048];
		size_t nread;
		int16 const *bptr;
		int nfr;

		TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
		TEST_EQUAL(0, acmod_start_utt(acmod));
		ngram_fwdtree_start(ngs);
		while (!feof(rawfh)) {
			nread = fread(buf, sizeof(*buf), 2048, rawfh);
			bptr = buf;
			while ((nfr = acmod_process_raw(acmod, &bptr, &nread, FALSE)) > 0) {
				while (ngram_fwdtree_search(ngs)) {
				}
			}
		}
		ngram_fwdtree_finish(ngs);
		printf("%s\n",
		       ngram_search_hyp(ngs, ngram_search_find_exit(ngs, -1, NULL)));

		TEST_EQUAL(0, acmod_end_utt(acmod));
		fclose(rawfh);
	}
	TEST_EQUAL(0, strcmp("GO FOR WORDS TEN YEARS",
			     ngram_search_hyp(ngs, ngram_search_find_exit(ngs, -1, NULL))));
	c = clock() - c;
	printf("5 * fwdtree search in %.2f sec\n",
	       (double)c / CLOCKS_PER_SEC);
	pocketsphinx_free(ps);

	return 0;
}
