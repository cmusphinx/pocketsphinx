/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2013 Carnegie Mellon University.  All rights
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


%define DOCSTRING
"This documentation was automatically generated using original comments in
Doxygen format. As some C types and data structures cannot be directly mapped
into Python types, some non-trivial type conversion could have place.
Basically a type is replaced with another one that has the closest match, and
sometimes one argument of generated function comprises several arguments of the
original function (usually two).

Functions having error code as the return value and returning effective
value in one of its arguments are transformed so that the effective value is
returned in a regular fashion and run-time exception is being thrown in case of
negative error code."
%enddef

#if SWIGJAVA
%module PocketSphinx
#else
%module(docstring=DOCSTRING) pocketsphinx
#endif

%feature("autodoc", "1");

%include typemaps.i
%include iterators.i
%import sphinxbase.i

#if SWIGPYTHON
%include pybuffer.i
#endif

%{
typedef cmd_ln_t Config;
typedef feat_t Feature;
typedef fe_t FrontEnd;
typedef fsg_model_t FsgModel;
typedef logmath_t LogMath;
typedef ngram_model_t NGramModel;
typedef ngram_model_t NGramModelSet;
%}

#if SWIGPYTHON
%begin %{
#include <Python.h>
%}
#endif

#if SWIGJAVASCRIPT
%begin %{
#include <v8.h>
#include <node.h>
#include <node_buffer.h>
%}
#endif


%begin %{

#ifndef __cplusplus
typedef int bool;
#define true 1
#define false 0
#endif

#include <pocketsphinx.h>

typedef ps_decoder_t Decoder;
typedef ps_decoder_t SegmentList;
typedef ps_decoder_t NBestList;
typedef ps_lattice_t Lattice;
%}


%inline %{

// TODO: make private with %immutable
typedef struct {
    char *hypstr;
    int best_score;
    int prob;
} Hypothesis;

typedef struct {
    char *word;
    int32 ascore;
    int32 lscore;
    int32 lback;
    int32 prob;
    int start_frame;
    int end_frame;
} Segment;

typedef struct {
    char *hypstr;
    int32 score;
} NBest;

%}

%nodefaultctor SegmentList;
%nodefaultctor NBestList;

sb_iterator(Segment, ps_seg, Segment)
sb_iterator(NBest, ps_nbest, NBest)
sb_iterable(SegmentList, Segment, ps_seg, ps_seg_iter, Segment)
sb_iterable(NBestList, NBest, ps_nbest, ps_nbest, NBest)

typedef struct {} Decoder;
typedef struct {} Lattice;
typedef struct {} NBestList;
typedef struct {} SegmentList;


#ifdef HAS_DOC
%include pydoc.i
#endif

%extend Hypothesis {
    Hypothesis(char const *hypstr, int best_score, int prob) {
        Hypothesis *h = (Hypothesis *)ckd_malloc(sizeof *h);
        if (hypstr)
            h->hypstr = ckd_salloc(hypstr);
        else
    	    h->hypstr = NULL;
        h->best_score = best_score;
        h->prob = prob;
        return h;  
    }

    ~Hypothesis() {
        if ($self->hypstr)
    	    ckd_free($self->hypstr);
        ckd_free($self);
    }
}

%extend Segment {

    static Segment* fromIter(ps_seg_t *itor) {
	Segment *seg;
	if (!itor)
	    return NULL;
	seg = (Segment *)ckd_malloc(sizeof(Segment));
	seg->word = ckd_salloc(ps_seg_word(itor));
	seg->prob = ps_seg_prob(itor, &(seg->ascore), &(seg->lscore), &(seg->lback));
	ps_seg_frames(itor, &seg->start_frame, &seg->end_frame);
	return seg;
    }
    ~Segment() {
	ckd_free($self->word);
	ckd_free($self);
    }
}

%extend NBest {

    static NBest* fromIter(ps_nbest_t *itor) {
	NBest *nbest;
	if (!itor)
	    return NULL;
	nbest = (NBest *)ckd_malloc(sizeof(NBest));
	nbest->hypstr = ckd_salloc(ps_nbest_hyp(itor, &(nbest->score)));
	return nbest;
    }
    
    %newobject hyp;
    Hypothesis* hyp() {
        return $self->hypstr ? new_Hypothesis($self->hypstr, $self->score, 0) : NULL;
    }
    
    ~NBest() {
	ckd_free($self->hypstr);
	ckd_free($self);
    }
}

%include ps_decoder.i
%include ps_lattice.i
