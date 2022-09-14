#include <stdio.h>

#include "util/ckd_alloc.h"

#include "test_macros.h"

int
main(int argc, char *argv[])
{
	jmp_buf env;

	(void)argc;
	(void)argv;
	ckd_set_jump(&env, FALSE);
	if (setjmp(env)) {
		printf("Successfully caught bad allocation!\n");
	}
	else {
		int failed_to_catch_bad_alloc = FALSE;

		/* Guaranteed to fail, we hope!. */
		(void) ckd_calloc(-1,-1);
		TEST_ASSERT(failed_to_catch_bad_alloc);
	}

	return 0;
}
