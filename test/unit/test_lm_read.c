#include <fbs.h>
#include <stdio.h>
#include <string.h>

#include "test_macros.h"

static char *my_argv[] = {
	"test_lm_read",
	"-hmm", MODELDIR "/hmm/wsj1",
	"-lm", MODELDIR "/lm/swb/swb.lm.DMP",
	"-dict", MODELDIR "/lm/swb/swb.dic",
	"-cepext", "",
	"-samprate", "16000"
};
static const int my_argc = sizeof(my_argv) / sizeof(my_argv[0]);

int
main(int argc, char *argv[])
{
	int32 frm;
	char *hyp;

	/* First decode it with the crappy SWB language model. */
	TEST_EQUAL(0, fbs_init(my_argc, my_argv));
	TEST_ASSERT(uttproc_decode_raw_file(DATADIR "/goforward.raw",
					    "goforward", 0, -1, FALSE));
	TEST_EQUAL(0, uttproc_result(&frm, &hyp, TRUE));
	/* FIXME: The trailing space might not always be there. */
	TEST_EQUAL(0, strcmp(hyp, "GO FORWARD TEN YEARS "));

	/* Now load the turtle language model. */
	TEST_EQUAL(0, lm_read(MODELDIR "/lm/turtle/turtle.lm",
			      "turtle", 7.5, 0.5, 0.65));
	TEST_EQUAL(0, uttproc_set_lm("turtle"));
	TEST_ASSERT(uttproc_decode_raw_file(DATADIR "/goforward.raw",
					    "goforward", 0, -1, FALSE));
	TEST_EQUAL(0, uttproc_result(&frm, &hyp, TRUE));

	/* Oops!  It's still not correct, because METERS isn't in the
	 * dictionary that we originally loaded. */
	TEST_EQUAL(0, strcmp(hyp, "GO FORWARD TEN DEGREES "));
	/* So let's add it to the dictionary. */
	TEST_ASSERT(-1 != uttproc_add_word("METERS", "M IY T ER Z", TRUE));
	/* And try again. */
	TEST_ASSERT(uttproc_decode_raw_file(DATADIR "/goforward.raw",
					    "goforward", 0, -1, FALSE));
	TEST_EQUAL(0, uttproc_result(&frm, &hyp, TRUE));
	/* Bingo! */
	TEST_EQUAL(0, strcmp(hyp, "GO FORWARD TEN METERS "));

	TEST_EQUAL(0, fbs_end());
	return 0;
}
