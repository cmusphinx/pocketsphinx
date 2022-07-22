# cython: embedsignature=True
# Copyright (c) 2008-2020 Carnegie Mellon University. All rights
# reserved.
#
# You may copy, modify, and distribute this code under the same terms
# as PocketSphinx or Python, at your convenience, as long as this
# notice is not removed.
#
# Author: David Huggins-Daines <dhdaines@gmail.com>

from libc.stdlib cimport malloc, free
from libc.string cimport memcpy
import itertools
import logging
import pocketsphinx5
cimport _pocketsphinx

LOGGER = logging.getLogger("pocketsphinx5")

cdef class Config:
    """Configuration object for PocketSphinx.

    The PocketSphinx recognizer can be configured either implicitly,
    by passing keyword arguments to `Decoder`, or by creating and
    manipulating `Config` objects.  There are a large number of
    parameters, most of which are not important or subject to change.

    A `Config` can be initialized from a list of arguments as on a
    command line::

        config = Config("-hmm", "path/to/things", "-dict", "my.dict")

    It can also be initialized with keyword arguments::

        config = Config(hmm="path/to/things", dict="my.dict")

    It is possible to access the `Config` either with the set_*
    methods or directly by setting and getting keys, as with a Python
    dictionary.  The set_* methods are not recommended as they require
    the user to know the type of the configuration parameter and to
    pre-pend a leading '-' character.  That is, the following are
    equivalent::

        config.get_string("-dict")
        config["dict"]

    In a future version, probably the next major one, these methods
    will be deprecated or may just disappear.

    In general, a `Config` mostly acts like a dictionary, and can be
    iterated over in the same fashion.  However, attempting to access
    a parameter that does not already exist will raise a `KeyError`.

    See :doc:`config_params` for a description of existing parameters.

 """
    cdef cmd_ln_t *cmd_ln

    def __cinit__(self, *args, **kwargs):
        cdef char **argv
        if args or kwargs:
            args = [str(k).encode('utf-8')
                    for k in args]
            for k, v in kwargs.items():
                if len(k) > 0 and k[0] != '-':
                    k = "-" + k
                args.append(k.encode('utf-8'))
                if v is None:
                    args.append(None)
                else:
                    args.append(str(v).encode('utf-8'))
            argv = <char **> malloc((len(args) + 1) * sizeof(char *))
            argv[len(args)] = NULL
            for i, buf in enumerate(args):
                if buf is None:
                    argv[i] = NULL
                else:
                    argv[i] = buf
            self.cmd_ln = cmd_ln_parse_r(NULL, ps_args(),
                                         len(args), argv, 0)
            free(argv)
        else:
            self.cmd_ln = cmd_ln_parse_r(NULL, ps_args(), 0, NULL, 0)

    def __init__(self, *args, **kwargs):
        pass

    def __dealloc__(self):
        cmd_ln_free_r(self.cmd_ln)

    def get_float(self, key):
        return cmd_ln_float_r(self.cmd_ln, key.encode('utf-8'))

    def get_int(self, key):
        return cmd_ln_int_r(self.cmd_ln, key.encode('utf-8'))

    def get_string(self, key):
        cdef const char *val = cmd_ln_str_r(self.cmd_ln,
                                            key.encode('utf-8'))
        if val == NULL:
            return None
        else:
            return val.decode('utf-8')

    def get_boolean(self, key):
        return cmd_ln_int_r(self.cmd_ln, key.encode('utf-8')) != 0

    def set_float(self, key, double val):
        cmd_ln_set_float_r(self.cmd_ln, key.encode('utf-8'), val)

    def set_int(self, key, long val):
        cmd_ln_set_int_r(self.cmd_ln, key.encode('utf-8'), val)

    def set_boolean(self, key, val):
        cmd_ln_set_int_r(self.cmd_ln, key.encode('utf-8'), val != 0)

    def set_string(self, key, val):
        if val == None:
            cmd_ln_set_str_r(self.cmd_ln, key.encode('utf-8'), NULL)
        else:
            cmd_ln_set_str_r(self.cmd_ln, key.encode('utf-8'), val.encode('utf-8'))

    def set_string_extra(self, key, val):
        if val == None:
            cmd_ln_set_str_extra_r(self.cmd_ln, key.encode('utf-8'), NULL)
        else:
            cmd_ln_set_str_extra_r(self.cmd_ln, key.encode('utf-8'), val.encode('utf-8'))

    def exists(self, key):
        return key in self

    cdef _normalize_key(self, key):
        # Note, returns a Python bytes string, to avoid unsafe temps
        if len(key) > 0:
            if key[0] == "_":
                # Ask for underscore, get underscore
                return key.encode('utf-8')
            elif key[0] == "-":
                # Ask for dash, get underscore or dash
                under_key = ("_" + key[1:]).encode('utf-8')
                if cmd_ln_exists_r(self.cmd_ln, under_key):
                    return under_key
                else:
                    return key.encode('utf-8')
            else:
                # No dash or underscore, try underscore, then dash
                under_key = ("_" + key).encode('utf-8')
                if cmd_ln_exists_r(self.cmd_ln, under_key):
                    return under_key
                dash_key = ("-" + key).encode('utf-8')
                if cmd_ln_exists_r(self.cmd_ln, dash_key):
                    return dash_key
        return key.encode('utf-8')

    cdef _normalize_ckey(self, const char *ckey):
        key = ckey.decode('utf-8')
        if len(key) == 0:
            return key
        if key[0] in "-_":
            return key[1:]

    def __contains__(self, key):
        return cmd_ln_exists_r(self.cmd_ln, self._normalize_key(key))

    def __getitem__(self, key):
        cdef const char *cval
        cdef cmd_ln_val_t *at;
        ckey = self._normalize_key(key)
        at = cmd_ln_access_r(self.cmd_ln, ckey)
        if at == NULL:
            raise KeyError("Unknown key %s" % key)
        elif at.type & ARG_STRING:
            cval = cmd_ln_str_r(self.cmd_ln, ckey)
            if cval == NULL:
                return None
            else:
                return cval.decode('utf-8')
        elif at.type & ARG_INTEGER:
            return cmd_ln_int_r(self.cmd_ln, ckey)
        elif at.type & ARG_FLOATING:
            return cmd_ln_float_r(self.cmd_ln, ckey)
        elif at.type & ARG_BOOLEAN:
            return cmd_ln_int_r(self.cmd_ln, ckey) != 0
        else:
            raise ValueError("Unable to handle parameter type %d" % at.type)

    def __setitem__(self, key, val):
        cdef cmd_ln_val_t *at;
        ckey = self._normalize_key(key)
        at = cmd_ln_access_r(self.cmd_ln, ckey)
        if at == NULL:
            # FIXME: for now ... but should handle this
            raise KeyError("Unknown key %s" % key)
        elif at.type & ARG_STRING:
            if val is None:
                cmd_ln_set_str_r(self.cmd_ln, ckey, NULL)
            else:
                cmd_ln_set_str_r(self.cmd_ln, ckey, str(val).encode('utf-8'))
        elif at.type & ARG_INTEGER:
            cmd_ln_set_int_r(self.cmd_ln, ckey, int(val))
        elif at.type & ARG_FLOATING:
            cmd_ln_set_float_r(self.cmd_ln, ckey, float(val))
        elif at.type & ARG_BOOLEAN:
            cmd_ln_set_int_r(self.cmd_ln, ckey, val != 0)
        else:
            raise ValueError("Unable to handle parameter type %d" % at.type)

    def __iter__(self):
        cdef hash_table_t *ht = self.cmd_ln.ht
        cdef hash_iter_t *itor
        cdef const char *ckey
        keys = set()
        itor = hash_table_iter(self.cmd_ln.ht)
        while itor != NULL:
            ckey = hash_entry_key(itor.ent)
            key = self._normalize_ckey(ckey)
            keys.add(key)
            itor = hash_table_iter_next(itor)
        return iter(keys)

    def items(self):
        keys = list(self)
        for key in keys:
            yield (key, self[key])

    def __len__(self):
        # Incredibly, the only way to do this, but necessary also
        # because of dash and underscore magic.
        return sum(1 for _ in self)

    def describe(self):
        """Iterate over parameter descriptions.

        This function returns a generator over the parameters defined
        in a configuration, as `Arg` objects.

        Returns:
            Iterable[Arg]: Descriptions of parameters including their
            default values and documentation

        """
        cdef const arg_t *arg = self.cmd_ln.defn
        cdef int base_type
        while arg != NULL and arg.name != NULL:
            name = arg.name.decode('utf-8')
            if name[0] == '-':
                name = name[1:]
            if arg.deflt == NULL:
                default = None
            else:
                default = arg.deflt.decode('utf-8')
            if arg.doc == NULL:
                doc = None
            else:
                doc = arg.doc.decode('utf-8')
            required = (arg.type & ARG_REQUIRED) != 0
            base_type = arg.type & ~ARG_REQUIRED
            if base_type == ARG_INTEGER:
                arg_type = int
            elif base_type == ARG_FLOATING:
                arg_type = float
            elif base_type == ARG_STRING:
                arg_type = str
            elif base_type == ARG_BOOLEAN:
                arg_type = bool
            else:
                raise RuntimeError("Unknown type %d in argument %s"
                                   % (base_type, name))
            arg = arg + 1
            yield pocketsphinx5.Arg(name=name, default=default, doc=doc,
                                    type=arg_type, required=required)

cdef class LogMath:
    """Log-space computation object used by PocketSphinx.

    PocketSphinx does various computations internally using integer
    math in logarithmic space with a very small base (usually 1.0001
    or 1.0003).  In general we try to hide this detail from the user,
    but some legacy APIs require it."""
    cdef logmath_t *lmath

    # This is __init__ and *not* __cinit__ because we do not want it
    # to get called by create() below (would leak memory)
    def __init__(self, base=1.0001, shift=0, use_table=False):
        self.lmath = logmath_init(base, shift, use_table)

    @staticmethod
    cdef create(logmath_t *lmath):
        cdef LogMath self = LogMath.__new__(LogMath)
        self.lmath = logmath_retain(lmath)
        return self

    def __dealloc__(self):
        if self.lmath != NULL:
            logmath_free(self.lmath)

    def log(self, p):
        return logmath_log(self.lmath, p)

    def exp(self, p):
        return logmath_exp(self.lmath, p)

    def ln_to_log(self, p):
        return logmath_ln_to_log(self.lmath, p)

    def log_to_ln(self, p):
        return logmath_log_to_ln(self.lmath, p)

    def log10_to_log(self, p):
        return logmath_log10_to_log(self.lmath, p)

    def log_to_log10(self, p):
        return logmath_log_to_log10(self.lmath, p)

    def add(self, p, q):
        return logmath_add(self.lmath, p, q)

    def get_zero(self):
        return logmath_get_zero(self.lmath)

cdef class Segment:
    """Word segmentation, as generated by `Decoder.seg`.

    Attributes:
      word(str): Name of word.
      start_frame(int): Index of start frame.
      end_frame(int): Index of start frame.
      ascore(float): Acoustic score (density).
      lscore(float): Language model score (joint probability).
      lback(int): Language model backoff order.
    """
    cdef public str word
    cdef public int start_frame
    cdef public int end_frame
    cdef public int lback
    cdef public double ascore
    cdef public double prob
    cdef public double lscore

    @staticmethod
    cdef create(ps_seg_t *seg, logmath_t *lmath):
        cdef int ascr, lscr, lback
        cdef int sf, ef
        cdef Segment self

        self = Segment.__new__(Segment)
        self.word = ps_seg_word(seg).decode('utf-8')
        ps_seg_frames(seg, &sf, &ef)
        self.start_frame = sf
        self.end_frame = ef
        self.prob = logmath_exp(lmath,
                                ps_seg_prob(seg, &ascr, &lscr, &lback));
        self.ascore = logmath_exp(lmath, ascr)
        self.lscore = logmath_exp(lmath, lscr)
        self.lback = lback
        return self


cdef class SegmentList:
    """List of word segmentations, as returned by `Decoder.seg`.

    This is a one-time iterator over the word segmentation.  Basically
    you can think of it as Iterable[Segment].  You should not try to
    create it directly.
    """
    cdef ps_seg_t *seg
    cdef logmath_t *lmath

    def __cinit__(self):
        self.seg = NULL
        self.lmath = NULL

    @staticmethod
    cdef create(ps_seg_t *seg, logmath_t *lmath):
        cdef SegmentList self = SegmentList.__new__(SegmentList)
        self.seg = seg
        self.lmath = logmath_retain(lmath)
        return self

    def __iter__(self):
        while self.seg != NULL:
            yield Segment.create(self.seg, self.lmath)
            self.seg = ps_seg_next(self.seg)

    def __dealloc__(self):
        if self.seg != NULL:
            ps_seg_free(self.seg)
        if self.lmath != NULL:
            logmath_free(self.lmath)

cdef class Hypothesis:
    """Recognition hypothesis, as returned by `Decoder.hyp`.

    Attributes:
      hypstr(str): Recognized text.
      score(float): Recognition score.
      best_score(float): Alias for `score` for compatibility.
      prob(float): Posterior probability.
    """
    cdef public str hypstr
    cdef public double score
    cdef public double prob

    @property
    def best_score(self):
        return self.score

    def __init__(self, hypstr, score, prob):
        self.hypstr = hypstr
        self.score = score
        self.prob = prob


cdef class NBestList:
    """List of hypotheses, as returned by `Decoder.nbest`.

    This is a one-time iterator over the N-Best list.  Basically
    you can think of it as Iterable[Hypothesis].  You should not try to
    create it directly.
    """
    cdef ps_nbest_t *nbest
    cdef logmath_t *lmath

    def __cinit__(self):
        self.nbest = NULL
        self.lmath = NULL

    @staticmethod
    cdef create(ps_nbest_t *nbest, logmath_t *lmath):
        cdef NBestList self = NBestList.__new__(NBestList)
        self.nbest = nbest
        self.lmath = logmath_retain(lmath)
        return self

    def __iter__(self):
        while self.nbest != NULL:
            yield self.hyp()
            self.nbest = ps_nbest_next(self.nbest)

    def __dealloc__(self):
        if self.nbest != NULL:
            ps_nbest_free(self.nbest)
        if self.lmath != NULL:
            logmath_free(self.lmath)

    def hyp(self):
        """Get current recognition hypothesis.

        Returns:
            Hypothesis: Current recognition output.
        """
        cdef const char *hyp
        cdef int score

        hyp = ps_nbest_hyp(self.nbest, &score)
        if hyp == NULL:
             return None
        prob = 0
        return Hypothesis(hyp.decode('utf-8'),
                          logmath_exp(self.lmath, score),
                          logmath_exp(self.lmath, prob))


cdef class FsgModel:
    """Finite-state recognition grammar.

    Note that you *cannot* create one of these directly, as it depends
    on some internal configuration from the `Decoder`.  Use the
    factory methods such as `create_fsg` or `parse_jsgf` instead.
    """
    cdef fsg_model_t *fsg

    def __cinit__(self):
        # Unsure this is the right way to do this.  The FsgModel
        # constructor isn't meant to be called from Python.
        self.fsg = NULL

    def __dealloc__(self):
        fsg_model_free(self.fsg)


cdef class Decoder:
    """Main class for speech recognition and alignment in PocketSphinx.

    See :doc:`config_params` for a description of keyword arguments.

    Args:
        config(Config): Optional configuration object.  You can also
                        use keyword arguments, the most important of
                        which are noted below.  See :doc:`config_params`
                        for more information.
        hmm(str): Path to directory containing acoustic model files.
        dict(str): Path to pronunciation dictionary.
        jsgf(str): Path to JSGF grammar file.
        fsg(str): Path to FSG grammar file (only one of ``jsgf`` or ``fsg`` should
                  be specified).
        toprule(str): Name of top-level rule in JSGF file to use as entry point.
        samprate(float): Sampling rate for raw audio data.
        logfn(str): File to write log messages to (set to `os.devnull` to
                    silence these messages)

    """
    cdef ps_decoder_t *ps
    cdef public Config config

    def __cinit__(self, *args, **kwargs):
        if len(args) == 1 and isinstance(args[0], Config):
            self.config = args[0]
        else:
            self.config = Config(*args, **kwargs)
        if self.config is None:
            raise RuntimeError, "Failed to parse argument list"
        self.ps = ps_init(self.config.cmd_ln)
        if self.ps == NULL:
            raise RuntimeError, "Failed to initialize PocketSphinx"

    def __init__(self, *args, **kwargs):
        pass

    def __dealloc__(self):
        ps_free(self.ps)

    @classmethod
    def default_config(_):
        """Return default configuraiton.

        Actually this does the same thing as just creating a `Config`.

        Returns:
            Config: Newly created default configuration.
        """
        return Config()

    def reinit(self, Config config=None):
        """Reinitialize the decoder.

        Args:
            config(Config): Optional new configuration to apply, otherwise
                            the existing configuration in the `config`
                            attribute will be reloaded.

        """
        cdef cmd_ln_t *cconfig
        if config is None:
            cconfig = NULL
        else:
            self.config = config
            cconfig = config.cmd_ln
        if ps_reinit(self.ps, cconfig) != 0:
            raise RuntimeError("Failed to reinitialize decoder configuration")

    def reinit_feat(self, Config config=None):
        """Reinitialize only the feature extraction.

        Args:
            config(Config): Optional new configuration to apply, otherwise
                            the existing configuration in the `config`
                            attribute will be reloaded.

        """
        cdef cmd_ln_t *cconfig
        if config is None:
            cconfig = NULL
        else:
            self.config = config
            cconfig = config.cmd_ln
        if ps_reinit_feat(self.ps, cconfig) < 0:
            raise RuntimeError("Failed to reinitialize feature extraction")

    def start_utt(self):
        """Start processing raw audio input.

        This method must be called at the beginning of each separate
        "utterance" of raw audio input.

        """
        if ps_start_utt(self.ps) < 0:
            raise RuntimeError, "Failed to start utterance processing"

    def process_raw(self, data, no_search=False, full_utt=False):
        """Process a block of raw audio.

        Args:
            data(bytes): Raw audio data, a block of 16-bit signed integer binary data.
            no_search(bool): If `True`, do not do any decoding on this data.
            full_utt(bool): If `True`, assume this is the entire utterance, for
                            purposes of acoustic normalization.

        """
        cdef const unsigned char[:] cdata = data
        cdef Py_ssize_t n_samples = len(cdata) // 2
        if ps_process_raw(self.ps, <const short *>&cdata[0],
                          n_samples, no_search, full_utt) < 0:
            raise RuntimeError, "Failed to process %d samples of audio data" % len / 2

    def process_cep(self, data, no_search=False, full_utt=False):
        """Process a block of MFCC data.

        Args:
            data(bytes): Raw MFCC data, a block of 32-bit floating point data.
            no_search(bool): If `True`, do not do any decoding on this data.
            full_utt(bool): If `True`, assume this is the entire utterance, for
                            purposes of acoustic normalization.

        """
        cdef const unsigned char[:] cdata = data
        cdef int ncep = fe_get_output_size(ps_get_fe(self.ps))
        cdef int nfr = len(cdata) // (ncep * sizeof(float))
        cdef float **feats = <float **>ckd_alloc_2d_ptr(nfr, ncep, <void *>&cdata[0], sizeof(float))
        rv = ps_process_cep(self.ps, feats, nfr, no_search, full_utt)
        ckd_free(feats)
        if rv < 0:
            raise RuntimeError, "Failed to process %d frames of audio data" % nfr

    def end_utt(self):
        """Finish processing raw audio input.

        This method must be called at the end of each separate
        "utterance" of raw audio input.  It takes care of flushing any
        internal buffers and finalizing recognition results.

        """
        if ps_end_utt(self.ps) < 0:
            raise RuntimeError, "Failed to stop utterance processing"

    def hyp(self):
        """Get current recognition hypothesis.

        Returns:
            Hypothesis: Current recognition output.

        """
        cdef const char *hyp
        cdef logmath_t *lmath
        cdef int score

        hyp = ps_get_hyp(self.ps, &score)
        if hyp == NULL:
             return None
        lmath = ps_get_logmath(self.ps)
        prob = ps_get_prob(self.ps)
        return Hypothesis(hyp.decode('utf-8'),
                          logmath_exp(lmath, score),
                          logmath_exp(lmath, prob))

    def get_prob(self):
        """Posterior probability of current recogntion hypothesis.

        Returns:
            float: Posterior probability of current hypothesis.  FIXME:
            At the moment this is almost certainly 1.0, as confidence
            estimation is not well supported.

        """
        cdef logmath_t *lmath
        cdef const char *uttid
        lmath = ps_get_logmath(self.ps)
        return logmath_exp(lmath, ps_get_prob(self.ps))

    def add_word(self, word, phones, update=True):
        """Add a word to the pronunciation dictionary.

        Args:
            word(str): Text of word to be added.
            phones(str): Space-separated list of phones for this
                         word's pronunciation.  This will depend on
                         the underlying acoustic model but is probably
                         in ARPABET.
            update(bool): Update the recognizer immediately.  You can
                          set this to `False` if you are adding a lot
                          of words, to speed things up.

        """
        if not isinstance(word, bytes):
            word = word.encode("utf-8")
        if not isinstance(phones, bytes):
            phones = phones.encode("utf-8")
        return ps_add_word(self.ps, word, phones, update)

    def lookup_word(self, word):
        """Look up a word in the dictionary and return phone transcription
        for it.

        Args:
            word(str|bytes): Text of word to search for.
        Returns:
            str: Space-separated list of phones, or None if not found.
        """
        cdef const char *cphones
        if not isinstance(word, bytes):
            word = word.encode("utf-8")
        cphones = ps_lookup_word(self.ps, word)
        if cphones == NULL:
            return None
        else:
            return cphones.decode("utf-8")

    def seg(self):
        """Get current word segmentation.

        Returns:
            Iterable[Segment]: Generator over word segmentations.

        """
        cdef ps_seg_t *itor
        cdef logmath_t *lmath
        itor = ps_seg_iter(self.ps)
        if itor == NULL:
            return
        lmath = ps_get_logmath(self.ps)
        return SegmentList.create(itor, lmath)


    def nbest(self):
        """Get N-Best hypotheses.

        Returns:
            Iterable[Hypothesis]: Generator over N-Best recognition results

        """
        cdef ps_nbest_t *itor
        cdef logmath_t *lmath
        itor = ps_nbest(self.ps)
        if itor == NULL:
            return
        lmath = ps_get_logmath(self.ps)
        return NBestList.create(itor, lmath)


    def read_fsg(self, filename):
        """Read a grammar from an FSG file.

        Args:
            filename(str): Path to FSG file.

        Returns:
            FsgModel: Newly loaded finite-state grammar.

        """
        cdef logmath_t *lmath
        cdef float lw

        lw = cmd_ln_float_r(self.config.cmd_ln, "-lw")
        lmath = ps_get_logmath(self.ps)
        fsg = FsgModel()
        # FIXME: not the proper way to encode filenames on Windows, I think
        fsg.fsg = fsg_model_readfile(filename.encode(), lmath, lw)
        if fsg.fsg == NULL:
            raise RuntimeError("Failed to read FSG from %s" % filename)
        return fsg

    def read_jsgf(self, filename):
        """Read a grammar from a JSGF file.

        The top rule used is the one specified by the "toprule"
        configuration parameter.

        Args:
            filename(str): Path to JSGF file.

        Returns:
            FsgModel: Newly loaded finite-state grammar.

        """
        cdef logmath_t *lmath
        cdef float lw

        lw = cmd_ln_float_r(self.config.cmd_ln, "-lw")
        lmath = ps_get_logmath(self.ps)
        fsg = FsgModel()
        fsg.fsg = jsgf_read_file(filename.encode(), lmath, lw)
        if fsg.fsg == NULL:
            raise RuntimeError("Failed to read JSGF from %s" % filename)
        return fsg

    def create_fsg(self, name, start_state, final_state, transitions):
        """Create a finite-state grammar.

        This method allows the creation of a grammar directly from a
        list of transitions.  States and words will be created
        implicitly from the state numbers and word strings present in
        this list.  Make sure that the pronunciation dictionary
        contains the words, or you will not be able to recognize.
        Basic usage::

            fsg = decoder.create_fsg("mygrammar",
                                     start_state=0, final_state=3,
                                     transitions=[(0, 1, 0.75, "hello"),
                                                  (0, 1, 0.25, "goodbye"),
                                                  (1, 2, 0.75, "beautiful"),
                                                  (1, 2, 0.25, "cruel"),
                                                  (2, 3, 1.0, "world")])

        Args:
            name(str): Name to give this FSG (not very important).
            start_state(int): Index of starting state.
            final_state(int): Index of end state.
            transitions(list): List of transitions, each of which is a 3-
                               or 4-tuple of (from, to, probability[, word]).
                               If the word is not specified, this is an
                               epsilon (null) transition that will always be
                               followed.

        Returns:
            FsgModel: Newly created finite-state grammar.

        """
        cdef logmath_t *lmath
        cdef float lw
        cdef int wid

        lw = cmd_ln_float_r(self.config.cmd_ln, "-lw")
        lmath = ps_get_logmath(self.ps)
        fsg = FsgModel()
        n_state = max(itertools.chain(*((t[0], t[1]) for t in transitions))) + 1
        fsg.fsg = fsg_model_init(name.encode("utf-8"), lmath, lw, n_state)
        fsg.fsg.start_state = start_state
        fsg.fsg.final_state = final_state
        for t in transitions:
            source, dest, prob = t[0:3]
            if len(t) > 3:
                word = t[3]
                wid = fsg_model_word_add(fsg.fsg, word.encode("utf-8"))
                if wid == -1:
                    raise RuntimeError("Failed to add word to FSG: %s" % word)
                fsg_model_trans_add(fsg.fsg, source, dest,
                                    logmath_log(lmath, prob), wid)
            else:
                fsg_model_null_trans_add(fsg.fsg, source, dest,
                                         logmath_log(lmath, prob))
        return fsg

    def parse_jsgf(self, jsgf_string, toprule=None):
        """Parse a JSGF grammar from bytes or string.

        Because PocketSphinx uses UTF-8 internally, it is more
        efficient to parse from bytes, as a string will get encoded
        and subsequently decoded.

        Args:
            jsgf_string(bytes): JSGF grammar as string or UTF-8 encoded
                                bytes.

        Returns:
            FsgModel: Newly loaded finite-state grammar.

        """
        cdef jsgf_t *jsgf
        cdef jsgf_rule_t *rule
        cdef logmath_t *lmath
        cdef float lw

        if not isinstance(jsgf_string, bytes):
            jsgf_string = jsgf_string.encode("utf-8")
        jsgf = jsgf_parse_string(jsgf_string, NULL)
        if jsgf == NULL:
            raise RuntimeError("Failed to parse JSGF")
        if toprule is not None:
            rule = jsgf_get_rule(jsgf, toprule.encode('utf-8'))
            if rule == NULL:
                jsgf_grammar_free(jsgf)
                raise RuntimeError("Failed to find top rule %s" % toprule)
        else:
            rule = jsgf_get_public_rule(jsgf)
            if rule == NULL:
                jsgf_grammar_free(jsgf)
                raise RuntimeError("No public rules found in JSGF")
        lw = cmd_ln_float_r(self.config.cmd_ln, "-lw")
        lmath = ps_get_logmath(self.ps)
        fsg = FsgModel()
        fsg.fsg = jsgf_build_fsg(jsgf, rule, lmath, lw)
        jsgf_grammar_free(jsgf)
        return fsg

    def set_fsg(self, FsgModel fsg):
        """Set the grammar for recognition.

        Args:
            fsg(FsgModel): Previously loaded or constructed grammar.

        """
        if ps_set_fsg(self.ps, fsg_model_name(fsg.fsg), fsg.fsg) != 0:
            raise RuntimeError("Failed to set FSG in decoder")

    def set_jsgf_file(self, filename, name="_default"):
        """Set the grammar for recognition from a JSGF file.

        Args:
            filename(str): Path to a JSGF file to load.
            name(str): Optional name to give the grammar (not very useful).
        """
        if ps_set_jsgf_file(self.ps, name.encode("utf-8"),
                            filename.encode()) != 0:
            raise RuntimeError("Failed to set JSGF from %s" % filename)

    def set_jsgf_string(self, jsgf_string, name="_default"):
        """Set the grammar for recognition from JSGF bytes or string.

        Args:
            jsgf_string(bytes): JSGF grammar as string or UTF-8 encoded
                                bytes.
            name(str): Optional name to give the grammar (not very useful).
        """
        if not isinstance(jsgf_string, bytes):
            jsgf_string = jsgf_string.encode("utf-8")
        if ps_set_jsgf_string(self.ps, name.encode("utf-8"), jsgf_string) != 0:
            raise RuntimeError("Failed to parse JSGF in decoder")

    def get_logmath(self):
        """Get the LogMath object for this decoder."""
        cdef logmath_t *lmath = ps_get_logmath(self.ps)
        return LogMath.create(lmath)
