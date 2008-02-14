#include <fbs.h>
#include <stdio.h>
#include <string.h>

#include "acmod.h"
#include "search.h"
#include "log.h"
#include "test_macros.h"

int
main(int argc, char *argv[])
{
	FILE *rawfh;
	int16 buf[2048];
	size_t nread;
	static char *cmdln[] = {
		"-hmm", MODELDIR "/hmm/wsj1",
		"-lm", MODELDIR "/lm/swb/swb.lm.DMP",
		"-dict", MODELDIR "/lm/swb/swb.dic",
		"-fwdtree", "yes",
		"-fwdflat", "no",
		"-bestpath", "no",
		"-samprate", "16000"
	};
	static const int cmdln_count = sizeof(cmdln)/sizeof(cmdln[0]);
	acmod_t *acmod;
	int32 nfr;
	int16 const *bptr;

	TEST_EQUAL(0, fbs_init(cmdln_count,cmdln));
	acmod = acmod_init(cmd_ln_get(), g_lmath, NULL, NULL);

	setbuf(stdout, NULL);
	TEST_ASSERT(rawfh = fopen(DATADIR "/goforward.raw", "rb"));
	TEST_EQUAL(0, acmod_start_utt(acmod));
	search_start_fwd();
	while (!feof(rawfh)) {
		nread = fread(buf, sizeof(*buf), 2048, rawfh);
		bptr = buf;
		while ((nfr = acmod_process_raw(acmod, &bptr, &nread, FALSE)) > 0) {
			while (acmod->n_feat_frame) {
				search_fwd(acmod->feat_buf[0]);
				--acmod->n_feat_frame;
				memmove(acmod->feat_buf[0][0], acmod->feat_buf[1][0],
					(acmod->n_feat_frame
					 * feat_dimension(acmod->fcb)
					 * sizeof(***acmod->feat_buf)));
			}
		}
	}
	search_finish_fwd();
	TEST_EQUAL(0, acmod_end_utt(acmod));
	fbs_end();
	return 0;
}
