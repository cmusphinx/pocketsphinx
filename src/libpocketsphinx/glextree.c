/**
 * @file glextree.h
 * @brief Generic lexicon trees for token-fsg search (and maybe elsewhere)
 * @author David Huggins-Daines <dhuggins@cs.cmu.edu>
 */

#include "glextree.h"

glextree_t *
glextree_build(dict_t *dict, dict2pid_t *d2p)
{
	glextree_t *tree;

	tree = ckd_calloc(1, sizeof(*tree));
	tree->refcount = 1;

	return tree;
}

glextree_t *
glextree_retain(glextree_t *tree)
{
	if (tree == NULL)
		return NULL;
	++tree->refcount;
	return tree;
}

int
glextree_free(glextree_t *tree)
{
	if (tree == NULL)
		return 0;
	if (--tree->refcount > 0)
		return tree->refcount;
	ckd_free(tree);
	return 0;
}

glexnode_t *
glextree_find_root(glextree_t *tree, int32 ci, int32 lc)
{
	int32 key[2];
	void *val;

	key[0] = ci;
	key[1] = lc;
	if (hash_tree_lookup_bkey(tree->rhash, key, sizeof(key), &val) < 0)
		return NULL;
	return (glexnode_t *)val;
}

