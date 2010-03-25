/**
 * @file glextree.h
 * @brief Generic lexicon trees for token-fsg search (and maybe elsewhere)
 * @author David Huggins-Daines <dhuggins@cs.cmu.edu>
 */

#ifndef __GLEXTREE_H__
#define __GLEXTREE_H__

/* System includes. */

/* SphinxBase includes. */
#include <hash_table.h>

/* Local includes. */
#include "hmm.h"
#include "dict2pid.h"
#include "dict.h"

/**
 * Generic lexicon tree node structure.
 */
typedef struct glexnode_s glexnode_t;
struct glexnode_s {
	hmm_t hmm;         /**< Base HMM structure. */
	int refcount;      /**< Reference count. */
	glexnode_t *sibs;  /**< Siblings of this node. */
	glexnode_t *kids;  /**< Children of this node, NULL for leaves. */
	int32 rc;          /**< Right context information (for leaf nodes). */
};

/**
 * Generic lexicon tree structure.
 */
typedef struct glextree_s glextree_t;
struct glextree_s {
	int refcount;        /**< Reference count. */
	int n_roots;         /**< Number of root nodes. */
	glexnode_t *roots;   /**< Array of root nodes. */
	hash_table_t *rhash; /**< Hash table mapping diphones
				  to root nodes. */
};

/**
 * Build a lexicon tree from a dictionary and context mapping.
 */
glextree_t *glextree_build(dict_t *dict, dict2pid_t *d2p);

/**
 * Retain a pointer to a lexicon tree.
 */
glextree_t *glextree_retain(glextree_t *tree);

/**
 * Release a pointer to a lexicon tree.
 */
int glextree_free(glextree_t *tree);

/**
 * Find the root node corresponding to a base phone and left context.
 */
glexnode_t *glextree_find_root(glextree_t *tree, int32 ci, int32 lc);

#endif /* __GLEXTREE_H__ */
