/* -*- tab-width: 8; c-file-style: "linux" -*- */
/* Copyright (c) 2007 Carnegie Mellon University. All rights *
 * reserved.
 *
 * You may copy, modify, and distribute this code under the same terms
 * as PocketSphinx or Python, at your convenience, as long as this
 * notice is not removed.
 *
 * Author: David Huggins-Daines <dhuggins@cs.cmu.edu>
 *
 * This work was partially funded by BAE Systems under contract from
 * the Defense Advanced Research Projects Agency.
 */

#include "Python.h"

#include "fbs.h"
#include "cmd_ln.h"

static char **argv;
static int argc;
static int utt_done;

/* FIXME/NOTE: The pocketsphinx_parse_foo() functions are copied
 * exactly from the ones in _sphinx3module.c, since the command line
 * interface is the same.
 */

/* Parse command line from file */
static PyObject *
pocketsphinx_parse_argfile(PyObject *self, PyObject *args)
{
	const char *filename;

	if (!PyArg_ParseTuple(args, "s", &filename))
		return NULL;
	if (cmd_ln_parse_file(fbs_get_args(), (char *)filename, FALSE) == -1) {
		/* Raise an IOError, the file did not exist (probably). */
		PyErr_SetString(PyExc_IOError, "Argument file could not be read");
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

/* Parse command line from Python dictionary */
static PyObject *
pocketsphinx_parse_argdict(PyObject *self, PyObject *args)
{
	PyObject *seq;
	int i;

	if (!PyArg_ParseTuple(args, "O", &seq))
		return NULL;

	if ((seq = PyMapping_Items(seq)) == NULL) {
		return NULL;
	}
	
	if (argv) {
		for (i = 0; i < argc; ++i) {
			free(argv[i]);
		}
		free(argv);
		argv = NULL;
	}

	argc = PySequence_Size(seq);
	/* Allocate bogus initial and NULL final entries */
	if ((argv = calloc(argc * 2 + 2, sizeof(*argv))) == NULL)
		return PyErr_NoMemory();
	argv[0] = strdup("pocketsphinx_python");

	for (i = 0; i < argc; ++i) {
		PyObject *pair, *str;
		const char *key, *val;

		if ((pair = PySequence_GetItem(seq, i)) == NULL)
			return NULL;
		if ((str = PyTuple_GetItem(pair, 0)) == NULL)
			return NULL;
		if ((str = PyObject_Str(str)) == NULL)
			return NULL;
		if ((key = PyString_AsString(str)) == NULL)
			return NULL;
		Py_DECREF(str);
		if (key[0] != '-') {
			argv[i*2+1] = calloc(strlen(key) + 2, 1);
			argv[i*2+1][0] = '-';
			strcat(argv[i*2+1], key);
		}
		else
			argv[i*2+1] = strdup(key);

		if ((str = PyTuple_GetItem(pair, 1)) == NULL)
			return NULL;
		if ((str = PyObject_Str(str)) == NULL)
			return NULL;
		Py_DECREF(str);
		if ((val = PyString_AsString(str)) == NULL)
			return NULL;
		argv[i*2+2] = strdup(val);
	}

	argc = argc * 2 + 1;
	if (cmd_ln_parse(fbs_get_args(), argc, argv, FALSE) == -1) {
		/* This actually won't ever happen */
		PyErr_SetString(PyExc_ValueError, "Arguments are invalid");
		return NULL;
	}

	Py_DECREF(seq);
	Py_INCREF(Py_None);
	return Py_None;
}

/* Parse command line from Python array or sequence */
static PyObject *
pocketsphinx_parse_argv(PyObject *self, PyObject *args)
{
	PyObject *seq;
	int i;

	if (!PyArg_ParseTuple(args, "O", &seq))
		return NULL;

	if (!PySequence_Check(seq)) {
		PyErr_SetString(PyExc_TypeError, "Argument is not a sequence");
		return NULL;
	}

	if (argv) {
		for (i = 0; i < argc; ++i) {
			free(argv[i]);
		}
		free(argv);
		argv = NULL;
	}

	argc = PySequence_Size(seq);
	if ((argv = calloc(argc + 1, sizeof(*argv))) == NULL)
		return PyErr_NoMemory();

	for (i = 0; i < argc; ++i) {
		PyObject *str;
		const char *arg;

		if ((str = PySequence_GetItem(seq, i)) == NULL)
			return NULL;
		if ((str = PyObject_Str(str)) == NULL)
			return NULL;
		if ((arg = PyString_AsString(str)) == NULL)
			return NULL;
		argv[i] = strdup(arg);

		Py_DECREF(str);
	}

	if (cmd_ln_parse(fbs_get_args(), argc, argv, FALSE) == -1) {
		/* This actually won't ever happen */
		PyErr_SetString(PyExc_ValueError, "Arguments are invalid");
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

/* Initialize live decoder */
static PyObject *
pocketsphinx_init(PyObject *self, PyObject *args)
{
	if (argv == NULL) {
		PyErr_SetString(PyExc_RuntimeError,
				"_pocketsphinx.init called before parsing arguments");
		return NULL;
	}
	/* We already parsed the command line so make it NULL. */
	fbs_init(0, NULL);
	Py_INCREF(Py_None);
	return Py_None;
}

/* Wrap up the live decoder */
static PyObject *
pocketsphinx_close(PyObject *self, PyObject *args)
{
	fbs_end();
	cmd_ln_free();
	Py_INCREF(Py_None);
	return Py_None;
}

/* Get hypothesis string and segmentation. */
static PyObject *
pocketsphinx_get_hypothesis(PyObject *self, PyObject *args)
{
	PyObject *hypstr_obj, *hypseg_obj;
	search_hyp_t *hypsegs, *h;
	char *hypstr;
	int nhyps, nframes, i;

	if (utt_done) {
		if (uttproc_result(&nframes, &hypstr, 1) < 0) {
			PyErr_SetString(PyExc_RuntimeError,
					"uttproc_result() failed");
			return NULL;
		}
		hypstr_obj = PyString_FromString(hypstr);
		if (uttproc_result_seg(&nframes, &hypsegs, 1) < 0) {
			PyErr_SetString(PyExc_RuntimeError,
					"uttproc_result_seg() failed");
			return NULL;
		}
	}
	else {
		if (uttproc_partial_result(&nframes, &hypstr) < 0) {
			PyErr_SetString(PyExc_RuntimeError,
					"uttproc_partial_result() failed");
			return NULL;
		}
		hypstr_obj = PyString_FromString(hypstr);
		if (uttproc_partial_result_seg(&nframes, &hypsegs) < 0) {
			PyErr_SetString(PyExc_RuntimeError,
					"uttproc_partial_result_seg() failed");
			return NULL;
		}
	}
	nhyps = 0;
	for (h = hypsegs; h; h = h->next)
		++nhyps;

	hypseg_obj = PyTuple_New(nhyps);
	for (i = 0, h = hypsegs; h; ++i, h = h->next) {
		PyObject *seg_obj;

		seg_obj = Py_BuildValue("(siiii)",
					h->word,
					h->sf,
					h->ef,
					h->ascr,
					h->lscr);
		PyTuple_SET_ITEM(hypseg_obj, i, seg_obj);
	}

	return Py_BuildValue("(OO)", hypstr_obj, hypseg_obj);
}

static PyObject *
pocketsphinx_begin_utt(PyObject *self, PyObject *args)
{
	char *uttid = NULL;

	if (!PyArg_ParseTuple(args, "|s", &uttid))
		return NULL;

	if (uttproc_begin_utt(uttid) < 0) {
		PyErr_SetString(PyExc_RuntimeError, "uttproc_begin_utt() failed");
		return NULL;
	}

	/* NOTE: Necessitated by the fbs.h interface having different
	 * functions for partial and final results. */
	utt_done = 0;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
pocketsphinx_end_utt(PyObject *self, PyObject *args)
{
	if (uttproc_end_utt() < 0) {
		PyErr_SetString(PyExc_RuntimeError, "uttproc_end_utt() failed");
		return NULL;
	}
	/* NOTE: Necessitated by the fbs.h interface having different
	 * functions for partial and final results. */
	utt_done = 1;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
pocketsphinx_process_raw(PyObject *self, PyObject *args)
{
	PyObject *str;
	int16 *data;
	int32 nsamps;
	int32 nframes, block = FALSE;

	if (!PyArg_ParseTuple(args, "O|i", &str, &block))
		return NULL;
	if ((data = (int16 *)PyString_AsString(str)) == NULL)
		return NULL;
	nsamps = PyString_Size(str)/2;

	if ((nframes = uttproc_rawdata(data, nsamps, block)) == -1) {
		PyErr_SetString(PyExc_ValueError, "Problem in uttproc_rawdata()");
		return NULL;
	}

	return Py_BuildValue("i", nframes);
}

static PyObject *
pocketsphinx_read_lm(PyObject *self, PyObject *args)
{
	const char *lmfile, *lmname;

	if (!PyArg_ParseTuple(args, "ss", &lmfile, &lmname))
		return NULL;

	lm_read(lmfile, lmname,
			  cmd_ln_float32("-lw"),
			  cmd_ln_float32("-uw"),
			  cmd_ln_float32("-wip"));
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
pocketsphinx_set_lm(PyObject *self, PyObject *args)
{
	const char *lmname;

	if (!PyArg_ParseTuple(args, "s", &lmname))
		return NULL;

	uttproc_set_lm(lmname);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
pocketsphinx_delete_lm(PyObject *self, PyObject *args)
{
	const char *lmname;

	if (!PyArg_ParseTuple(args, "s", &lmname))
		return NULL;

	lm_delete(lmname);
	Py_INCREF(Py_None);
	return Py_None;
}

/* Decode raw waveform data */
static PyObject *
pocketsphinx_decode_raw(PyObject *self, PyObject *args)
{
	PyObject *str;
	int16 *data;
	int32 nsamps;
	int32 nframes;
	char *uttid = NULL;

	if (!PyArg_ParseTuple(args, "O|s", &str, &uttid))
		return NULL;
	if ((data = (int16 *)PyString_AsString(str)) == NULL)
		return NULL;
	nsamps = PyString_Size(str)/2;

	if (uttproc_begin_utt(uttid) < 0) {
		PyErr_SetString(PyExc_RuntimeError, "uttproc_begin_utt() failed");
		return NULL;
	}
	if ((nframes = uttproc_rawdata(data, nsamps, 1)) == -1) {
		PyErr_SetString(PyExc_ValueError, "Problem in uttproc_rawdata()");
		return NULL;
	}
	if (uttproc_end_utt() < 0) {
		PyErr_SetString(PyExc_RuntimeError, "uttproc_end_utt() failed");
		return NULL;
	}

	/* NOTE: Necessitated by the fbs.h interface having different
	 * functions for partial and final results. */
	utt_done = 1;

	/* Now get the results and return them. */
	return pocketsphinx_get_hypothesis(self, args);
}

/* Decode a feature file */
static PyObject *
pocketsphinx_decode_cep_file(PyObject *self, PyObject *args)
{
	const char *filename;
	char *uttid = NULL;
	int sf = 0;
	int ef = -1;
	int nframes;

	if (!PyArg_ParseTuple(args, "s|iis", &filename, &sf, &ef, &uttid))
		return NULL;

	if ((nframes = uttproc_decode_cep_file(filename, uttid, sf, ef, 0)) < 0) {
		PyErr_SetString(PyExc_RuntimeError, "uttproc_decode_cep_file() failed");
		return NULL;
	}

	/* NOTE: Necessitated by the fbs.h interface having different
	 * functions for partial and final results. */
	utt_done = 1;

	/* Now get the results and return them. */
	return pocketsphinx_get_hypothesis(self, args);
}

static PyMethodDef pocketsphinxmethods[] = {
	{ "init", pocketsphinx_init, METH_VARARGS,
	  "Initialize the PocketSphinx decoder.\n"
	  "You must first call one of parse_argfile, parse_argdict, parse_argv." },
	{ "close", pocketsphinx_close, METH_VARARGS,
	  "Shut down the PocketSphinx decoder." },
	{ "parse_argfile", pocketsphinx_parse_argfile, METH_VARARGS,
	  "Load PocketSphinx parameters from a file." },
	{ "parse_argv", pocketsphinx_parse_argv, METH_VARARGS,
	  "Set PocketSphinx parameters from an argv array (e.g. sys.argv)\n"
	  "Note that the first element of this array is IGNORED." },
	{ "parse_argdict", pocketsphinx_parse_argdict, METH_VARARGS,
	  "Load PocketSphinx parameters from a dictionary.\n"
	  "Keys can optionally begin with a - as in the command line."	},
	{ "begin_utt", pocketsphinx_begin_utt, METH_VARARGS,
	  "Mark the start of the current utterance.\n" },
	{ "end_utt", pocketsphinx_end_utt, METH_VARARGS,
	  "Mark the end of the current utterance, doing final search if necessary.\n" },
	{ "process_raw", pocketsphinx_process_raw, METH_VARARGS,
	  "Process a block of raw audio.\n" },
	/* Processing cepstra might wait for a bit until I decide if
	 * it's worth pulling in NumPy */
	{ "get_hypothesis", pocketsphinx_get_hypothesis, METH_VARARGS,
	  "Get current hypothesis string and segmentation.\n" },
	{ "set_lm", pocketsphinx_set_lm, METH_VARARGS,
	  "Set the current language model to the one named (must be previously loaded).\n" },
	{ "read_lm", pocketsphinx_read_lm, METH_VARARGS,
	  "Load a language model from a file and associate it with a name.\n" },
	{ "delete_lm", pocketsphinx_delete_lm, METH_VARARGS,
	  "Unload and free resources used by the named language model.\n" },
	{ "decode_raw", pocketsphinx_decode_raw, METH_VARARGS,
	  "Decode an entire utterance of raw waveform data from a Python string.\n" },
	{ "decode_cep_file", pocketsphinx_decode_cep_file, METH_VARARGS,
	  "Decode a Sphinx-format feature (MFCC) file.\n" },
	{ NULL, NULL, 0, NULL }
};

PyMODINIT_FUNC
init_pocketsphinx(void)
{
	(void) Py_InitModule("_pocketsphinx", pocketsphinxmethods);
}
