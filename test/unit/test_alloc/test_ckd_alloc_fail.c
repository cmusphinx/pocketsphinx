#include <stdio.h>

#include "util/ckd_alloc.h"

#include "test_macros.h"

int
main(int argc, char *argv[])
{
	int bad_alloc_did_not_fail = FALSE;

	(void)argc;
	(void)argv;
	ckd_set_jump(NULL, FALSE);
	/* Guaranteed to fail, we hope!. */
	(void) ckd_calloc(-1,-1);
	TEST_ASSERT(bad_alloc_did_not_fail);

	return 0;
}
