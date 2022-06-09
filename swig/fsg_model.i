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


// TODO: search for functions returning error code
%extend FsgModel {

    FsgModel(const char *name, LogMath *logmath, float lw, int n) {
	return fsg_model_init(name, logmath, lw, n);
    }

    FsgModel(const char *path, LogMath *logmath, float lw) {
        return fsg_model_readfile(path, logmath, lw);
    }

    ~FsgModel() {
        fsg_model_free($self);
    }

    int word_id(const char *word) {
        return fsg_model_word_id($self, word);
    }

    const char *word_str(int wid) {
        return fsg_model_word_str($self, wid);
    }
    
    int word_add(const char *word) {
        return fsg_model_word_add($self, word);
    }

    void trans_add(int src, int dst, int logp, int wid) {
        fsg_model_trans_add($self, src, dst, logp, wid);
    }

    int null_trans_add(int src, int dst, int logp) {
        return fsg_model_null_trans_add($self, src, dst, logp);
    }

    int tag_trans_add(int src, int dst, int logp, int wid) {
        return fsg_model_tag_trans_add($self, src, dst, logp, wid);
    }

    int add_silence(const char *silword, int state, float silprob) {
        return fsg_model_add_silence($self, silword, state, silprob); 
    }

    int add_alt(const char *baseword, const char *altword) {
        return fsg_model_add_alt($self, baseword, altword);
    }

    // TODO: Figure out what to do about null_trans_closure, model_trans, arc iterators, etc

    void writefile(const char *path) {
        fsg_model_writefile($self, path);
    }

    void writefile_fsm(const char *path) {
        fsg_model_writefile_fsm($self, path);
    }

    void writefile_symtab(const char *path) {
        fsg_model_writefile_symtab($self, path);
    }

    // TODO: Figure out if we can support the FILE* versions of those functions

    int get_final_state() {
        return fsg_model_final_state($self);
    }

    void set_final_state(int state) {
        fsg_model_final_state($self) = state;
    }

    int get_start_state() {
        return fsg_model_start_state($self);
    }

    void set_start_state(int state) {
        fsg_model_start_state($self) = state;
    }

    int log(double logp) {
        return fsg_model_log($self, logp);
    }

    float get_lw() {
        return fsg_model_lw($self);
    }

    char const *get_name() {
        return fsg_model_name($self);
    }

    int get_n_word() {
        return fsg_model_n_word($self);
    }

    bool has_sil() {
        return fsg_model_has_sil($self);
    }

    bool has_alt() {
        return fsg_model_has_alt($self);
    }

    bool is_filler(int wid) {
        return fsg_model_is_filler($self, wid);
    }

    bool is_alt(int wid) {
        return fsg_model_is_alt($self, wid);
    }
}
