/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2004 Carnegie Mellon University.  All rights
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
/* uttproc.h: Internal uttproc functions */
#ifndef __UTTPROC_H__
#define __UTTPROC_H__

#include <feat.h>
#include <prim_type.h>

int32 uttproc_init(void);
int32 uttproc_end(void);

/* Set the feature computation object. */
void uttproc_set_feat(feat_t *fcb);

/* The global feature computation object. */
extern feat_t *fcb;

/* Build and store an utterance ID (called if none given) */
char *build_uttid(char const *utt);

/* Parses line of control file (with optional start and end frames and uttid) */
int32 uttproc_parse_ctlfile_entry(char *line,
				  char *filename, int32 * sf, int32 * ef,
				  char *idspec);

/* Returns #frames converted to feature vectors; -1 if error */
int32 uttproc_file2feat(const char *utt, int32 sf, int32 ef, int32 nosearch);

/* Returns a pointer to the internal feature buffer */
int32 uttproc_get_featbuf(mfcc_t ****feat);

/* Open an audio file. */
FILE * adcfile_open(char const *utt);
/* Read from audio file. */
int32 adc_file_read(FILE *uttfp, int16 * buf, int32 max);

/*------------------------------------------------------------------------*
 * Compute an elapsed time from two timeval structs
 */
#if defined(_WIN32) && !defined(GNUWINCE) && !defined(CYGWIN)
#include <windows.h>
double win32_cputime(FILETIME * st, FILETIME * et);
#else
#include <time.h>
double MakeSeconds(struct timeval const *s, struct timeval const *e);
#endif


#endif /* __UTTPROC_H__ */
