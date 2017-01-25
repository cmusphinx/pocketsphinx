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

%extend Decoder {

    /* Following functions have no bindings:
     * ps_mllr_t *ps_update_mllr - requires
     * int ps_decode_senscr
     */
    Decoder(int *errcode) {
        Decoder *d = ps_init(cmd_ln_init(NULL, ps_args(), FALSE, NULL));
        *errcode = d ? 0 : -1;
        return d;
    }

    Decoder(Config *config, int *errcode) {
        Decoder *d = ps_init(config);
        *errcode = d ? 0 : -1;
        return d;
    }

    ~Decoder() {
        ps_free($self);
    }

    void reinit(Config *config, int *errcode) {
        *errcode = ps_reinit($self, config);
    }

    void load_dict(
        char const *fdict, char const *ffilter, char const *format, int *errcode) {
        *errcode = ps_load_dict($self, fdict, ffilter, format);
    }

    void save_dict(char const *dictfile, char const *format, int *errcode) {
        *errcode = ps_save_dict($self, dictfile, format);
    }

    void add_word(char const *word, char const *phones, int update, int *errcode) {
        *errcode = ps_add_word($self, word, phones, update);
    }
    
    %newobject lookup_word;
    char * lookup_word(const char *word) {
        return ps_lookup_word($self, word);
    }

    Lattice * get_lattice() {
        return ps_lattice_retain(ps_get_lattice($self));
    }

    %newobject get_config;
    Config *get_config() {
        return cmd_ln_retain(ps_get_config($self));
    }

    %newobject default_config;
    static Config *default_config() {
        return cmd_ln_parse_r(NULL, ps_args(), 0, NULL, FALSE);
    }

    %newobject file_config;
    static Config *file_config(char const * path) {
        return cmd_ln_parse_file_r(NULL, ps_args(), path, FALSE);
    }

    void start_stream(int *errcode) {
        *errcode = ps_start_stream($self);
    }

    void start_utt(int *errcode) {
        *errcode = ps_start_utt($self);
    }

    void end_utt(int *errcode) {
        *errcode = ps_end_utt($self);
    }

#ifdef SWIGPYTHON
    %include <pybuffer.i>
    %pybuffer_binary(const char* SDATA, size_t NSAMP);
    int process_raw(const char* SDATA, size_t NSAMP, bool no_search, bool full_utt,
                int *errcode) {
        NSAMP /= sizeof(int16);
        return *errcode = ps_process_raw($self, (int16 *)SDATA, NSAMP, no_search, full_utt);
    }
    
    int process_cep(const char *SDATA, size_t NSAMP, bool no_search, bool full_utt,
                int *errcode) {
        mfcc_t **feats;
        int ncep = fe_get_output_size(ps_get_fe($self));
        NSAMP /= ncep * sizeof(mfcc_t);
        feats = ckd_calloc_2d(NSAMP, ncep, sizeof(mfcc_t));
        memcpy(feats[0], SDATA, NSAMP * ncep * sizeof(mfcc_t));
        *errcode = ps_process_cep($self, feats, NSAMP, no_search, full_utt);
        ckd_free_2d(feats);
        return *errcode;
    }
#elif SWIGJAVASCRIPT
    int process_raw(SWIG_Object ptr, bool no_search, bool full_utt,
                int *errcode) {
	int16* data = (int16*) node::Buffer::Data(ptr);
	size_t length = node::Buffer::Length(ptr) / sizeof(int16);
        return *errcode = ps_process_raw($self, data, length, no_search, full_utt);
    }
#elif SWIGJAVA
    int process_raw(const short *SDATA, size_t NSAMP, bool no_search, bool full_utt,
                int *errcode) {
        return *errcode = ps_process_raw($self, SDATA, NSAMP, no_search, full_utt);
    }
#elif SWIGRUBY
    int process_raw(const char* STRING, size_t SIZE, bool no_search, bool full_utt,
                int *errcode) {
        return *errcode = ps_process_raw($self, (const int16 *)STRING, SIZE / 2, no_search, full_utt);
    }
#elif SWIGCSHARP
    int process_raw(const unsigned char* SDATA, int SIZE, bool no_search, bool full_utt,
                int *errcode) {
        return *errcode = ps_process_raw($self, (const int16 *)SDATA, SIZE / 2, no_search, full_utt);
    }
#endif


#ifdef SWIGJAVA
    // Not sure how to properly return binary buffer in python yet (python3 is also an issue)
    void set_rawdata_size(size_t size) {
	ps_set_rawdata_size($self, size);
    }

    short *get_rawdata(int *RAWDATA_SIZE) {
	short *result;
	ps_get_rawdata($self, &result, RAWDATA_SIZE);
	return result;
    }
#endif

    %newobject hyp;
    Hypothesis * hyp() {
        char const *hyp;
        int best_score, prob;
        hyp = ps_get_hyp($self, &best_score);
        if (hyp)
            prob = ps_get_prob($self);
        return hyp ? new_Hypothesis(hyp, best_score, prob) : NULL;
    }

    FrontEnd * get_fe() {
        return ps_get_fe($self);
    }

    Feature * get_feat() {
        return ps_get_feat($self);
    }
   
    bool get_in_speech() {
        return ps_get_in_speech($self);
    }

    FsgModel * get_fsg(const char *name) {
        return fsg_model_retain(ps_get_fsg($self, name));
    }

    void set_fsg(const char *name, FsgModel *fsg, int *errcode) {
        *errcode = ps_set_fsg($self, name, fsg);
    }

    void set_jsgf_file(const char *name, const char *path, int *errcode) {
        *errcode = ps_set_jsgf_file($self, name, path);
    }

    void set_jsgf_string(const char *name, const char *jsgf_string, int *errcode) {
        *errcode = ps_set_jsgf_string($self, name, jsgf_string);
    }

    const char * get_kws(const char *name) {
        return ps_get_kws($self, name);
    }

    void set_kws(const char *name, const char *keyfile, int *errcode) {
        *errcode = ps_set_kws($self, name, keyfile);
    }

    void set_keyphrase(const char *name, const char *keyphrase, int *errcode) {
        *errcode = ps_set_keyphrase($self, name, keyphrase);
    }
    
    void set_allphone_file(const char *name, const char *lmfile, int *errcode) {
	*errcode = ps_set_allphone_file($self, name, lmfile);
    }

    %newobject get_lm;
    NGramModel * get_lm(const char *name) {
        return ngram_model_retain(ps_get_lm($self, name));
    }

    void set_lm(const char *name, NGramModel *lm, int *errcode) {
        *errcode = ps_set_lm($self, name, lm);
    }

    void set_lm_file(const char *name, const char *path, int *errcode) {
        *errcode = ps_set_lm_file($self, name, path);
    }

    %newobject get_logmath;
    LogMath * get_logmath() {
        return logmath_retain(ps_get_logmath($self));
    }

    void set_search(const char *search_name, int *errcode) {
      *errcode = ps_set_search($self, search_name);
    }

    void unset_search(const char *search_name, int *errcode) {
      *errcode = ps_unset_search($self, search_name);
    }

    const char * get_search() {
        return ps_get_search($self);
    }

    int n_frames() {
        return ps_get_n_frames($self);
    }

    SegmentList *seg() {
	return $self;
    }
    
    NBestList *nbest() {
	return $self;
    }
}
