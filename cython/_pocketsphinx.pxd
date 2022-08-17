# cython: embedsignature=True
# Copyright (c) 2008-2020 Carnegie Mellon University. All rights
# reserved.
#
# You may copy, modify, and distribute this code under the same terms
# as PocketSphinx or Python, at your convenience, as long as this
# notice is not removed.
#
# Author: David Huggins-Daines <dhdaines@gmail.com>


cdef extern from "sphinxbase/err.h":
    cdef enum err_e:
        ERR_DEBUG,
        ERR_INFO,
        ERR_WARN,
        ERR_ERROR,
        ERR_FATAL,
        ERR_MAX
    ctypedef err_e err_lvl_t
    ctypedef void (*err_cb_f)(void* user_data, err_lvl_t lvl, const char *msg)
    void err_set_callback(err_cb_f callback, void *user_data)


cdef extern from "sphinxbase/logmath.h":
    ctypedef struct logmath_t:
        pass
    
    logmath_t *logmath_init(double base, int shift, int use_table)
    logmath_t *logmath_retain(logmath_t *lmath)
    int logmath_free(logmath_t *lmath)

    int logmath_log(logmath_t *lmath, double p)
    double logmath_exp(logmath_t *lmath, int p)

    int logmath_ln_to_log(logmath_t *lmath, double p)
    double logmath_log_to_ln(logmath_t *lmath, int p)

    int logmath_log10_to_log(logmath_t *lmath, double p)
    double logmath_log_to_log10(logmath_t *lmath, int p)

    int logmath_add(logmath_t *lmath, int p, int q)

    int logmath_get_zero(logmath_t *lmath)

cdef extern from "sphinxbase/fe.h":
    ctypedef struct fe_t:
        pass
    int fe_get_output_size(fe_t *fe)

cdef extern from "sphinxbase/hash_table.h":
    ctypedef struct hash_table_t:
        pass
    ctypedef struct hash_entry_t:
        const char *key
    ctypedef struct hash_iter_t:
        hash_entry_t *ent
    hash_iter_t *hash_table_iter(hash_table_t *h)
    hash_iter_t *hash_table_iter_next(hash_iter_t *h)
    const char *hash_entry_key(hash_entry_t *ent)


cdef extern from "sphinxbase/ckd_alloc.h":
    void *ckd_alloc_2d_ptr(size_t d1,
                           size_t d2,
                           void *store,
                           size_t elem_size)
    void ckd_free(void *ptr)


cdef extern from "sphinxbase/cmd_ln.h":
    ctypedef struct arg_t:
        const char *name
        int type
        const char *deflt
        const char *doc
    ctypedef struct cmd_ln_t:
        hash_table_t *ht
        const arg_t *defn
    cdef enum:
        ARG_REQUIRED,
        ARG_INTEGER,
        ARG_FLOATING,
        ARG_STRING,
        ARG_BOOLEAN,
        REQARG_INTEGER,
        REQARG_FLOATING,
        REQARG_STRING,
        REQARG_BOOLEAN
    ctypedef struct cmd_ln_val_t:
        int type
        
    cmd_ln_t *cmd_ln_parse_r(cmd_ln_t *inout_cmdln, arg_t *defn,
                             int argc, char **argv, int strict)
    cmd_ln_t *cmd_ln_parse_file_r(cmd_ln_t *inout_cmdln, arg_t *defn,
                                  const char *path, int strict)
    cmd_ln_t *cmd_ln_retain(cmd_ln_t *cmdln)
    int cmd_ln_free_r(cmd_ln_t *cmdln)

    double cmd_ln_float_r(cmd_ln_t *cmdln, const char *name)
    int cmd_ln_type_r(cmd_ln_t *cmdln, const char *name)
    long cmd_ln_int_r(cmd_ln_t *cmdln, const char *name)
    const char *cmd_ln_str_r(cmd_ln_t *cmdln, const char *name)
    void cmd_ln_set_str_r(cmd_ln_t *cmdln, const char *name, const char *str)
    void cmd_ln_set_str_extra_r(cmd_ln_t *cmdln, const char *name, const char *str)
    void cmd_ln_set_int_r(cmd_ln_t *cmdln, const char *name, long iv)
    void cmd_ln_set_float_r(cmd_ln_t *cmdln, const char *name, double fv)
    cmd_ln_val_t *cmd_ln_access_r(cmd_ln_t *cmdln, const char *name)

    int cmd_ln_exists_r(cmd_ln_t *cmdln, const char *name)


cdef extern from "sphinxbase/ngram_model.h":
    cdef enum ngram_file_type_e:
        NGRAM_INVALID,
        NGRAM_AUTO,
        NGRAM_ARPA,
        NGRAM_BIN
    ctypedef ngram_file_type_e ngram_file_type_t
    cdef enum ngram_case_e:
        NGRAM_UPPER,
        NGRAM_LOWER
    ctypedef ngram_case_e ngram_case_t
    ctypedef struct ngram_model_t:
        pass
    ctypedef struct ngram_iter_t:
        pass
    ctypedef struct ngram_model_set_iter_t:
        pass
    ngram_model_t *ngram_model_read(cmd_ln_t *config,
				    const char *file_name,
                                    ngram_file_type_t file_type,
				    logmath_t *lmath)
    int ngram_model_write(ngram_model_t *model, const char *file_name,
		          ngram_file_type_t format)
    ngram_file_type_t ngram_file_name_to_type(const char *file_name)
    ngram_file_type_t ngram_str_to_type(const char *str_name)
    const char *ngram_type_to_str(int type)
    ngram_model_t *ngram_model_retain(ngram_model_t *model)
    int ngram_model_free(ngram_model_t *model)
    int ngram_model_casefold(ngram_model_t *model, int kase)
    int ngram_model_apply_weights(ngram_model_t *model,
                                  float lw, float wip)
    float ngram_model_get_weights(ngram_model_t *model, int *out_log_wip)
    int ngram_score(ngram_model_t *model, const char *word, ...)
    int ngram_tg_score(ngram_model_t *model,
                         int w3, int w2, int w1,
                         int *n_used)
    int ngram_bg_score(ngram_model_t *model,
                       int w2, int w1,
                       int *n_used)
    int ngram_ng_score(ngram_model_t *model, int wid, int *history,
                       int n_hist, int *n_used)
    int ngram_probv(ngram_model_t *model, const char *word, ...)
    int ngram_prob(ngram_model_t *model, const char* const *words, int n)
    int ngram_ng_prob(ngram_model_t *model, int wid, int *history,
                      int n_hist, int *n_used)
    int ngram_score_to_prob(ngram_model_t *model, int score)
    int ngram_wid(ngram_model_t *model, const char *word)
    const char *ngram_word(ngram_model_t *model, int wid)
    int ngram_unknown_wid(ngram_model_t *model)
    int ngram_zero(ngram_model_t *model)
    int ngram_model_get_size(ngram_model_t *model)
    const unsigned int *ngram_model_get_counts(ngram_model_t *model)
    ngram_iter_t *ngram_model_mgrams(ngram_model_t *model, int m)
    ngram_iter_t *ngram_iter(ngram_model_t *model, const char *word, ...)
    ngram_iter_t *ngram_ng_iter(ngram_model_t *model, int wid, int *history, int n_hist)
    const int *ngram_iter_get(ngram_iter_t *itor,
                              int *out_score,
                              int *out_bowt)
    ngram_iter_t *ngram_iter_successors(ngram_iter_t *itor)
    ngram_iter_t *ngram_iter_next(ngram_iter_t *itor)
    void ngram_iter_free(ngram_iter_t *itor)
    int ngram_model_add_word(ngram_model_t *model,
                             const char *word, float weight)
    int ngram_model_read_classdef(ngram_model_t *model,
                                  const char *file_name)
    int ngram_model_add_class(ngram_model_t *model,
                              const char *classname,
                              float classweight,
                              char **words,
                              const float *weights,
                              int n_words)
    int ngram_model_add_class_word(ngram_model_t *model,
                                   const char *classname,
                                   const char *word,
                                   float weight)
    ngram_model_t *ngram_model_set_init(cmd_ln_t *config,
                                        ngram_model_t **models,
                                        char **names,
                                        const float *weights,
                                        int n_models)
    ngram_model_t *ngram_model_set_read(cmd_ln_t *config,
                                        const char *lmctlfile,
                                        logmath_t *lmath)
    int ngram_model_set_count(ngram_model_t *set)
    ngram_model_set_iter_t *ngram_model_set_iter(ngram_model_t *set)
    ngram_model_set_iter_t *ngram_model_set_iter_next(ngram_model_set_iter_t *itor)
    void ngram_model_set_iter_free(ngram_model_set_iter_t *itor)
    ngram_model_t *ngram_model_set_iter_model(ngram_model_set_iter_t *itor,
                                              const char **lmname)
    ngram_model_t *ngram_model_set_select(ngram_model_t *set,
                                          const char *name)
    ngram_model_t *ngram_model_set_lookup(ngram_model_t *set,
                                          const char *name)
    const char *ngram_model_set_current(ngram_model_t *set)
    ngram_model_t *ngram_model_set_interp(ngram_model_t *set,
                                          const char **names,
                                          const float *weights)
    ngram_model_t *ngram_model_set_add(ngram_model_t *set,
                                       ngram_model_t *model,
                                       const char *name,
                                       float weight,
                                       int reuse_widmap)
    ngram_model_t *ngram_model_set_remove(ngram_model_t *set,
                                          const char *name,
                                          int reuse_widmap)
    void ngram_model_set_map_words(ngram_model_t *set,
                                   const char **words,
                                   int n_words)
    int ngram_model_set_current_wid(ngram_model_t *set,
                                    int set_wid)
    int ngram_model_set_known_wid(ngram_model_t *set, int set_wid)
    void ngram_model_flush(ngram_model_t *lm)


cdef extern from "sphinxbase/fsg_model.h":
    ctypedef struct fsg_model_t:
        int start_state
        int final_state

    fsg_model_t *fsg_model_init(const char *name, logmath_t *lmath,
                                float lw, int n_state)
    fsg_model_t *fsg_model_readfile(const char *file, logmath_t *lmath,
                                    float lw)
    const char *fsg_model_name(fsg_model_t *fsg)
    fsg_model_t *fsg_model_retain(fsg_model_t *fsg)
    int fsg_model_free(fsg_model_t *fsg)

    int fsg_model_word_add(fsg_model_t *fsg, const char *word)
    int fsg_model_word_id(fsg_model_t *fsg, const char *word)
    const char *fsg_model_word_str(fsg_model_t *fsg, int wid)
    void fsg_model_trans_add(fsg_model_t * fsg,
                             int source, int dest, int logp, int wid)
    int fsg_model_null_trans_add(fsg_model_t * fsg, int source, int dest,
                                 int logp)
    int fsg_model_tag_trans_add(fsg_model_t * fsg, int source, int dest,
                                int logp, int wid)
    int fsg_model_add_silence(fsg_model_t * fsg, const char *silword,
                              int state, float silprob)
    int fsg_model_add_alt(fsg_model_t * fsg, const char *baseword,
                          const char *altword)
    void fsg_model_writefile(fsg_model_t *fsg, const char *file)
    void fsg_model_writefile_fsm(fsg_model_t *fsg, const char *file)
    void fsg_model_writefile_symtab(fsg_model_t *fsg, const char *file)


cdef extern from "sphinxbase/jsgf.h":
    ctypedef struct jsgf_t:
        pass
    ctypedef struct jsgf_rule_t:
        pass
    fsg_model_t *jsgf_read_file(const char *name, logmath_t *lmath,
                                float lw)
    jsgf_t *jsgf_parse_file(const char *string, jsgf_t *parent)
    jsgf_t *jsgf_parse_string(const char *string, jsgf_t *parent)
    const char *jsgf_grammar_name(jsgf_t *jsgf)
    void jsgf_grammar_free(jsgf_t *jsgf)
    jsgf_rule_t *jsgf_get_rule(jsgf_t *grammar, const char *name)
    jsgf_rule_t *jsgf_get_public_rule(jsgf_t *grammar)
    const char *jsgf_rule_name(jsgf_rule_t *rule)
    int jsgf_rule_public(jsgf_rule_t *rule)
    fsg_model_t *jsgf_build_fsg(jsgf_t *grammar, jsgf_rule_t *rule,
                                logmath_t *lmath, float lw)


cdef extern from "pocketsphinx/ps_lattice.h":
    ctypedef struct ps_lattice_t:
        pass
    ctypedef struct ps_latnode_t:
        pass
    ctypedef struct ps_latnode_iter_t:
        pass
    ctypedef struct ps_latlink_t:
        pass
    ctypedef struct ps_latlink_iter_t:
        pass

    ps_lattice_t *ps_lattice_read(ps_decoder_t *ps,
                                  const char *file)
    ps_lattice_t *ps_lattice_retain(ps_lattice_t *dag)
    int ps_lattice_free(ps_lattice_t *dag)
    int ps_lattice_write(ps_lattice_t *dag, const char *filename)
    int ps_lattice_write_htk(ps_lattice_t *dag, const char *filename)
    logmath_t *ps_lattice_get_logmath(ps_lattice_t *dag)
    ps_latnode_iter_t *ps_latnode_iter(ps_lattice_t *dag)
    ps_latnode_iter_t *ps_latnode_iter_next(ps_latnode_iter_t *itor)
    void ps_latnode_iter_free(ps_latnode_iter_t *itor)
    ps_latnode_t *ps_latnode_iter_node(ps_latnode_iter_t *itor)
    int ps_latnode_times(ps_latnode_t *node, short *out_fef, short *out_lef)
    const char *ps_latnode_word(ps_lattice_t *dag, ps_latnode_t *node)
    const char *ps_latnode_baseword(ps_lattice_t *dag, ps_latnode_t *node)
    ps_latlink_iter_t *ps_latnode_exits(ps_latnode_t *node)
    ps_latlink_iter_t *ps_latnode_entries(ps_latnode_t *node)
    int ps_latnode_prob(ps_lattice_t *dag, ps_latnode_t *node,
                        ps_latlink_t **out_link)
    ps_latlink_iter_t *ps_latlink_iter_next(ps_latlink_iter_t *itor)
    void ps_latlink_iter_free(ps_latlink_iter_t *itor)
    ps_latlink_t *ps_latlink_iter_link(ps_latlink_iter_t *itor)
    int ps_latlink_times(ps_latlink_t *link, short *out_sf)
    ps_latnode_t *ps_latlink_nodes(ps_latlink_t *link, ps_latnode_t **out_src)
    const char *ps_latlink_word(ps_lattice_t *dag, ps_latlink_t *link)
    const char *ps_latlink_baseword(ps_lattice_t *dag, ps_latlink_t *link)
    ps_latlink_t *ps_latlink_pred(ps_latlink_t *link)
    int ps_latlink_prob(ps_lattice_t *dag, ps_latlink_t *link, int *out_ascr)
    void ps_lattice_link(ps_lattice_t *dag, ps_latnode_t *_from, ps_latnode_t *to,
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
    int ps_lattice_posterior_prune(ps_lattice_t *dag, int beam)
    int ps_lattice_n_frames(ps_lattice_t *dag)


cdef extern from "pocketsphinx.h":
    ctypedef struct ps_decoder_t:
        pass
    ctypedef struct ps_seg_t:
        pass
    ctypedef struct ps_nbest_t:
        pass
    arg_t *ps_args()
    ps_decoder_t *ps_init(cmd_ln_t *config)
    int ps_free(ps_decoder_t *ps)
    int ps_reinit(ps_decoder_t *ps, cmd_ln_t *config)
    int ps_reinit_feat(ps_decoder_t *ps, cmd_ln_t *config)
    logmath_t *ps_get_logmath(ps_decoder_t *ps)
    int ps_start_stream(ps_decoder_t *ps)
    int ps_get_in_speech(ps_decoder_t *ps)
    int ps_start_utt(ps_decoder_t *ps)
    int ps_process_raw(ps_decoder_t *ps,
                       const short *data, size_t n_samples,
                       int no_search, int full_utt)
    int ps_process_cep(ps_decoder_t *ps,
                       float **data,
                       int n_frames,
                       int no_search,
                       int full_utt)
    int ps_end_utt(ps_decoder_t *ps)
    const char *ps_get_hyp(ps_decoder_t *ps, int *out_best_score)
    int ps_get_prob(ps_decoder_t *ps)
    ps_seg_t *ps_seg_iter(ps_decoder_t *ps)
    ps_seg_t *ps_seg_next(ps_seg_t *seg)
    const char *ps_seg_word(ps_seg_t *seg)
    void ps_seg_frames(ps_seg_t *seg, int *out_sf, int *out_ef)
    int ps_seg_prob(ps_seg_t *seg, int *out_ascr, int *out_lscr, int *out_lback)
    void ps_seg_free(ps_seg_t *seg)
    int ps_add_word(ps_decoder_t *ps, char *word, char *phones, int update)
    char *ps_lookup_word(ps_decoder_t *ps, const char *word)
    ps_nbest_t *ps_nbest(ps_decoder_t *ps)
    ps_nbest_t *ps_nbest_next(ps_nbest_t *nbest)
    const char *ps_nbest_hyp(ps_nbest_t *nbest, int *out_score)
    ps_seg_t *ps_nbest_seg(ps_nbest_t *nbest)
    void ps_nbest_free(ps_nbest_t *nbest)
    ps_lattice_t *ps_get_lattice(ps_decoder_t *ps)
    void ps_get_utt_time(ps_decoder_t *ps, double *out_nspeech,
                         double *out_ncpu, double *out_nwall)
    void ps_get_all_time(ps_decoder_t *ps, double *out_nspeech,
                         double *out_ncpu, double *out_nwall)
    fe_t *ps_get_fe(ps_decoder_t *ps)
    int ps_get_n_frames(ps_decoder_t *ps)
    cmd_ln_t *ps_get_config(ps_decoder_t *ps)
    int ps_load_dict(ps_decoder_t *ps, const char *dictfile,
                     const char *fdictfile, const char *format)
    int ps_save_dict(ps_decoder_t *ps, const char *dictfile,
                     const char *format)


cdef extern from "pocketsphinx/ps_search.h":
    ctypedef struct ps_search_iter_t:
        pass
    int ps_set_search(ps_decoder_t *ps, const char *name)
    int ps_unset_search(ps_decoder_t *ps, const char *name)
    const char *ps_get_search(ps_decoder_t *ps)
    ps_search_iter_t *ps_search_iter(ps_decoder_t *ps)
    ps_search_iter_t *ps_search_iter_next(ps_search_iter_t *itor)
    const char* ps_search_iter_val(ps_search_iter_t *itor)
    void ps_search_iter_free(ps_search_iter_t *itor)
    ngram_model_t *ps_get_lm(ps_decoder_t *ps, const char *name)
    int ps_set_lm(ps_decoder_t *ps, const char *name, ngram_model_t *lm)
    int ps_set_lm_file(ps_decoder_t *ps, const char *name, const char *path)
    fsg_model_t *ps_get_fsg(ps_decoder_t *ps, const char *name)
    int ps_set_fsg(ps_decoder_t *ps, const char *name, fsg_model_t *fsg)
    int ps_set_jsgf_file(ps_decoder_t *ps, const char *name, const char *path)
    int ps_set_jsgf_string(ps_decoder_t *ps, const char *name, const char *jsgf_string)
    const char* ps_get_kws(ps_decoder_t *ps, const char *name)
    int ps_set_kws(ps_decoder_t *ps, const char *name, const char *keyfile)
    int ps_set_keyphrase(ps_decoder_t *ps, const char *name, const char *keyphrase)
    int ps_set_allphone(ps_decoder_t *ps, const char *name, ngram_model_t *lm)
    int ps_set_allphone_file(ps_decoder_t *ps, const char *name, const char *path)
    int ps_set_align(ps_decoder_t *ps, const char *name, const char *words)

cdef extern from "../src/common_audio/vad/include/webrtc_vad.h":
    ctypedef struct VadInst:
        pass
    VadInst* WebRtcVad_Create()
    void WebRtcVad_Free(VadInst* handle)
    int WebRtcVad_Init(VadInst* handle)
    int WebRtcVad_set_mode(VadInst* handle, int mode)
    int WebRtcVad_Process(VadInst* handle,
                          int fs,
                          const short* audio_frame,
                          size_t frame_length)
    int WebRtcVad_ValidRateAndFrameLength(int rate, size_t frame_length)
