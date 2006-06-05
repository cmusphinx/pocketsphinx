/* ====================================================================
 * Copyright (c) 1999-2001 Carnegie Mellon University.  All rights
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

/* 
 *------------------------------------------------------------*
 * DESCRIPTION
 *	Read tied distribution hmms on behalf of fbs.
 *------------------------------------------------------------*
 * HISTORY
 *
 * $Log: hmm_tied_r.c,v $
 * Revision 1.1.1.1  2006/05/23 18:44:59  dhuggins
 * re-importation
 *
 * Revision 1.15  2005/09/01 21:09:54  dhdfu
 * Really, actually, truly consolidate byteswapping operations into
 * byteorder.h.  Where unconditional byteswapping is needed, SWAP_INT32()
 * and SWAP_INT16() are to be used.  The WORDS_BIGENDIAN macro from
 * autoconf controls the functioning of the conditional swap macros
 * (SWAP_?[LW]) whose names and semantics have been regularized.
 * Private, adhoc macros have been removed.
 *
 * Revision 1.14  2004/12/10 16:48:56  rkm
 * Added continuous density acoustic model handling
 *
 * 
 * 23-Jan-01    H J Fox (hjf@cs.brown.edu) at Brown University
 *              Hacked to run under Solaris 8 - flipped the byte swap
 *              condition.
 * 
 * Revision 1.4  2001/01/25 19:36:29  lenzo
 * Fixing some memory leaks
 *
 * Revision 1.3  2000/12/12 23:01:42  lenzo
 * Rationalizing libs and names some more.  Split a/d and fe libs out.
 *
 * Revision 1.2  2000/12/05 01:45:12  lenzo
 * Restructuring, hear rationalization, warning removal, ANSIfy
 *
 * Revision 1.1.1.1  2000/01/28 22:08:50  lenzo
 * Initial import of sphinx2
 *
 * Revision 8.4  94/05/10  10:46:37  rkm
 * Added original .map file timestamp info to map dump file.
 * 
 * Revision 8.3  94/04/22  13:51:27  rkm
 * *** empty log message ***
 * 
 * 14-Apr-94  M K Ravishankar (rkm@cs) at Carnegie-Mellon University
 * 	Added map-file dump and load facility for faster startup.
 * 
 *  2-Nov-91  Fil Alleva (faa) at Carnegie-Mellon University
 *	Massive changes for fbs.
 *
 * 16-Mar-87 Hsiao-wuen Hon (Hon)  at Carnegie Mellon University.
 *	Initial version.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef WIN32
#include <fcntl.h>
#else
#include <sys/file.h>
#endif

#include "s2types.h"
#include "CM_macros.h"
#include "basic_types.h"
#include "list.h"
#include "hash.h"
#include "phone.h"
#include "search_const.h"
#include "msd.h"
#include "magic.h"
#include "log.h"
#include "cviterbi4.h"
#include "smmap4f.h"
#include "dict.h"
#include "lmclass.h"
#include "lm_3g.h"
#include "kb.h"
#include "hmm_tied_r.h"
#include "err.h"
#include "s2io.h"
#include "byteorder.h"
#include "bio.h"
#include "ckd_alloc.h"
#include "vector.h"

#define QUIT(x)		{fprintf x; exit(-1);}

#define MGAU_MIXW_VERSION	"1.0"	/* Sphinx-3 file format version for mixw */
#define MAX_ALPHABET		256

#define HAS_ARC(fs,ts,S)	S->topo[((fs * S->stateCnt) + ts) / 32] & \
				(1 << ((fs * S->stateCnt) + ts) % 32)

int32 totalDists;		/* Total number of distributions */
int32 *numDists;		/* Number of senones for each of the CI
				 * and WD base phone units.
				 */
int32 *numDPDists;		/* Number of DiPhone senones. These are
				 * generated from the appropriate cd units.
				 */
static int32 **distMap;		/* Distribution map
				 */
static int32 *ssIdMap;		/* Senone sequence Id Map.
				 * ssIdMap[phone_id] == the senone sequence id.
				 */
static int32 numSSeq = 0;	/* Number of unique senone sequences
				 */

int32 *Out_Prob0;
int32 *Out_Prob1;
int32 *Out_Prob2;
int32 *Out_Prob3;
int32 *Out_Prob4;

OPDF_8BIT_T out_prob_8b[4];	/* for the 4 features */

/* Quantize senone PDFs to 8-bits for speed. */
static void quantize_pdfs(OPDF_8BIT_T *p, int32 n_comp, int32 n_sen)
{
    int32 f, s, c, pid, scr, qscr;
    
    E_INFO ("%s(%d): Quantizing senone PDFs to 8 bits\n", __FILE__, __LINE__);

    for (f = 0; f < 4; ++f) {
	for (c = 0; c < n_comp; c++) {
	    for (s = 0; s < n_sen; s++) {
		pid = p[f].id[c][s];
		scr = p[f].prob[c][pid];
		/* ** HACK!! ** hardwired threshold!!! */
		if (scr < -161900)
		    E_FATAL("**ERROR** Too low senone PDF value: %d\n", scr);
		qscr = (511-scr) >> 10;
		if ((qscr > 255) || (qscr < 0))
		    E_FATAL("scr(%d,%d,%d) = %d\n", f, c, s, scr);
		p[f].id[c][s] = (unsigned char) qscr;
	    }
	}
	free(p[f].prob);
	p[f].prob = NULL;
    }
}

/*
 * load_senone_dists_8bits:  Allocate space for and load precompiled senone prob
 * distributions file.
 * The precompiled file contains senone probs smoothed, normalized and compressed
 * according to the definition of type OPDF_8BIT_T.
 */
static int
load_senone_dists_8bits(OPDF_8BIT_T *p,		/* Output probs, clustered */
			int32 r, int32 c,	/* #rows, #cols */
			char const *file,	/* dumped probs */
			char const *dir)	/* original hmm directory */
{
    FILE *fp;
    char line[1000];
    size_t n;
    int32 i;
    int32 use_mmap, do_swap;
    size_t filesize, offset;
    int n_clust = 256; /* Number of clusters (if zero, we are just using
			* 8-bit quantized weights) */

    use_mmap = kb_get_mmap_flag();
    
    if ((fp = fopen (file, "rb")) == NULL)
	return -1;

    E_INFO("Loading senones from dump file %s\n", file);
    /* Read title size, title */
    n = fread_int32 (fp, 1, 999, "Title length", 0, &do_swap);
    if (do_swap && use_mmap) {
	E_ERROR("Dump file is byte-swapped, cannot use memory-mapping\n");
	use_mmap = 0;
    }
    if (fread (line, sizeof(char), n, fp) != n)
	E_FATAL("Cannot read title\n");
    if (line[n-1] != '\0')
	E_FATAL("Bad title in dump file\n");
    E_INFO("%s\n", line);
    
    /* Read header size, header */
    n = fread_int32 (fp, 1, 999, "Header length", 1, &do_swap);
    if (fread (line, sizeof(char), n, fp) != n)
	E_FATAL("Cannot read header\n");
    if (line[n-1] != '\0')
	E_FATAL("Bad header in dump file\n");
    
#if 0
    if (strcmp (line, dir) != 0) {
	E_WARN("HMM DIRECTORY NAME IN DUMPFILE HEADER: %s\n", line);
	E_INFOCONT("\tINCONSISTENT WITH -hmmdir ARGUMENT:    %s\n\n", dir);
    }
#endif

    /* Read other header strings until string length = 0 */
    for (;;) {
	n = fread_int32 (fp, 0, 999, "string length", 1, &do_swap);
	if (n == 0)
	    break;
	if (fread (line, sizeof(char), n, fp) != n)
	    E_FATAL("Cannot read header\n");
	/* Look for a cluster count, if present */
	if (!strncmp(line, "cluster_count ", strlen("cluster_count "))) {
	    n_clust = atoi(line + strlen("cluster_count "));
	}
    }
    
    /* Read #codewords, #pdfs */
    fread_int32 (fp, r, r, "#codewords", 1, &do_swap);
    c = fread_int32 (fp, c, (c+3)&~3, "#pdfs", 1, &do_swap);

    if (use_mmap) {
      if (n_clust) {
	  E_ERROR("Dump file is not pre-quantized, cannot use memory-mapping\n");
	  use_mmap = 0;
      }
      else
	  E_INFO("Using memory-mapped I/O for senones\n");
    }
    /* Verify alignment constraints for using mmap() */
    if ((c & 3) != 0) {
	/* This will cause us to run very slowly, so don't do it. */
	E_ERROR("Number of PDFs (%d) not padded to multiple of 4, will not use mmap()\n", c);
	use_mmap = 0;
    }
    offset = ftell(fp);
    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);
    fseek(fp, offset, SEEK_SET);
    if ((offset & 3) != 0) {
	E_ERROR("PDFs are not aligned to 4-byte boundary in file, will not use mmap()\n");
	use_mmap = 0;
    }

    /* Allocate memory for pdfs */
    if (use_mmap) {
	for (i = 0; i < 4; i++) {
	    /* Pointers into the mmap()ed 2d array */
	    /* Use the last index to hide a pointer to the entire file's memory */
	    if (n_clust)
		p[i].prob = (int32 **) CM_calloc (r, sizeof(int32 *));
	    p[i].id = (unsigned char **) CM_calloc (r+1, sizeof(unsigned char *));
	}

	p[0].id[r] = s2_mmap(file);
	for (n = 0; n < 4; n++) {
	    for (i = 0; i < r; i++) {
		if (n_clust) {
		    p[n].prob[i] = (int32 *)((caddr_t)p[0].id[r] + offset);
		    offset += n_clust*sizeof(int32);
		}
		p[n].id[i] = (char *)((caddr_t)p[0].id[r] + offset);
		offset += c;
	    }
	}
    }
    else {
	for (i = 0; i < 4; i++) {
	    if (n_clust)
		p[i].prob = (int32 **) CM_2dcalloc (r, n_clust, sizeof(int32));
	    p[i].id = (unsigned char **) CM_2dcalloc (r, c, sizeof(unsigned char));
	}
	/* Read pdf values and ids */
	for (n = 0; n < 4; n++) {
	    for (i = 0; i < r; i++) {
		if (n_clust) {
		    if (fread (p[n].prob[i], sizeof(int32), n_clust, fp) != n_clust)
			E_FATAL("fread failed\n");
		    if (do_swap) {
			int j;
			for (j = 0; j < n_clust; j++) {
			    SWAP_INT32(&p[n].prob[i][j]);
			}
		    }
		}
		if (fread (p[n].id[i], sizeof (unsigned char), c, fp) != (size_t) c)
		    E_FATAL("fread failed\n");
	    }
	}
    }
    if (n_clust && !use_mmap)
	quantize_pdfs(p, r, c);
    
    fclose (fp);
    return 0;
}

static void
dump_probs(int32 *p0, int32 *p1,
	   int32 *p2, int32 *p3,	/* pdfs, may be transposed */
	   int32 r, int32 c,		/* rows, cols */
	   char const *file,		/* output file */
	   char const *dir)		/* **ORIGINAL** HMM directory */
{
    FILE *fp;
    int32 i, k;
    static char const *title = "V6 Senone Probs, Smoothed, Normalized";
    
    E_INFO("Dumping HMMs to dump file %s\n", file);
    
    if ((fp = fopen (file, "wb")) == NULL) {
	E_ERROR("fopen(%s,wb) failed\n", file);
	return;
    }
    
    /* Write title size and title (directory name) */
    k = strlen (title)+1;	/* including trailing null-char */
    fwrite_int32 (fp, k);
    fwrite (title, sizeof(char), k, fp);
    
    /* Write header size and header (directory name) */
    k = strlen (dir)+1;		/* including trailing null-char */
    fwrite_int32 (fp, k);
    fwrite (dir, sizeof(char), k, fp);
    
    /* Write 0, terminating header strings */
    fwrite_int32 (fp, 0);
    
    /* Write #rows, #cols; this also indicates whether pdfs already transposed */
    fwrite_int32 (fp, r);
    fwrite_int32 (fp, c);

    /* Write pdfs */
    for (i = 0; i < r*c; i++)
	fwrite_int32 (fp, p0[i]);
    for (i = 0; i < r*c; i++)
	fwrite_int32 (fp, p1[i]);
    for (i = 0; i < r*c; i++)
	fwrite_int32 (fp, p2[i]);
    for (i = 0; i < r*c; i++)
	fwrite_int32 (fp, p3[i]);
    
    fclose (fp);
}

static void
dump_probs_8b(OPDF_8BIT_T *p,	/* pdfs, will be transposed */
	      int32 r, int32 c,		/* rows, cols */
	      char const *file,		/* output file */
	      char const *dir)		/* **ORIGINAL** HMM directory */
{
    FILE *fp;
    int32 f, i, k, aligned_c;
    static char const *title = "V6 Senone Probs, Smoothed, Normalized";
    static char const *clust_hdr = "cluster_count 0";
    
    E_INFO("Dumping quantized HMMs to dump file %s\n", file);
    
    if ((fp = fopen (file, "wb")) == NULL) {
	E_ERROR("fopen(%s,wb) failed\n", file);
	return;
    }
    
    /* Write title size and title (directory name) */
    k = strlen (title)+1;	/* including trailing null-char */
    fwrite_int32 (fp, k);
    fwrite (title, sizeof(char), k, fp);
    
    /* Write header size and header (directory name) */
    k = strlen (dir)+1;		/* including trailing null-char */
    fwrite_int32 (fp, k);
    fwrite (dir, sizeof(char), k, fp);

    /* Write cluster count = 0 to indicate this is pre-quantized */
    k = strlen (clust_hdr)+1;
    fwrite_int32 (fp, k);
    fwrite (clust_hdr, sizeof(char), k, fp);
    
    /* Pad it out for alignment purposes */
    k = ftell(fp) & 3;
    if (k > 0) {
	fwrite_int32(fp, 4-k);
	fwrite("!!!!", 1, 4-k, fp);
    }

    /* Write 0, terminating header strings */
    fwrite_int32 (fp, 0);
    
    /* Align the number of pdfs to a 4-byte boundary. */
    aligned_c = (c + 3) & ~3;

    /* Write #rows, #cols; this also indicates whether pdfs already transposed */
    fwrite_int32 (fp, r);
    fwrite_int32 (fp, aligned_c);

    /* Now write out the quantized senones. */
    for (f = 0; f < 4; ++f) {
	for (i = 0; i < r; i++) {
	    fwrite (p[f].id[i], sizeof(char), c, fp);
	    /* Pad them out for alignment purposes */
	    fwrite ("\0\0\0\0", 1, aligned_c - c, fp);
	}
    }
    
    fclose (fp);
}


void
read_dists_s3(char const *file_name, int32 numAlphabet,
	      double SmoothMin, int32 useCiDistsOnly)
{
    char **argname, **argval, *dumpfile;
    char eofchk;
    FILE *fp;
    int32 byteswap, chksum_present;
    uint32 chksum;
    int32 *out[4];
    float32 *pdf;
    int32 i, f, c, n;
    int32 n_sen, n_ci;
    int32 n_feat;
    int32 n_comp;
    int32 n_err;
    bin_mdef_t *mdef;

    mdef = kb_mdef();
    n_ci = bin_mdef_n_ciphone(mdef);
    dumpfile = kb_get_senprob_dump_file();
    if (dumpfile && kb_get_senprob_size() == 8) {
	    if (load_senone_dists_8bits(out_prob_8b, numAlphabet,
					totalDists, dumpfile, file_name) == 0)
		    return;
	else
	    E_INFO("No senone dump file found, will compress mixture weights on-line\n");
    }

    if (kb_get_senprob_size() == 8) {
	out_prob_8b[0].id = (unsigned char **) CM_2dcalloc(numAlphabet, totalDists,
							   sizeof(unsigned char));
	out_prob_8b[1].id = (unsigned char **) CM_2dcalloc(numAlphabet, totalDists,
							   sizeof(unsigned char));
	out_prob_8b[2].id = (unsigned char **) CM_2dcalloc(numAlphabet, totalDists,
							   sizeof(unsigned char));
	out_prob_8b[3].id = (unsigned char **) CM_2dcalloc(numAlphabet, totalDists,
							   sizeof(unsigned char));
    }
    else {
	out[0] = Out_Prob1 = (int32 *) CM_calloc (numAlphabet * totalDists, sizeof(int32));
	out[1] = Out_Prob2 = (int32 *) CM_calloc (numAlphabet * totalDists, sizeof(int32));
	out[2] = Out_Prob3 = (int32 *) CM_calloc (numAlphabet * totalDists, sizeof(int32));
	out[3] = Out_Prob4 = (int32 *) CM_calloc (numAlphabet * totalDists, sizeof(int32));
    }

    E_INFO("Reading mixture weights file '%s'\n", file_name);
    
    if (useCiDistsOnly)
	E_INFO ("ONLY using CI Senones\n");

    if ((fp = fopen (file_name, "rb")) == NULL)
      E_FATAL("fopen(%s,rb) failed\n", file_name);
    
    /* Read header, including argument-value info and 32-bit byteorder magic */
    if (bio_readhdr (fp, &argname, &argval, &byteswap) < 0)
	E_FATAL("bio_readhdr(%s) failed\n", file_name);
    
    /* Parse argument-value list */
    chksum_present = 0;
    for (i = 0; argname[i]; i++) {
	if (strcmp (argname[i], "version") == 0) {
	    if (strcmp(argval[i], MGAU_MIXW_VERSION) != 0)
		E_WARN("Version mismatch(%s): %s, expecting %s\n",
			file_name, argval[i], MGAU_MIXW_VERSION);
	} else if (strcmp (argname[i], "chksum0") == 0) {
	    chksum_present = 1;	/* Ignore the associated value */
	}
    }
    bio_hdrarg_free (argname, argval);
    argname = argval = NULL;

    chksum = 0;

    /* Read #senones, #features, #codewords, arraysize */
    if ((bio_fread (&n_sen, sizeof(int32), 1, fp, byteswap, &chksum) != 1) ||
	(bio_fread (&n_feat, sizeof(int32), 1, fp, byteswap, &chksum) != 1) ||
	(bio_fread (&n_comp, sizeof(int32), 1, fp, byteswap, &chksum) != 1) ||
	(bio_fread (&n, sizeof(int32), 1, fp, byteswap, &chksum) != 1)) {
	E_FATAL("bio_fread(%s) (arraysize) failed\n", file_name);
    }
    if (n_feat != 4)
	E_FATAL("#Features streams(%d) != 4\n", n_feat);
    if (n_sen != totalDists)
	    E_FATAL("%s: #senones(%d) doesn't match mdef: %d\n",
		    file_name, n_sen, totalDists);
    if (n != n_sen * n_feat * n_comp) {
	E_FATAL("%s: #float32s(%d) doesn't match header dimensions: %d x %d x %d\n",
		file_name, i, n_sen, n_feat, n_comp);
    }
    
    /* Temporary structure to read in floats before conversion to (int32) logs3 */
    pdf = (float32 *) ckd_calloc (n_comp, sizeof(float32));
    
    /* Read senone probs data, normalize, floor, convert to logs3, truncate to 8 bits */
    n_err = 0;
    for (i = 0; i < n_sen; i++) {
	if (useCiDistsOnly && i == n_ci)
	    break;
	for (f = 0; f < n_feat; f++) {
	    if (bio_fread((void *)pdf, sizeof(float32),
			  n_comp, fp, byteswap, &chksum) != n_comp) {
		E_FATAL("bio_fread(%s) (arraydata) failed\n", file_name);
	    }
	    
	    /* Normalize and floor */
	    if (vector_sum_norm (pdf, n_comp) <= 0.0)
		n_err++;
	    vector_floor (pdf, n_comp, SmoothMin);
	    vector_sum_norm (pdf, n_comp);

	    if (kb_get_senprob_size() == 8) {
		/* Convert to logs3, quantize, and transpose */
		for (c = 0; c < n_comp; c++) {
		    int32 qscr;

		    qscr = LOG(pdf[c]);
		    /* ** HACK!! ** hardwired threshold!!! */
		    if (qscr < -161900)
			E_FATAL("**ERROR** Too low senone PDF value: %d\n", qscr);
		    qscr = (511-qscr) >> 10;
		    if ((qscr > 255) || (qscr < 0))
			E_FATAL("scr(%d,%d,%d) = %d\n", f, c, i, qscr);
		    out_prob_8b[f].id[c][i] = qscr;
		}
	    }
	    else {
		/* Convert to logs3 and transpose */
		for (c = 0; c < n_comp; c++) {
		    out[f][c * totalDists + i] = LOG(pdf[c]);
		}
	    }
	}
    }
    if (n_err > 0)
	E_ERROR("Weight normalization failed for %d senones\n", n_err);

    ckd_free (pdf);

    if (chksum_present)
	bio_verify_chksum (fp, byteswap, chksum);
    
    if (fread (&eofchk, 1, 1, fp) == 1)
	E_FATAL("More data than expected in %s\n", file_name);

    fclose(fp);

    E_INFO("Read %d x %d x %d mixture weights\n", n_sen, n_feat, n_comp);

    if ((dumpfile = kb_get_senprob_dump_file()) != NULL) {
	if (kb_get_senprob_size() == 8)
	    dump_probs_8b(out_prob_8b, numAlphabet, totalDists, dumpfile, file_name);
	else
	    dump_probs(Out_Prob1, Out_Prob2, Out_Prob3, Out_Prob4,
		       numAlphabet, totalDists, dumpfile, file_name);
    }
}


void
remap (SMD *smdV)	/* smd pointer vector */
/*------------------------------------------------------------*
 * DESCRIPTION
 *	Remap the distributions in the smd's.
 */
{
    int32 		i, j;

    for (i = 0; i < numSSeq; i++) {
	for (j = 0; j < TRANS_CNT; j++) {
	    smdV[i].dist[j] = distMap[i][smdV[i].dist[j]];
  	}
    }
    free (distMap);
}

void
remap_mdef (SMD *smdV, bin_mdef_t *mdef)
{
    int32 		i, j;

    for (i = 0; i < bin_mdef_n_sseq(mdef); i++) {
	/* This looks like a hack, but it really isn't, because the
	 * distribution IDs in the mdef are global rather than
	 * per-CIphone, so they will always be sequential within a
	 * senone sequence.  */
	for (j = 0; j < TRANS_CNT; j++)
	    smdV[i].dist[j] = bin_mdef_sseq2sen(mdef,i,j/3);
    }
    free (distMap);
}

typedef struct {
    int32 name;
    int32 idx;
} ARC;



/* Call this only after entire kb loaded */
void hmm_tied_r_dumpssidlist ()
{
  SMD *models;
  FILE *dumpfp;
  int32 i, j;
  
  models = kb_get_models();
  numSSeq = hmm_num_sseq();
  if ((dumpfp = fopen("ssid_list.txt", "w")) != NULL) {
    for (i = 0; i < numSSeq; i++) {
      fprintf (dumpfp, "%6d\t", i);
      for (j = 0; j < 5; j++)
	fprintf (dumpfp, " %5d", models[i].dist[j*3]);
      fprintf (dumpfp, "\n");
    }
  }
  fclose (dumpfp);
}


#define MAX_MEMBERS 256

int32 sets[NUMDISTRTYPES][MAX_MEMBERS];
int32 set_size[NUMDISTRTYPES];

int32
hmm_num_sseq (void)
/*------------------------------------------------------------*
 * Return number of unique senone sequences.
 * If the number is 0 we call this a fatal error.
 */
{
    bin_mdef_t *mdef;

    if ((mdef = kb_mdef()) != NULL)
	return bin_mdef_n_sseq(mdef);

    if (numSSeq == 0) {
	E_FATAL ("numSSeq (number of senone sequences is 0\n");
    }
    return numSSeq;
}

int32
hmm_pid2sid (int32 pid)
/*------------------------------------------------------------*
 * Convert a phone id to a senone sequence id\
 */
{
    bin_mdef_t *mdef;

    if ((mdef = kb_mdef()) != NULL)
	    return bin_mdef_pid2ssid(mdef, pid);

    return ssIdMap[pid];
}


int32 *
hmm_get_psen ( void )
{
    return numDists;
}
