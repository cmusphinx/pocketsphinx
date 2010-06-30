%module pocketsphinx

%{
#include <pocketsphinx.h>

/* Typedefs to make Java-esque class names. */
typedef struct cmd_ln_s Config;
typedef struct ps_seg_s SegmentIterator;
typedef struct ps_lattice_s Lattice;
typedef struct ps_decoder_s Decoder;
typedef int bool;
#define false 0
#define true 1

%}

/* These are opaque types but we have to "define" them for SWIG. */
typedef struct cmd_ln_s {
} Config;
typedef struct ps_seg_s {
} SegmentIterator;
typedef struct ps_lattice_s {
} Lattice;
typedef struct ps_decoder_s {
} Decoder;

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
	~Decoder() {
		ps_free($self);
	}
};
