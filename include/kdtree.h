/* ====================================================================
 * Copyright (c) 2005 Carnegie Mellon University.  All rights 
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * This work was supported in part by funding from the Defense Advanced 
 * Research Projects Agency and the National Science Foundation of the 
 * United States of America, and the CMU Sphinx Speech Consortium.
 *
 * THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND 
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
 * NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ====================================================================
 *
 */
/*********************************************************************
 *
 * File: kdtree.h
 * 
 * Description: Cut-down kd-trees for Sphinx2 decoding
 * 
 * Author: David Huggins-Daines <dhuggins@cs.cmu.edu>
 * 
 *********************************************************************/

#ifndef __KDTREE_H__
#define __KDTREE_H__

#include <fixpoint.h>
#include <fe.h>

#include "s2types.h"
#include "vector.h"

typedef struct kd_tree_node_s kd_tree_node_t;
struct kd_tree_node_s {
	uint8 *bbi; /* BBI list of intersecting Gaussians */
	mfcc_t split_plane;
	uint16 n_bbi, split_comp;
	uint16 left, right; /* Indices of left and right child nodes */
};
typedef struct kd_tree_s kd_tree_t;
struct kd_tree_s {
	uint32 n_nodes, n_level, n_comp;
	kd_tree_node_t *nodes;
};

int32 read_kd_trees(const char *infile, kd_tree_t ***out_trees, uint32 *out_n_trees,
		    uint32 maxdepth, int32 maxbbi);
void free_kd_tree(kd_tree_t *tree);
kd_tree_node_t *eval_kd_tree(kd_tree_t *tree, mfcc_t *feat, uint32 maxdepth);

#endif /* __KDTREE_H__ */
