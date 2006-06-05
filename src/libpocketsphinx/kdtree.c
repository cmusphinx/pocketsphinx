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
 * File: kdtree.c
 * 
 * Description: 
 * 	Implement kd-trees using the BBI algorithm as detailed in
 * 	J. Frisch and I. Rogina, "The Bucket Box Intersection
 * 	Algorithm for fast approximate evaluation of Diagonal Mixture
 * 	Gaussians", Proceedings of ICASSP 1996.
 *
 * Author: 
 * 	David Huggins-Daines <dhuggins@cs.cmu.edu>
 *********************************************************************/

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "kdtree.h"
#include "err.h"
#include "ckd_alloc.h"
#include "sc_vq_internal.h"

#define KDTREE_VERSION 1

static int32
read_tree_int(FILE *fp, const char *name, int32 *out, int32 optional)
{
	char line[256];
	int n;

	n = fscanf(fp, "%255s %d", line, out);
	if ((optional == 0 && n != 2) || strcmp(line, name)) {
		E_ERROR("%s not found: %d %s %d\n", name, n, line, out);
		return -1;
	}
	return n;
}

static int32
read_tree_float(FILE *fp, const char *name, float32 *out, int32 optional)
{
	char line[256];
	int n;

	n = fscanf(fp, "%255s %f", line, out);
	if ((optional == 0 && n != 2) || strcmp(line, name)) {
		E_ERROR("%s not found: %d %s %f\n", name, n, line, out);
		return -1;
	}
	return n;
}

static int32
read_bbi_list(FILE *fp, kd_tree_node_t *node, int32 maxbbi)
{
	uint8 bbi_list[NUM_ALPHABET];
	int bbi, nbbi = 0, nr;

	if (maxbbi == -1)
		maxbbi = NUM_ALPHABET;
	if ((nr = read_tree_int(fp, "bbi", &bbi, TRUE)) < 0)
		return -1;
	if (nr > 1) {
		if (bbi >= NUM_ALPHABET) {
			E_ERROR("BBI Gaussian %d out of range! %d\n", bbi);
			return -1;
		}
		bbi_list[0] = bbi;
		nbbi = 1;
		while (fscanf(fp, "%d", &bbi)) {
			if (feof(fp))
				break;
			if (bbi >= NUM_ALPHABET) {
				E_ERROR("BBI Gaussian %d out of range!\n", bbi);
				return -1;
			}
			if (nbbi < maxbbi)
				bbi_list[nbbi++] = bbi;
		}
	}
	if (node == NULL)
		return 0;
	if (nbbi > maxbbi)
		nbbi = maxbbi;
	node->n_bbi = nbbi;
	if (nbbi) {
		node->bbi = ckd_calloc(node->n_bbi, sizeof(*node->bbi));
		memcpy(node->bbi, bbi_list, node->n_bbi*sizeof(*node->bbi));
	}
	return 0;
}

static int32
read_kd_nodes(FILE *fp, kd_tree_t *tree, uint32 maxdepth, int32 maxbbi)
{
	uint32 i, j, in, out;
	int32 ilevel, olevel;

	/* Balanced binary trees, so we have 2^nlevels-1 nodes. */
	if (maxdepth == 0 || maxdepth > tree->n_level)
		maxdepth = tree->n_level;
	in = (1<<tree->n_level)-1;
	out = (1<<maxdepth)-1;
	tree->nodes = ckd_calloc(out, sizeof(kd_tree_node_t));

	/* Nodes are read in depth-first ordering. */
	for (j = i = 0; i < in; ++i) {
		float32 split_plane;
		int32 split_comp;

		if (read_tree_int(fp, "NODE", &ilevel, FALSE) < 0)
			break;
		if (read_tree_int(fp, "split_comp", &split_comp, FALSE) < 0)
			return -1;
		if (read_tree_float(fp, "split_plane", &split_plane, FALSE) < 0)
			return -1;
		olevel = ilevel - (tree->n_level - maxdepth);
		if (olevel > 0) {
			/* Only create a node if we are above maxdepth */
			assert(j < out);
			tree->nodes[j].split_comp = split_comp;
			tree->nodes[j].split_plane = FLOAT2MFCC(split_plane);
			/* We only need the BBI list for leafnodes now. */
			if (olevel == 1) {
				if (read_bbi_list(fp, tree->nodes + j, maxbbi) < 0)
					return -1;
			}
			else {
				if (read_bbi_list(fp, NULL, 0) < 0)
					return -1;
			}
			/* They are also full trees, hence: */
			if (olevel > 1) {
				tree->nodes[j].left = j + 1;
				tree->nodes[j].right = j + (1<<(olevel-1));
			}
			++j;
		}
		else { /* Have to read the BBI list anyway. */
			if (read_bbi_list(fp, NULL, 0) < 0)
				return -1;
		}
	}
	E_INFO("Read %d nodes\n", j);

	return 0;
}

int32
read_kd_trees(const char *infile, kd_tree_t ***out_trees, uint32 *out_n_trees,
	      uint32 maxdepth, int32 maxbbi)
{
	FILE *fp;
	char line[256];
	int n, version;
	uint32 i;

	if ((fp = fopen(infile, "r"))  == NULL) {
		E_ERROR("Failed to open %s", infile);
		return -1;
	}
	n = fscanf(fp, "%256s", line);
	if (n != 1 || strcmp(line, "KD-TREES")) {
		E_ERROR("%s doesn't appear to be a kd-tree file: %s\n",
			infile, line);
		return -1;
	}
	n = fscanf(fp, "%256s %d", line, &version);
	if (n != 2 || strcmp(line, "version") || version > KDTREE_VERSION) {
		E_ERROR("Unsupported kd-tree file format %s %d\n", line, version);
		return -1;
	}
	if (read_tree_int(fp, "n_trees", out_n_trees, FALSE) < 0)
		return -1;

	*out_trees = ckd_calloc(*out_n_trees, sizeof(kd_tree_t **));
	for (i = 0; i < *out_n_trees; ++i) {
		kd_tree_t *tree;
		uint32 n_density;
		float32 threshold;

		if (read_tree_int(fp, "TREE", &n, FALSE) < 0)
			goto error_out;
		if (n != i) {
			E_ERROR("Tree number %d out of sequence\n", n);
			goto error_out;
		}

		E_INFO("Reading tree for feature %d\n", i);
		(*out_trees)[i] = tree = ckd_calloc(1, sizeof(*tree));
		if (read_tree_int(fp, "n_density", &n_density, FALSE) < 0)
			goto error_out;
		if (n_density != NUM_ALPHABET) {
			E_ERROR("Number of densities (%d) must be 256!\n", n_density);
			goto error_out;
		}
		if (read_tree_int(fp, "n_comp", &tree->n_comp, FALSE) < 0)
			goto error_out;
		if (read_tree_int(fp, "n_level", &tree->n_level, FALSE) < 0)
			goto error_out;
		if (tree->n_level > 16) {
			E_ERROR("Depth of tree (%d) must be < 16!\n", tree->n_level);
			goto error_out;
		}
		if (read_tree_float(fp, "threshold", &threshold, FALSE) < 0)
			goto error_out;
		E_INFO("n_density %d n_comp %d n_level %d threshold %f\n",
		       n_density, tree->n_comp, tree->n_level, threshold);
		if (read_kd_nodes(fp, tree, maxdepth, maxbbi) < 0)
			goto error_out;
		if (maxdepth)
			tree->n_level = maxdepth;
	}
	fclose(fp);
	return 0;

error_out:
	fclose(fp);
	for (i = 0; i < *out_n_trees; ++i) {
		free_kd_tree((*out_trees)[i]);
		(*out_trees)[i] = NULL;
	}
	ckd_free(*out_trees);
	*out_trees = NULL;
	return -1;
}

void
free_kd_tree(kd_tree_t *tree)
{
	uint32 i, n;

	if (tree == NULL)
		return;
	/* Balanced binary trees, so we have 2^level-1 nodes. */
	n = (1<<tree->n_level)-1;
	for (i = 0; i < n; ++i)
		ckd_free(tree->nodes[n].bbi);
	ckd_free(tree->nodes);
	ckd_free(tree);
}

kd_tree_node_t *
eval_kd_tree(kd_tree_t *tree, mfcc_t *feat, uint32 maxdepth)
{
	uint32 node;

	node = 0; /* Root of tree. */
	while (tree->nodes[node].left && --maxdepth > 0) {
		if (feat[tree->nodes[node].split_comp]
		    < tree->nodes[node].split_plane)
			node = tree->nodes[node].left;
		else
			node = tree->nodes[node].right;
	}
	return tree->nodes + node;
}
