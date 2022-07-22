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
    ctypedef void (*err_cb_f)(void* user_data, err_lvl_t lvl, const char *msg);
    void err_set_callback(err_cb_f callback, void *user_data)


cdef extern from "sphinxbase/logmath.h":
    ctypedef struct logmath_t:
        pass
    
    logmath_t *logmath_init(double base, int shift, int use_table)
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
    int cmd_ln_free_r(cmd_ln_t *cmdln)

    double cmd_ln_float_r(cmd_ln_t *cmdln, const char *name)
    int cmd_ln_type_r(cmd_ln_t *cmdln, const char *name)
    long cmd_ln_int_r(cmd_ln_t *cmdln, const char *name)
    const char *cmd_ln_str_r(cmd_ln_t *cmdln, const char *name)
    void cmd_ln_set_str_r(cmd_ln_t *cmdln, const char *name, const char *str)
    void cmd_ln_set_int_r(cmd_ln_t *cmdln, const char *name, long iv)
    void cmd_ln_set_float_r(cmd_ln_t *cmdln, const char *name, double fv)
    cmd_ln_val_t *cmd_ln_access_r(cmd_ln_t *cmdln, const char *name)

    int cmd_ln_exists_r(cmd_ln_t *cmdln, const char *name)


cdef extern from "sphinxbase/fsg_model.h":
    ctypedef struct fsg_model_t:
        int start_state
        int final_state

    fsg_model_t *fsg_model_init(const char *name, logmath_t *lmath,
                                float lw, int n_state)
    fsg_model_t *fsg_model_readfile(const char *file, logmath_t *lmath,
                                    float lw)
    const char *fsg_model_name(fsg_model_t *fsg)
    int fsg_model_free(fsg_model_t *fsg)

    int fsg_model_word_add(fsg_model_t *fsg, const char *word)
    int fsg_model_word_id(fsg_model_t *fsg, const char *word)
    void fsg_model_trans_add(fsg_model_t * fsg,
                             int source, int dest, int logp, int wid)
    int fsg_model_null_trans_add(fsg_model_t * fsg, int source, int dest,
                                 int logp)
    int fsg_model_tag_trans_add(fsg_model_t * fsg, int source, int dest,
                                int logp, int wid)
    void fsg_model_writefile(fsg_model_t *fsg, const char *file)


cdef extern from "sphinxbase/jsgf.h":
    ctypedef struct jsgf_t:
        pass
    ctypedef struct jsgf_rule_t:
        pass
    fsg_model_t *jsgf_read_file(const char *name, logmath_t *lmath,
                                float lw)
    jsgf_t *jsgf_parse_string(const char *string, jsgf_t *parent)
    const char *jsgf_grammar_name(jsgf_t *jsgf)
    void jsgf_grammar_free(jsgf_t *jsgf)
    jsgf_rule_t *jsgf_get_rule(jsgf_t *grammar, const char *name)
    jsgf_rule_t *jsgf_get_public_rule(jsgf_t *grammar)
    fsg_model_t *jsgf_build_fsg(jsgf_t *grammar, jsgf_rule_t *rule,
                                logmath_t *lmath, float lw)


cdef extern from "pocketsphinx.h":
    ctypedef struct ps_decoder_t:
        pass
    ctypedef struct ps_seg_t:
        pass
    arg_t *ps_args()
    ps_decoder_t *ps_init(cmd_ln_t *config)
    int ps_free(ps_decoder_t *ps)
    int ps_reinit(ps_decoder_t *ps, cmd_ln_t *config)
    int ps_reinit_feat(ps_decoder_t *ps, cmd_ln_t *config)
    logmath_t *ps_get_logmath(ps_decoder_t *ps)
    int ps_start_utt(ps_decoder_t *ps)
    int ps_process_raw(ps_decoder_t *ps,
                       const short *data, size_t n_samples,
                       int no_search, int full_utt)
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
    int ps_set_fsg(ps_decoder_t *ps, const char *name, fsg_model_t *fsg)
    int ps_set_jsgf_file(ps_decoder_t *ps, const char *name, const char *path)
    int ps_set_jsgf_string(ps_decoder_t *ps, const char *name, const char *jsgf_string)
