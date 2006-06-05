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
 * File: bin_mdef.c
 * 
 * Description: 
 *	Binary format model definition files, with support for
 *	heterogeneous topologies and variable-size N-phones
 *
 * Author: 
 * 	David Huggins-Daines <dhuggins@cs.cmu.edu>
 *********************************************************************/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "bin_mdef.h"
#include "ckd_alloc.h"
#include "byteorder.h"
#include "err.h"

/* FIXME: Sphinx-II legacy bogosity. */
extern int32 totalDists;

bin_mdef_t *
bin_mdef_read_text(const char *filename)
{
	bin_mdef_t *bmdef;
	mdef_t *mdef;
	int i, nodes, ci_idx, lc_idx, rc_idx;
	int nchars;

	if ((mdef = mdef_init((char *)filename)) == NULL)
		return NULL;

	bmdef = ckd_calloc(1, sizeof(*bmdef));

	/* Easy stuff.  The mdef.c code has done the heavy lifting for us. */
	bmdef->n_ciphone = mdef->n_ciphone;
	bmdef->n_phone = mdef->n_phone;
	bmdef->n_emit_state = mdef->n_emit_state;
	bmdef->n_ci_sen = mdef->n_ci_sen;
	bmdef->n_sen = mdef->n_sen;
	bmdef->n_tmat = mdef->n_tmat;
	bmdef->n_sseq = mdef->n_sseq;
	bmdef->sseq = mdef->sseq;
	bmdef->cd2cisen = mdef->cd2cisen;
	bmdef->sen2cimap = mdef->sen2cimap;
	bmdef->n_ctx = 3; /* Triphones only. */
	bmdef->sil = mdef->sil;
	mdef->sseq = NULL; /* We are taking over this one. */
	mdef->cd2cisen = NULL; /* And this one. */
	mdef->sen2cimap = NULL; /* And this one. */

	/* Set totalDists like read_map() would have done for S2 models. */
	totalDists = mdef->n_sen;

	/* Get the phone names.  If they are not sorted
	 * ASCII-betically then we are in a world of hurt and
	 * therefore will simply refuse to continue. */
	bmdef->ciname = ckd_calloc(bmdef->n_ciphone, sizeof(*bmdef->ciname));
	nchars = 0;
	for (i = 0; i < bmdef->n_ciphone; ++i)
		nchars += strlen(mdef->ciphone[i].name) + 1;
	bmdef->ciname[0] = ckd_calloc(nchars, 1);
	strcpy(bmdef->ciname[0], mdef->ciphone[0].name);
	for (i = 1; i < bmdef->n_ciphone; ++i) {
		bmdef->ciname[i] = bmdef->ciname[i-1] + strlen(bmdef->ciname[i-1]) + 1;
		strcpy(bmdef->ciname[i], mdef->ciphone[i].name);
		if (i > 0 && strcmp(bmdef->ciname[i-1], bmdef->ciname[i]) > 0)
			E_FATAL("Phone names are not in sorted order, sorry.");
	}

	/* Copy over phone information. */
	bmdef->phone = ckd_calloc(bmdef->n_phone, sizeof(*bmdef->phone));
	for (i = 0; i < mdef->n_phone; ++i) {
		bmdef->phone[i].ssid = mdef->phone[i].ssid;
		bmdef->phone[i].tmat = mdef->phone[i].tmat;
		if (i < bmdef->n_ciphone) {
			bmdef->phone[i].info.ci.filler = mdef->ciphone[i].filler;
		}
		else {
			bmdef->phone[i].info.cd.wpos = mdef->phone[i].wpos;
			bmdef->phone[i].info.cd.ctx[0] = mdef->phone[i].ci;
			bmdef->phone[i].info.cd.ctx[1] = mdef->phone[i].lc;
			bmdef->phone[i].info.cd.ctx[2] = mdef->phone[i].rc;
		}
	}

	/* Walk the wpos_ci_lclist once to find the total number of
	 * nodes and the starting locations for each level. */
	nodes = lc_idx = ci_idx = rc_idx = 0;
	for (i = 0; i < N_WORD_POSN; ++i) {
		int j;
		for (j = 0; j < mdef->n_ciphone; ++j) {
			ph_lc_t *lc;

			for (lc = mdef->wpos_ci_lclist[i][j];
			     lc; lc = lc->next) {
				ph_rc_t *rc;
				for (rc = lc->rclist; rc; rc = rc->next) {
					++nodes; /* RC node */
				}
				++nodes; /* LC node */
				++rc_idx; /* Start of RC nodes (after LC nodes) */
			}
			++nodes; /* CI node */
			++lc_idx; /* Start of LC nodes (after CI nodes) */
			++rc_idx; /* Start of RC nodes (after CI and LC nodes) */
		}
		++nodes; /* wpos node */
		++ci_idx; /* Start of CI nodes (after wpos nodes) */
		++lc_idx; /* Start of LC nodes (after CI nodes) */
		++rc_idx; /* STart of RC nodes (after wpos, CI, and LC nodes) */
	}
	E_INFO("cd_tree: nodes %d wpos start 0 ci start %d lc start %d rc start %d\n",
	       nodes, ci_idx, lc_idx, rc_idx);
	bmdef->n_cd_tree = nodes;
	bmdef->cd_tree = ckd_calloc(nodes, sizeof(*bmdef->cd_tree));
	for (i = 0; i < N_WORD_POSN; ++i) {
		int j;

		bmdef->cd_tree[i].ctx = i;
		bmdef->cd_tree[i].n_down = mdef->n_ciphone;
		bmdef->cd_tree[i].c.down = ci_idx;
#if 0
		E_INFO("%d => %c (%d@%d)\n",
		       i, (WPOS_NAME)[i],
		       bmdef->cd_tree[i].n_down,
		       bmdef->cd_tree[i].c.down);
#endif

		/* Now we can build the rest of the tree. */
		for (j = 0; j < mdef->n_ciphone; ++j) {
			ph_lc_t *lc;

			bmdef->cd_tree[ci_idx].ctx = j;
			bmdef->cd_tree[ci_idx].c.down = lc_idx;
			for (lc = mdef->wpos_ci_lclist[i][j]; lc; lc = lc->next) {
				ph_rc_t *rc;

				bmdef->cd_tree[lc_idx].ctx = lc->lc;
				bmdef->cd_tree[lc_idx].c.down = rc_idx;
				for (rc = lc->rclist; rc; rc = rc->next) {
					bmdef->cd_tree[rc_idx].ctx = rc->rc;
					bmdef->cd_tree[rc_idx].n_down = 0;
					bmdef->cd_tree[rc_idx].c.pid = rc->pid;
#if 0
					E_INFO("%d => %s %s %s %c (%d@%d)\n", 
					       rc_idx,
					       bmdef->ciname[j],
					       bmdef->ciname[lc->lc],
					       bmdef->ciname[rc->rc],
					       (WPOS_NAME)[i],
					       bmdef->cd_tree[rc_idx].n_down,
					       bmdef->cd_tree[rc_idx].c.down);
#endif

					++bmdef->cd_tree[lc_idx].n_down;
					++rc_idx;
				}
				/* If there are no triphones here,
				 * this is considered a leafnode, so
				 * set the pid to BAD_S3PID. */
				if (bmdef->cd_tree[lc_idx].n_down == 0)
					bmdef->cd_tree[lc_idx].c.pid = BAD_S3PID;
#if 0
				E_INFO("%d => %s %s %c (%d@%d)\n", 
				       lc_idx, 
				       bmdef->ciname[j],
				       bmdef->ciname[lc->lc],
				       (WPOS_NAME)[i],
				       bmdef->cd_tree[lc_idx].n_down,
				       bmdef->cd_tree[lc_idx].c.down);
#endif

				++bmdef->cd_tree[ci_idx].n_down;
				++lc_idx;
			}

			/* As above, so below. */
			if (bmdef->cd_tree[ci_idx].n_down == 0)
				bmdef->cd_tree[ci_idx].c.pid = BAD_S3PID;
#if 0
			E_INFO("%d => %d=%s (%d@%d)\n", 
			       ci_idx, j, bmdef->ciname[j],
			       bmdef->cd_tree[ci_idx].n_down,
			       bmdef->cd_tree[ci_idx].c.down);
#endif

			++ci_idx;
		}
	}

	/* FIXME: Need an mdef_free().  For now we will do it manually. */
	s3hash_free(mdef->ciphone_ht);
	for (i = 0; i < mdef->n_ciphone; ++i)
		ckd_free(mdef->ciphone[i].name);
	ckd_free(mdef->ciphone);
	ckd_free(mdef->phone);
	ckd_free(mdef->ciphone2n_cd_sen);
	ckd_free_2d((void **)mdef->wpos_ci_lclist);
	ckd_free(mdef);

	bmdef->alloc_mode = BIN_MDEF_FROM_TEXT;
	return bmdef;
}

void
bin_mdef_free(bin_mdef_t *m)
{
	if (m->alloc_mode == BIN_MDEF_FROM_TEXT) {
		ckd_free(m->sseq[0]);
		ckd_free(m->cd_tree);
	}
	if (m->alloc_mode == BIN_MDEF_IN_MEMORY)
		ckd_free(m->ciname[0]);

	ckd_free(m->ciname);
	ckd_free(m->sseq);
	ckd_free(m);
}

static const char format_desc[] =
"BEGIN FILE FORMAT DESCRIPTION\n"
"int32 n_ciphone;    /**< Number of base (CI) phones */\n"
"int32 n_phone;	     /**< Number of base (CI) phones + (CD) triphones */\n"
"int32 n_emit_state; /**< Number of emitting states per phone (0 if heterogeneous) */\n"
"int32 n_ci_sen;     /**< Number of CI senones; these are the first */\n"
"int32 n_sen;	     /**< Number of senones (CI+CD) */\n"
"int32 n_tmat;	     /**< Number of transition matrices */\n"
"int32 n_sseq;       /**< Number of unique senone sequences */\n"
"int32 n_ctx;	     /**< Number of phones of context */\n"
"int32 n_cd_tree;    /**< Number of nodes in CD tree structure */\n"
"int32 sil;	     /**< CI phone ID for silence */\n"
"char ciphones[][];  /**< CI phone strings (null-terminated) */\n"
"char padding[];     /**< Padding to a 4-bytes boundary */\n"
"struct { int16 ctx; int16 n_down; int32 pid/down } cd_tree[];\n"
"struct { int32 ssid; int32 tmat; int8 attr[4] } phones[];\n"
"int32 sseq[];       /**< Unique senone sequences */\n"
"int8 sseq_len[];    /**< Number of states in each sseq (none if homogeneous) */\n"
"END FILE FORMAT DESCRIPTION\n"
;

bin_mdef_t *
bin_mdef_read(const char *filename)
{
	bin_mdef_t *m;
	FILE *fh;
	size_t tree_start;
	int32 val, i, swap, pos, end;
	int32 *sseq_size;

	/* Try to read it as text first. */
	if ((m = bin_mdef_read_text(filename)) != NULL)
		return m;

	E_INFO("Reading binary model definition: %s\n", filename);
	if ((fh = fopen(filename, "rb")) == NULL)
		return NULL;

	if (fread(&val, 4, 1, fh) != 1)
		E_FATAL_SYSTEM("Failed to read byte-order marker from %s\n", filename);
	swap = 0;
	if (strncmp((char *)&val, "BMDF", 4)) {
		swap = 1;
		E_INFO("Must byte-swap %s\n", filename);
	}
	if (fread(&val, 4, 1, fh) != 1)
		E_FATAL_SYSTEM("Failed to read version from %s\n", filename);
	if (swap) SWAP_INT32(&val);
	if (val > BIN_MDEF_FORMAT_VERSION) {
		E_ERROR("File format version %d for %s is newer than library\n",
			val, filename);
		fclose(fh);
		return NULL;
	}
	if (fread(&val, 4, 1, fh) != 1)
		E_FATAL_SYSTEM("Failed to read header length from %s\n", filename);
	if (swap) SWAP_INT32(&val);
	/* Skip format descriptor. */
	fseek(fh, val, SEEK_CUR);

	/* Finally allocate it. */
	m = ckd_calloc(1, sizeof(*m));

	/* Don't bother to check each one, since they will all fail if one failed. */
	fread(&m->n_ciphone, 4, 1, fh);
	if (swap) SWAP_INT32(&m->n_ciphone);
	fread(&m->n_phone, 4, 1, fh);
	if (swap) SWAP_INT32(&m->n_phone);
	fread(&m->n_emit_state, 4, 1, fh);
	if (swap) SWAP_INT32(&m->n_emit_state);
	fread(&m->n_ci_sen, 4, 1, fh);
	if (swap) SWAP_INT32(&m->n_ci_sen);
	fread(&m->n_sen, 4, 1, fh);
	if (swap) SWAP_INT32(&m->n_sen);
	fread(&m->n_tmat, 4, 1, fh);
	if (swap) SWAP_INT32(&m->n_tmat);
	fread(&m->n_sseq, 4, 1, fh);
	if (swap) SWAP_INT32(&m->n_sseq);
	fread(&m->n_ctx, 4, 1, fh);
	if (swap) SWAP_INT32(&m->n_ctx);
	fread(&m->n_cd_tree, 4, 1, fh);
	if (swap) SWAP_INT32(&m->n_cd_tree);
	if (fread(&val, 4, 1, fh) != 1)
		E_FATAL_SYSTEM("Failed to read header from %s\n", filename);
	m->sil = val;

	/* Read the rest of the file as a big block of memory, and
	 * carve it into the relevant chunks (in the future we will
	 * mmap() it instead if possible). */
	pos = ftell(fh);
	fseek(fh, 0, SEEK_END);
	end = ftell(fh);
	fseek(fh, pos, SEEK_SET);

	m->ciname = ckd_calloc(m->n_ciphone, sizeof(*m->ciname));
	m->ciname[0] = ckd_malloc(end-pos);
	if (fread(m->ciname[0], 1, end-pos, fh) != end-pos)
		E_FATAL_SYSTEM("Failed to read %d bytes of data from %s\n",
			       end-pos, filename);
	for (i = 1; i < m->n_ciphone; ++i)
		m->ciname[i] = m->ciname[i-1] + strlen(m->ciname[i-1]) + 1;

	/* Skip past the padding. */
	tree_start = m->ciname[i-1] + strlen(m->ciname[i-1]) + 1 - m->ciname[0];
	tree_start = (tree_start + 3) & ~3;
	m->cd_tree = (cd_tree_t *)(m->ciname[0] + tree_start);
	if (swap) {
		for (i = 0; i < m->n_cd_tree; ++i) {
			SWAP_INT16(&m->cd_tree[i].ctx);
			SWAP_INT16(&m->cd_tree[i].n_down);
			SWAP_INT32(&m->cd_tree[i].c.down);
		}
	}
	m->phone = (mdef_entry_t *)(m->cd_tree + m->n_cd_tree);
	if (swap) {
		for (i = 0; i < m->n_phone; ++i) {
			SWAP_INT32(&m->phone[i].ssid);
			SWAP_INT32(&m->phone[i].tmat);
		}
	}
	sseq_size = (int32 *)(m->phone + m->n_phone);
	if (swap) SWAP_INT32(sseq_size);
	m->sseq = ckd_calloc(m->n_sseq, sizeof(*m->sseq));
	m->sseq[0] = (s3senid_t *)(sseq_size + 1);
	if (swap) {
		for (i = 0; i < *sseq_size; ++i)
			SWAP_INT16(m->sseq[0] + i);
	}
	if (m->n_emit_state) {
		for (i = 1; i < m->n_sseq; ++i)
			m->sseq[i] = m->sseq[0] + i * m->n_emit_state;
	}
	else {
		m->sseq_len = (int8 *)(m->sseq[0] + *sseq_size);
		for (i = 1; i < m->n_sseq; ++i)
			m->sseq[i] = m->sseq[i-1] + m->sseq_len[i-1];
	}

	/* Set totalDists like read_map() would have done for S2 models. */
	totalDists = m->n_sen;

	/* Now build the CD-to-CI mappings using the senone sequences.
	 * This is the only really accurate way to do it, though it is
	 * still inaccurate in the case of heterogeneous topologies or
	 * cross-state tying. */
	m->cd2cisen = (s3senid_t *) ckd_malloc (m->n_sen * sizeof(s3senid_t));
	m->sen2cimap = (s3cipid_t *) ckd_malloc (m->n_sen * sizeof(s3cipid_t));

	/* Default mappings (identity, none) */
	for (i = 0; i < m->n_ci_sen; ++i)
		m->cd2cisen[i] = i;
	for (;i < m->n_sen; ++i)
		m->cd2cisen[i] = BAD_S3SENID;
	for (i = 0; i < m->n_sen; ++i)
		m->sen2cimap[i] = BAD_S3CIPID;
	for (i = 0; i < m->n_phone; ++i) {
		int32 j, ssid = m->phone[i].ssid;

		for (j = 0; j < bin_mdef_n_emit_state_phone(m, i); ++j) {
			s3senid_t s = bin_mdef_sseq2sen(m, ssid, j);
			s3cipid_t ci = bin_mdef_pid2ci(m, i);
			/* Take the first one and warn if we have cross-state tying. */
			if (m->sen2cimap[s] == BAD_S3CIPID)
				m->sen2cimap[s] = ci;
			if (m->sen2cimap[s] != ci)
				E_WARN("Senone %d is shared between multiple base phones\n", s);

			if (j > bin_mdef_n_emit_state_phone(m, ci))
				E_WARN("CD phone %d has fewer states than CI phone %d\n",
				       i, ci);
			else
				m->cd2cisen[s] = bin_mdef_sseq2sen(m, m->phone[ci].ssid, j);
		}
	}

	/* Set the silence phone. */
	m->sil = bin_mdef_ciphone_id(m, S3_SILENCE_CIPHONE);

	E_INFO("%d CI-phone, %d CD-phone, %d emitstate/phone, %d CI-sen, %d Sen, %d Sen-Seq\n",
	       m->n_ciphone, m->n_phone - m->n_ciphone, m->n_emit_state,
	       m->n_ci_sen, m->n_sen, m->n_sseq);
	m->alloc_mode = BIN_MDEF_IN_MEMORY;
	return m;
}

int
bin_mdef_write(bin_mdef_t *m, const char *filename)
{
	FILE *fh;
	int32 val, i;

	if ((fh = fopen(filename, "wb")) == NULL)
		return -1;

	/* Byteorder marker. */
	fwrite("BMDF", 1, 4, fh);
	/* Version. */
	val = BIN_MDEF_FORMAT_VERSION;
	fwrite(&val, 1, sizeof(val), fh);

	/* Round the format descriptor size up to a 4-byte boundary. */
	val = ((sizeof(format_desc) + 3) & ~3);
	fwrite(&val, 1, sizeof(val), fh);
	fwrite(format_desc, 1, sizeof(format_desc), fh);
	/* Pad it with zeros. */
	i = 0;
	fwrite(&i, 1, val - sizeof(format_desc), fh);

	/* Binary header things. */
	fwrite(&m->n_ciphone, 4, 1, fh);
	fwrite(&m->n_phone, 4, 1, fh);
	fwrite(&m->n_emit_state, 4, 1, fh);
	fwrite(&m->n_ci_sen, 4, 1, fh);
	fwrite(&m->n_sen, 4, 1, fh);
	fwrite(&m->n_tmat, 4, 1, fh);
	fwrite(&m->n_sseq, 4, 1, fh);
	fwrite(&m->n_ctx, 4, 1, fh);
	fwrite(&m->n_cd_tree, 4, 1, fh);
	/* Write this as a 32-bit value to preserve alignment for the
	 * non-mmap case (we want things aligned both from the
	 * beginning of the file and the beginning of the phone
	 * strings). */
	val = m->sil;
	fwrite(&val, 4, 1, fh);

	/* Phone strings. */
	for (i = 0; i < m->n_ciphone; ++i)
		fwrite(m->ciname[i], 1, strlen(m->ciname[i])+1, fh);
	/* Pad with zeros. */
	val = (ftell(fh) + 3) & ~3;
	i = 0;
	fwrite(&i, 1, val - ftell(fh), fh);

	/* Write CD-tree */
	fwrite(m->cd_tree, sizeof(*m->cd_tree), m->n_cd_tree, fh);
	/* Write phones */
	fwrite(m->phone, sizeof(*m->phone), m->n_phone, fh);
	if (m->n_emit_state) {
		/* Write size of sseq */
		val = m->n_sseq * m->n_emit_state;
		fwrite(&val, 4, 1, fh);

		/* Write sseq */
		fwrite(m->sseq[0], sizeof(s3senid_t), m->n_sseq * m->n_emit_state, fh);
	}
	else {
		int32 n;

		/* Calcluate size of sseq */
		n = 0;
		for (i = 0; i < m->n_sseq; ++i)
			n += m->sseq_len[i];

		/* Write size of sseq */
		fwrite(&n, 4, 1, fh);

		/* Write sseq */
		fwrite(m->sseq[0], sizeof(s3senid_t), n, fh);

		/* Write sseq_len */
		fwrite(m->sseq_len, 1, m->n_sseq, fh);
	}
	fclose(fh);

	return 0;
}

int
bin_mdef_write_text(bin_mdef_t *m, const char *filename)
{
	FILE *fh;
	int p, i, n_total_state;

	if (strcmp(filename, "-") == 0)
		fh = stdout;
	else {
		if ((fh = fopen(filename, "w")) == NULL)
			return -1;
	}

	fprintf(fh, "0.3\n");
	fprintf(fh, "%d n_base\n", m->n_ciphone);
	fprintf(fh, "%d n_tri\n", m->n_phone - m->n_ciphone);
	if (m->n_emit_state)
		n_total_state = m->n_phone * (m->n_emit_state + 1);
	else {
		n_total_state = 0;
		for (i = 0; i < m->n_phone; ++i)
			n_total_state += m->sseq_len[m->phone[i].ssid] + 1;
	}
	fprintf(fh, "%d n_state_map\n", n_total_state);
	fprintf(fh, "%d n_tied_state\n", m->n_sen);
	fprintf(fh, "%d n_tied_ci_state\n", m->n_ci_sen);
	fprintf(fh, "%d n_tied_tmat\n", m->n_tmat);
	fprintf(fh, "#\n# Columns definitions\n");
	fprintf(fh, "#%4s %3s %3s %1s %6s %4s %s\n",
		"base", "lft", "rt", "p", "attrib", "tmat",
		"     ... state id's ...");

	for (p = 0; p < m->n_ciphone; p++) {
		int n_state;

		fprintf(fh, "%5s %3s %3s %1s",
			m->ciname[p],
			"-", "-", "-");

		if (bin_mdef_is_fillerphone(m, p))
			fprintf(fh, " %6s", "filler");
		else
			fprintf(fh, " %6s", "n/a");
		fprintf(fh, " %4d", m->phone[p].tmat);

		if (m->n_emit_state)
			n_state = m->n_emit_state;
		else
			n_state = m->sseq_len[m->phone[p].ssid];
		for (i = 0; i < n_state; i++) {
			fprintf(fh, " %6u", m->sseq[m->phone[p].ssid][i]);
		}
		fprintf(fh, " N\n");
	}


	for (; p < m->n_phone; p++) {
		int n_state;

		fprintf(fh, "%5s %3s %3s %c",
			m->ciname[m->phone[p].info.cd.ctx[0]],
			m->ciname[m->phone[p].info.cd.ctx[1]],
			m->ciname[m->phone[p].info.cd.ctx[2]],
			(WPOS_NAME)[m->phone[p].info.cd.wpos]);

		if (bin_mdef_is_fillerphone(m, p))
			fprintf(fh, " %6s", "filler");
		else
			fprintf(fh, " %6s", "n/a");
		fprintf(fh, " %4d", m->phone[p].tmat);


		if (m->n_emit_state)
			n_state = m->n_emit_state;
		else
			n_state = m->sseq_len[m->phone[p].ssid];
		for (i = 0; i < n_state; i++) {
			fprintf(fh, " %6u", m->sseq[m->phone[p].ssid][i]);
		}
		fprintf(fh, " N\n");
	}

	if (strcmp(filename, "-") != 0)
		fclose(fh);
	return 0;
}

s3cipid_t
bin_mdef_ciphone_id (bin_mdef_t *m,
		     const char *ciphone)
{
	int low, mid, high;

	/* Exact binary search on m->ciphone */
	low = 0;
	high = m->n_ciphone;
	while (low < high) {
		int c;

		mid = (low+high)/2;
		c = strcmp(ciphone, m->ciname[mid]);
		if (c == 0)
			return mid;
		else if (c > 0)
			low = mid+1;
		else if (c < 0)
			high = mid;
	}
	return BAD_S3CIPID;
}

const char *
bin_mdef_ciphone_str (bin_mdef_t *m,
		      int32 ci)
{
	assert(m != NULL);
	assert(ci < m->n_ciphone);
	return m->ciname[ci];
}

s3pid_t
bin_mdef_phone_id (bin_mdef_t *m,
		   int32 ci,
		   int32 lc,
		   int32 rc,
		   int32 wpos)
{
	cd_tree_t *cd_tree;
	int level, max;
	s3cipid_t ctx[4];

	assert (m);
	assert ((ci >= 0) && (ci < m->n_ciphone));
	assert ((lc >= 0) && (lc < m->n_ciphone));
	assert ((rc >= 0) && (rc < m->n_ciphone));
	assert ((wpos >= 0) && (wpos < N_WORD_POSN));

	/* Create a context list, mapping fillers to silence. */
	ctx[0] = wpos;
	ctx[1] = ci;
	ctx[2] = (IS_S3CIPID(m->sil) && m->phone[lc].info.ci.filler) ? m->sil : lc;
	ctx[3] = (IS_S3CIPID(m->sil) && m->phone[rc].info.ci.filler) ? m->sil : rc;

	/* Walk down the cd_tree. */
	cd_tree = m->cd_tree;
	level = 0; /* What level we are on. */
	max = N_WORD_POSN; /* Number of nodes on this level. */
	while (level < 4) {
		int i;

#if 0
		E_INFO("Looking for context %d=%s in %d at %d\n",
		       ctx[level], m->ciname[ctx[level]],
		       max, cd_tree - m->cd_tree);
#endif
		for (i = 0; i < max; ++i) {
#if 0
			E_INFO("Look at context %d=%s at %d\n",
			       cd_tree[i].ctx,
			       m->ciname[cd_tree[i].ctx],
			       cd_tree + i - m->cd_tree);
#endif
			if (cd_tree[i].ctx == ctx[level])
				break;
		}
		if (i == max)
			return BAD_S3PID;
#if 0
		E_INFO("Found context %d=%s at %d, n_down=%d, down=%d\n",
		       ctx[level], m->ciname[ctx[level]],
		       cd_tree + i - m->cd_tree,
		       cd_tree[i].n_down, cd_tree[i].c.down);
#endif
		/* Leaf node, stop here. */
		if (cd_tree[i].n_down == 0)
			return cd_tree[i].c.pid;

		/* Go down one level. */
		max = cd_tree[i].n_down;
		cd_tree = m->cd_tree + cd_tree[i].c.down;
		++level;
	}
	/* We probably shouldn't get here. */
	return BAD_S3PID;
}

int32
bin_mdef_phone_str (bin_mdef_t *m,
		    s3pid_t pid,
		    char *buf)
{
	char *wpos_name;
    
	assert (m);
	assert ((pid >= 0) && (pid < m->n_phone));
	wpos_name = WPOS_NAME;
    
	buf[0] = '\0';
	if (pid < m->n_ciphone)
		sprintf (buf, "%s", bin_mdef_ciphone_str(m, (s3cipid_t) pid));
	else {
		sprintf (buf, "%s(%s,%s)%c",
			 bin_mdef_ciphone_str(m, m->phone[pid].info.cd.ctx[0]),
			 bin_mdef_ciphone_str(m, m->phone[pid].info.cd.ctx[1]),
			 bin_mdef_ciphone_str(m, m->phone[pid].info.cd.ctx[2]),
			 wpos_name[m->phone[pid].info.cd.wpos]);
	}
	return 0;
}
