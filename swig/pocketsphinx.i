%module pocketsphinx
%include <arrays_java.i>
%{
#include <pocketsphinx.h>
#include <sphinxbase/err.h>

/* Typedefs to make Java-esque class names. */
typedef struct cmd_ln_s Config;
typedef struct ps_seg_s SegmentIterator;
typedef struct ps_lattice_s Lattice;
typedef struct ps_decoder_s Decoder;
typedef int bool;
#define false 0
#define true 1

/* Auxiliary objects used to return multiple values. */
typedef struct hyp_s {
	char *hypstr;
	char *uttid;
	int best_score;
} Hypothesis;

/* Nbest iterator */
typedef struct nbest_s {
	ps_nbest_t *nbest;
} Nbest;

%}

/* Special typemap for arrays of audio. */
%typemap(in) (short const *SDATA, size_t NSAMP) {
	$1 = (short const *) JCALL2(GetShortArrayElements, jenv, $input, NULL);
	$2 = JCALL1(GetArrayLength, jenv, $input);
};
%typemap(freearg) (short const *SDATA, size_t NSAMP) {
	JCALL3(ReleaseShortArrayElements, jenv, $input, $1, 0);
};
%typemap(jni) (short const *SDATA, size_t NSAMP) "jshortArray"
%typemap(jtype) (short const *SDATA, size_t NSAMP) "short[]"
%typemap(jstype) (short const *SDATA, size_t NSAMP) "short[]"
%typemap(javain) (short const *SDATA, size_t NSAMP) "$javainput"

/* Auxiliary types for returning multiple values. */
typedef struct hyp_s {
	char *hypstr;
	char *uttid;
	int best_score;
} Hypothesis;

/* Nbest iterator */
typedef struct nbest_s {
	ps_nbest_t *nbest;
} Nbest;

/* These are opaque types but we have to "define" them for SWIG. */
typedef struct cmd_ln_s {
} Config;
typedef struct ps_seg_s {
} SegmentIterator;
typedef struct ps_lattice_s {
} Lattice;
typedef struct ps_decoder_s {
} Decoder;

%extend Hypothesis {
	Hypothesis(char const *hypstr, char const *uttid, int best_score) {
		Hypothesis *h = ckd_calloc(1, sizeof(*h));
		if (hypstr)
			h->hypstr = ckd_salloc(hypstr);
		if (uttid)
			h->uttid = ckd_salloc(uttid);
		h->best_score = best_score;
		return h;
		
	}
	~Hypothesis() {
		ckd_free($self->hypstr);
		ckd_free($self->uttid);
		ckd_free($self);
	}
}

%extend Config {
	Config() {
		Config *c = cmd_ln_init(NULL, ps_args(), FALSE, NULL);
		return c;
	}
	Config(char const *file) {
		Config *c = cmd_ln_parse_file_r(NULL, ps_args(), file, FALSE);
		return c;
	}
	~Config() {
		cmd_ln_free_r($self);
	}
	void setBoolean(char const *key, bool val) {
		cmd_ln_set_boolean_r($self, key, val);
	}
	void setInt(char const *key, int val) {
		cmd_ln_set_int_r($self, key, val);
	}
	void setFloat(char const *key, double val) {
		cmd_ln_set_float_r($self, key, val);
	}
	void setString(char const *key, char const *val) {
		cmd_ln_set_str_r($self, key, val);
	}
	bool exists(char const *key) {
		return cmd_ln_exists_r($self, key);
	}
	bool getBoolean(char const *key) {
		return cmd_ln_boolean_r($self, key);
	}
	int getInt(char const *key) {
		return cmd_ln_int_r($self, key);
	}
	double getFloat(char const *key) {
		return cmd_ln_float_r($self, key);
	}
	char const *getString(char const *key) {
		return cmd_ln_str_r($self, key);
	}
};

%extend SegmentIterator {
	SegmentIterator() {
		return NULL;
	}
}

%extend Lattice {
	Lattice() {
		return NULL;
	}
};

%extend Nbest {
	Nbest(Decoder *d) {
	    Nbest *nbest = ckd_calloc(1, sizeof(*nbest));
	    nbest->nbest = ps_nbest(d, 0, -1, NULL, NULL);
	    return nbest;
	}
	~Nbest() {
	    if ($self->nbest)
	        ps_nbest_free($self->nbest);
	    ckd_free($self);
	}
	
	bool next() {
	    $self->nbest = ps_nbest_next($self->nbest);
	    return $self->nbest == NULL;
	}
	
	Hypothesis* hyp() {
	    const char* hyp;
	    int32 score;
	    hyp = ps_nbest_hyp($self->nbest, &score);
	    return new_Hypothesis(hyp, "", score);
	}
};

%extend Decoder {
	Decoder() {
		Decoder *d = ps_init(cmd_ln_init(NULL, ps_args(), FALSE, NULL));
		return d;
	}
	Decoder(Config *c) {
		Decoder *d = ps_init(c);
		return d;
	}
	Config *getConfig() {
		return cmd_ln_retain(ps_get_config($self));
	}
	int startUtt() {
		return ps_start_utt($self, NULL);
	}
	int startUtt(char const *uttid) {
		return ps_start_utt($self, uttid);
	}
	char const *getUttid() {
		return ps_get_uttid($self);
	}
	int endUtt() {
		return ps_end_utt($self);
	}
	int processRaw(const short const *SDATA, size_t NSAMP, bool no_search, bool full_utt) {
		return ps_process_raw($self, SDATA, NSAMP, no_search, full_utt);
	}
	int processRaw(short const shorts[], size_t nshorts, bool no_search, bool full_utt) {
		return ps_process_raw($self, shorts, nshorts, no_search, full_utt);
	}
	Hypothesis *getHyp() {
		char const *hyp, *uttid;
		int32 best_score;
		hyp = ps_get_hyp($self, &best_score, &uttid);
		return new_Hypothesis(hyp, uttid, best_score);
	}
	~Decoder() {
		ps_free($self);
	}	
};

%inline {
	/* Static method to set the logging file. */
	void setLogfile(char const *path) {
		err_set_logfile(path);
	}
};

