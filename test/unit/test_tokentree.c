#include <pocketsphinx.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "pocketsphinx_internal.h"
#include "tokentree.h"
#include "test_macros.h"

int
main(int argc, char *argv[])
{
	tokentree_t *tree;
	token_t *root, *tok;
	void *data;
	int32 val;

	tree = tokentree_init();

	TEST_ASSERT(root = tokentree_add(tree, 0, 0, NULL));
	heap_top(tree->leaves, &data, &val);
	TEST_EQUAL(data, (void *)root);
	tokentree_add(tree, 10, 4, root);
	tok = tokentree_add(tree, 8, 1, root);
	tokentree_add(tree, 33, 2, root);
	tokentree_add(tree, 14, 3, root);
	heap_top(tree->leaves, &data, &val);
	TEST_EQUAL(data, (void *)tok);
	TEST_EQUAL(heap_size(tree->leaves), 4);
	tokentree_prune(tree, 15);
	TEST_EQUAL(heap_size(tree->leaves), 3);
	tokentree_add(tree, 8, 1, root);
	tokentree_add(tree, 42, 6, root);
	tokentree_prune_topn(tree, 3);
	TEST_EQUAL(heap_size(tree->leaves), 3);

	tokentree_free(tree);
	return 0;
}
