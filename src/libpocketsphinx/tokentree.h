/**
 * @name tokentree.h
 * @brief Token-passing search algorithm
 * @author David Huggins-Daines <dhuggins@cs.cmu.edu>
 */

#ifndef __TOKENTREE_H__
#define __TOKENTREE_H__

/* System includes. */

/* SphinxBase includes. */
#include <listelem_alloc.h>

/* Local includes. */


/**
 * Token, which represents a particular path through the decoding graph.
 */
typedef struct token_s token_t;
struct token_s {
	int32 pathscore;  /**< Score of the path ending with this token. */
	int32 arcid;      /**< Head arc (or word) represented by this token. */
	token_t *prev;    /**< Previous token in this path. */
};

/**
 * Tree (lattice) of tokens, representing all paths explored so far.
 */
typedef struct tokentree_s tokentree_t;
struct tokentree_s {
	listelem_alloc_t *token_alloc;  /**< Allocator for tokens. */
};

#endif /* __TOKENTREE_H__ */
