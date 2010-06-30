%module pocketsphinx

%{
#include <pocketsphinx.h>

/* Typedefs to make Java-esque class names. */
typedef struct cmd_ln_s Config;
typedef struct ps_seg_s SegmentIterator;
typedef struct ps_lattice_s Lattice;
typedef struct ps_decoder_s Decoder;

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
	~Config() {
		cmd_ln_free_r($self);
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
		Decoder *d = ps_init(NULL);
		return d;
	}
	~Decoder() {
		ps_free($self);
	}
};
