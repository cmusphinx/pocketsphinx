# Copyright (c) 2008 Carnegie Mellon University. All rights
# reserved.
#
# You may copy, modify, and distribute this code under the same terms
# as PocketSphinx or Python, at your convenience, as long as this
# notice is not removed.
#
# Author: David Huggins-Daines <dhuggins@cs.cmu.edu>

# Import SphinxBase C types
from sphinxbase cimport arg_t, cmd_ln_t, ngram_model_t, fsg_model_t, logmath_t

# Import SphinxBase extention types
from sphinxbase cimport NGramModel

# Finally, import this for SphinxBase functions (since there are too many to list)
cimport sphinxbase as sb

# System and Python headers we need
cdef extern from "stdio.h":
    ctypedef struct FILE # oh dear...

cdef extern from "Python.h":
    FILE *PyFile_AsFile(object p)
    int PyString_AsStringAndSize(object p, char **buf, Py_ssize_t *len) except -1

# Don't rely on having PyGTK actually installed
cdef extern from "bogus_pygobject.h":
    ctypedef struct PyGBoxed:
        void *boxed
    ctypedef struct PyGPointer:
        void *pointer

# PocketSphinx declarations
cdef extern from "fsg_set.h":
    ctypedef struct fsg_set_t
    ctypedef struct fsg_set_iter_t

cdef extern from "ps_lattice.h":
    ctypedef struct ps_lattice_t
    ctypedef struct ps_latnode_t
    ctypedef struct ps_latnode_iter_t
    ctypedef struct ps_latlink_t
    ctypedef struct ps_latlink_iter_t
    ctypedef struct ps_decoder_t

    ps_lattice_t *ps_lattice_read(ps_decoder_t *ps, char *file)
    ps_lattice_t *ps_lattice_retain(ps_lattice_t *dag)
    int ps_lattice_free(ps_lattice_t *dag)
    int ps_lattice_write(ps_lattice_t *dag, char *filename)
    logmath_t *ps_lattice_get_logmath(ps_lattice_t *dag)
    ps_latnode_iter_t *ps_latnode_iter(ps_lattice_t *dag)
    ps_latnode_iter_t *ps_latnode_iter_next(ps_latnode_iter_t *itor)
    void ps_latnode_iter_free(ps_latnode_iter_t *itor)
    ps_latnode_t *ps_latnode_iter_node(ps_latnode_iter_t *itor)
    int ps_latnode_times(ps_latnode_t *node, short *out_fef, short *out_lef)
    char *ps_latnode_word(ps_lattice_t *dag, ps_latnode_t *node)
    char *ps_latnode_baseword(ps_lattice_t *dag, ps_latnode_t *node)
    ps_latlink_iter_t *ps_latnode_exits(ps_latnode_t *node)
    ps_latlink_iter_t *ps_latnode_entries(ps_latnode_t *node)
    int ps_latnode_prob(ps_lattice_t *dag, ps_latnode_t *node, ps_latlink_t **out_link)
    ps_latlink_iter_t *ps_latlink_iter_next(ps_latlink_iter_t *itor)
    void ps_latlink_iter_free(ps_latlink_iter_t *itor)
    ps_latlink_t *ps_latlink_iter_link(ps_latlink_iter_t *itor)
    int ps_latlink_times(ps_latlink_t *link, short *out_sf)
    ps_latnode_t *ps_latlink_nodes(ps_latlink_t *link, ps_latnode_t **out_src)
    char *ps_latlink_word(ps_lattice_t *dag, ps_latlink_t *link)
    char *ps_latlink_baseword(ps_lattice_t *dag, ps_latlink_t *link)
    int ps_latlink_prob(ps_lattice_t *dag, ps_latlink_t *link, int *out_ascr)
    ps_latlink_t *ps_latlink_pred(ps_latlink_t *link)
    void ps_lattice_link(ps_lattice_t *dag, ps_latnode_t *src, ps_latnode_t *dest,
                         int score, int ef)
    ps_latlink_t *ps_lattice_traverse_edges(ps_lattice_t *dag, ps_latnode_t *start,
                                            ps_latnode_t *end)
    ps_latlink_t *ps_lattice_traverse_next(ps_lattice_t *dag, ps_latnode_t *end)
    ps_latlink_t *ps_lattice_reverse_edges(ps_lattice_t *dag, ps_latnode_t *start,
                                           ps_latnode_t *end)
    ps_latlink_t *ps_lattice_reverse_next(ps_lattice_t *dag, ps_latnode_t *start)
    ps_latlink_t *ps_lattice_bestpath(ps_lattice_t *dag, ngram_model_t *lmset,
                                      float lwf, float ascale)
    int ps_lattice_posterior(ps_lattice_t *dag, ngram_model_t *lmset, float ascale)
    int ps_lattice_n_frames(ps_lattice_t *dag)

cdef extern from "pocketsphinx.h":
    ctypedef struct ps_decoder_t
    ctypedef struct ps_nbest_t
    ctypedef struct ps_seg_t
    ctypedef int size_t
    ctypedef int int32

    ps_decoder_t *ps_init(cmd_ln_t *config)
    int ps_reinit(ps_decoder_t *ps, cmd_ln_t *config)
    ps_decoder_t *ps_retain(ps_decoder_t *ps)
    void ps_free(ps_decoder_t *ps)
    arg_t *ps_args()
    cmd_ln_t *ps_get_config(ps_decoder_t *ps)
    logmath_t *ps_get_logmath(ps_decoder_t *ps)
    ngram_model_t *ps_get_lmset(ps_decoder_t *ps)
    ngram_model_t *ps_update_lmset(ps_decoder_t *ps, ngram_model_t *lmset)
    fsg_set_t *ps_get_fsgset(ps_decoder_t *ps)
    fsg_set_t *ps_update_fsgset(ps_decoder_t *ps)
    int ps_load_dict(ps_decoder_t *ps, char *dictfile,
                     char *fdictfile, char *format)
    int ps_save_dict(ps_decoder_t *ps, char *dictfile, char *format)
    int ps_add_word(ps_decoder_t *ps, char *word, char *phones, int update)
    int ps_decode_raw(ps_decoder_t *ps, FILE *rawfh,
                      char *uttid, long maxsamps)
    int ps_start_utt(ps_decoder_t *ps, char *uttid)
    int ps_process_raw(ps_decoder_t *ps, char *data, size_t n_samples,
                       int no_search, int full_utt)
    int ps_end_utt(ps_decoder_t *ps)
    char *ps_get_hyp(ps_decoder_t *ps, int32 *out_best_score, char **out_uttid)
    ps_lattice_t *ps_get_lattice(ps_decoder_t *ps)
    ps_seg_t *ps_seg_iter(ps_decoder_t *ps, int32 *out_best_score)
    ps_seg_t *ps_seg_next(ps_seg_t *seg)
    char *ps_seg_word(ps_seg_t *seg)
    void ps_seg_frames(ps_seg_t *seg, int *out_sf, int *out_ef)
    int ps_seg_prob(ps_seg_t *seg, int32 *out_ascr, int32 *out_lscr, int32 *out_lback)
    void ps_seg_free(ps_seg_t *seg)
    ps_nbest_t *ps_nbest(ps_decoder_t *ps, int sf, int ef, char *ctx1, char *ctx2)
    ps_nbest_t *ps_nbest_next(ps_nbest_t *nbest)
    char *ps_nbest_hyp(ps_nbest_t *nbest, int32 *out_score)
    ps_seg_t *ps_nbest_seg(ps_nbest_t *nbest, int32 *out_score)
    void ps_nbest_free(ps_nbest_t *nbest)
    void ps_get_utt_time(ps_decoder_t *ps, double *out_nspeech,
                         double *out_ncpu, double *out_nwall)
    void ps_get_all_time(ps_decoder_t *ps, double *out_nspeech,
                         double *out_ncpu, double *out_nwall)

# Now, our extension classes
cdef class Decoder:
    cdef ps_decoder_t *ps
    cdef char **argv
    cdef int argc
    cdef set_boxed(Decoder self, box)

cdef class Lattice:
    cdef ps_lattice_t *dag
    cdef set_dag(Lattice self, ps_lattice_t *dag)
    cdef set_boxed(Lattice self, box)
    cdef readonly n_frames

cdef class LatLink:
    cdef ps_latlink_t *link
    cdef ps_lattice_t *dag # FIXME: This may or may not cause memory leaks?
    cdef readonly char *word, *baseword
    cdef readonly int sf, ef
    cdef readonly double prob
    cdef set_link(LatLink self, ps_lattice_t *dag, ps_latlink_t *link)

cdef class LatLinkIterator:
    cdef ps_lattice_t *dag
    cdef ps_latlink_iter_t *itor
    cdef int first_link

cdef class LatNode:
    cdef ps_latnode_t *node
    cdef ps_lattice_t *dag # FIXME: This may or may not cause memory leaks?
    cdef readonly char *word, *baseword
    cdef readonly int sf, fef, lef
    cdef readonly double prob
    cdef readonly LatLink best_exit
    cdef set_node(LatNode self, ps_lattice_t *dag, ps_latnode_t *node)

cdef class LatNodeIterator:
    cdef ps_lattice_t *dag
    cdef ps_latnode_iter_t *itor
    cdef int first_node
    cdef int start, end
