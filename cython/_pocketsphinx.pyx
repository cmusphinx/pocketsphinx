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

    Finally it can also be initialized by parsing a file::

        config = Config.parse_file("feat.params")

    It is possible to access the `Config` either with the set_*
    methods or directly by setting and getting keys, as with a Python
    dictionary.  The set_* methods are not recommended as they require
    the user to know the type of the configuration parameter and to
    pre-pend a leading '-' character.  That is, the following are
    equivalent::

        config.get_string("-dict")
        config["dict"]

    In general, a `Config` mostly acts like a dictionary, and can be
    iterated over in the same fashion.  However, attempting to access
    a parameter that does not already exist will raise a `KeyError`.

    See :doc:`config_params` for a description of existing parameters.

 """
    cdef cmd_ln_t *cmd_ln

    # This is __init__ so we can bypass it if necessary
    def __init__(self, *args, **kwargs):
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

    @staticmethod
    cdef create_from_ptr(cmd_ln_t *cmd_ln):
        cdef Config self = Config.__new__(Config)
        self.cmd_ln = cmd_ln
        return self

    @staticmethod
    def parse_file(str path):
        """Parse a config file.

        This reads a configuration file in "command-line" format, for example::

            -arg1 value -arg2 value
            -arg3 value

        Args:
            path(str): Path to configuration file.
        Returns:
            Config: Parsed config, or None on error.
        """
        cdef cmd_ln_t *config = cmd_ln_parse_file_r(NULL, ps_args(),
                                                    path.encode(), False)
        if config == NULL:
            return None
        return Config.create_from_ptr(config)
        
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
                raise ValueError("Unknown type %d in argument %s"
                                 % (base_type, name))
            arg = arg + 1
            yield pocketsphinx5.Arg(name=name, default=default, doc=doc,
                                    type=arg_type, required=required)

cdef class LogMath:
    """Log-space computation object used by PocketSphinx.

    PocketSphinx does various computations internally using integer
    math in logarithmic space with a very small base (usually 1.0001
    or 1.0003)."""
    cdef logmath_t *lmath

    # This is __init__ and *not* __cinit__ because we do not want it
    # to get called by create() below (would leak memory)
    def __init__(self, base=1.0001, shift=0, use_table=False):
        self.lmath = logmath_init(base, shift, use_table)

    @staticmethod
    cdef create_from_ptr(logmath_t *lmath):
        cdef LogMath self = LogMath.__new__(LogMath)
        self.lmath = lmath
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


cdef class NGramModel:
    """N-Gram language model."""
    cdef ngram_model_t *lm

    def __init__(self, Config config, LogMath logmath, str path):
        cdef ngram_model_t *lm = ngram_model_read(config.cmd_ln,
                                                  path.encode("utf-8"),
                                                  NGRAM_AUTO,
                                                  logmath.lmath)
        if lm == NULL:
            raise ValueError("Unable to create language model")
        self.lm = lm

    @staticmethod
    def readfile(str path):
        cdef logmath_t *lmath = logmath_init(1.0001, 0, 0)
        cdef ngram_model_t *lm = ngram_model_read(NULL, path.encode("utf-8"),
                                                  NGRAM_AUTO, lmath)
        logmath_free(lmath)
        if lm == NULL:
            raise ValueError("Unable to read language model from %s" % path)
        return NGramModel.create_from_ptr(lm)

    @staticmethod
    cdef create_from_ptr(ngram_model_t *lm):
        cdef NGramModel self = NGramModel.__new__(NGramModel)
        self.lm = lm
        return self

    def __dealloc__(self):
        if self.lm != NULL:
            ngram_model_free(self.lm)

    def write(self, str path, ngram_file_type_t ftype=NGRAM_AUTO):
        cdef int rv = ngram_model_write(self.lm, path.encode(), ftype)
        if rv < 0:
            raise RuntimeError("Failed to write language model to %s" % path)

    @staticmethod
    def str_to_type(str typestr):
        return ngram_str_to_type(typestr.encode("utf-8"))

    @staticmethod
    def type_to_str(ngram_file_type_t _type):
        return ngram_type_to_str(_type).decode("utf-8")

    def casefold(self, ngram_case_t kase):
        cdef int rv = ngram_model_casefold(self.lm, kase)
        if rv < 0:
            raise RuntimeError("Failed to case-fold language model")

    def size(self):
        return ngram_model_get_size(self.lm)

    def add_word(self, word, float weight):
        if not isinstance(word, bytes):
            word = word.encode("utf-8")
        return ngram_model_add_word(self.lm, word, weight)

    def prob(self, words):
        cdef const char **cwords
        cdef int prob
        bwords = [w.encode("utf-8") for w in words]
        cwords = <const char **>malloc(len(bwords))
        for i, w in enumerate(bwords):
            cwords[i] = w
        prob = ngram_prob(self.lm, cwords, len(words))
        free(cwords)
        return prob


cdef class FsgModel:
    """Finite-state recognition grammar.
    """
    cdef fsg_model_t *fsg

    def __init__(self, name, LogMath logmath, float lw, int nstate):
        if not isinstance(name, bytes):
            name = name.encode("utf-8")
        self.fsg = fsg_model_init(name, logmath.lmath,
                                  lw, nstate)
        if self.fsg == NULL:
            raise ValueError("Failed to initialize FSG model")

    @staticmethod
    def readfile(str filename, LogMath logmath, float lw):
        cdef fsg_model_t *cfsg
        cdef FsgModel fsg
        cfsg = fsg_model_readfile(filename.encode(), logmath.lmath, lw)
        return FsgModel.create_from_ptr(cfsg)

    @staticmethod
    def jsgf_read_file(str filename, LogMath logmath, float lw):
        cdef fsg_model_t *cfsg
        cdef FsgModel fsg
        cfsg = jsgf_read_file(filename.encode(), logmath.lmath, lw)
        return FsgModel.create_from_ptr(cfsg)

    @staticmethod
    cdef create_from_ptr(fsg_model_t *fsg):
        cdef FsgModel self = FsgModel.__new__(FsgModel)
        self.fsg = fsg
        return self

    def __dealloc__(self):
        fsg_model_free(self.fsg)

    def word_id(self, word):
        if not isinstance(word, bytes):
            word = word.encode("utf-8")
        return fsg_model_word_id(self.fsg, word)

    def word_str(self, wid):
        return fsg_model_word_str(self.fsg, wid).decode("utf-8")

    def word_add(self, word):
        if not isinstance(word, bytes):
            word = word.encode("utf-8")
        return fsg_model_word_add(self.fsg, word)

    def trans_add(self, int src, int dst, int logp, int wid):
        fsg_model_trans_add(self.fsg, src, dst, logp, wid)

    def null_trans_add(self, int src, int dst, int logp):
        return fsg_model_null_trans_add(self.fsg, src, dst, logp)

    def tag_trans_add(self, int src, int dst, int logp, int wid):
        return fsg_model_tag_trans_add(self.fsg, src, dst, logp, wid)

    def add_silence(self, silword, int state, float silprob):
        if not isinstance(silword, bytes):
            silword = silword.encode("utf-8")
        return fsg_model_add_silence(self.fsg, silword, state, silprob)

    def add_alt(self, baseword, altword):
        if not isinstance(baseword, bytes):
            baseword = baseword.encode("utf-8")
        if not isinstance(altword, bytes):
            altword = altword.encode("utf-8")
        return fsg_model_add_alt(self.fsg, baseword, altword)

    def writefile(self, str path):
        cpath = path.encode()
        fsg_model_writefile(self.fsg, cpath)

    def writefile_fsm(self, str path):
        cpath = path.encode()
        fsg_model_writefile_fsm(self.fsg, cpath)

    def writefile_symtab(self, str path):
        cpath = path.encode()
        fsg_model_writefile_symtab(self.fsg, cpath)


cdef class JsgfRule:
    """JSGF Rule.

    Do not create this class directly."""
    cdef jsgf_rule_t *rule

    @staticmethod
    cdef create_from_ptr(jsgf_rule_t *rule):
        cdef JsgfRule self = JsgfRule.__new__(JsgfRule)
        self.rule = rule
        return self

    def get_name(self):
        return jsgf_rule_name(self.rule).decode("utf-8")

    def is_public(self):
        return jsgf_rule_public(self.rule)


cdef class Jsgf:
    """JSGF parser.
    """
    cdef jsgf_t *jsgf
    
    def __init__(self, str path, Jsgf parent=None):
        cdef jsgf_t *cparent
        cpath = path.encode()
        if parent is not None:
            cparent = parent.jsgf
        else:
            cparent = NULL
        self.jsgf = jsgf_parse_file(cpath, cparent)
        if self.jsgf == NULL:
            raise ValueError("Failed to parse %s as JSGF" % path)

    def __dealloc__(self):
        if self.jsgf != NULL:
            jsgf_grammar_free(self.jsgf)

    def get_name(self):
        return jsgf_grammar_name(self.jsgf).decode("utf-8")

    def get_rule(self, name):
        cdef jsgf_rule_t *rule = jsgf_get_rule(self.jsgf, name.encode("utf-8"))
        return JsgfRule.create_from_ptr(rule)

    def build_fsg(self, JsgfRule rule, LogMath logmath, float lw):
        cdef fsg_model_t *fsg = jsgf_build_fsg(self.jsgf, rule.rule, logmath.lmath, lw)
        return FsgModel.create_from_ptr(fsg)


cdef class Lattice:
    """Word lattice."""
    cdef ps_lattice_t *dag

    @staticmethod
    def readfile(str path):
        cdef ps_lattice_t *dag = ps_lattice_read(NULL, path.encode("utf-8"))
        if dag == NULL:
            raise ValueError("Unable to read lattice from %s" % path)
        return Lattice.create_from_ptr(dag)

    @staticmethod
    cdef create_from_ptr(ps_lattice_t *dag):
        cdef Lattice self = Lattice.__new__(Lattice)
        self.dag = dag
        return self

    def __dealloc__(self):
        if self.dag != NULL:
            ps_lattice_free(self.dag)
    
    def write(self, str path):
        rv = ps_lattice_write(self.dag, path.encode("utf-8"))
        if rv < 0:
            raise RuntimeError("Failed to write lattice to %s" % path)
    
    def write_htk(self, str path):
        rv = ps_lattice_write_htk(self.dag, path.encode("utf-8"))
        if rv < 0:
            raise RuntimeError("Failed to write lattice to %s" % path)

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
        lm(str): Path to N-Gram language model.
        jsgf(str): Path to JSGF grammar file.
        fsg(str): Path to FSG grammar file (only one of ``lm``, ``jsgf``,
                  or ``fsg`` should be specified).
        toprule(str): Name of top-level rule in JSGF file to use as entry point.
        samprate(float): Sampling rate for raw audio data.
        loglevel(str): Logging level, one of "INFO", "ERROR", "FATAL".
        logfn(str): File to write log messages to.
    Raises:
        ValueError: On invalid configuration or argument list.
        RuntimeError: On invalid configuration or other failure to
                      reinitialize decoder.
    """
    cdef ps_decoder_t *ps
    cdef public Config config

    def __init__(self, *args, **kwargs):
        if len(args) == 1 and isinstance(args[0], Config):
            self.config = args[0]
        else:
            self.config = Config(*args, **kwargs)
        if self.config is None:
            raise ValueError, "Failed to parse argument list"
        self.ps = ps_init(self.config.cmd_ln)
        if self.ps == NULL:
            raise RuntimeError, "Failed to initialize PocketSphinx"

    def __dealloc__(self):
        ps_free(self.ps)

    def reinit(self, Config config=None):
        """Reinitialize the decoder.

        Args:
            config(Config): Optional new configuration to apply, otherwise
                            the existing configuration in the `config`
                            attribute will be reloaded.
        Raises:
            RuntimeError: On invalid configuration or other failure to
                          reinitialize decoder.
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
        Raises:
            RuntimeError: On invalid configuration or other failure to
                          initialize feature extraction.
        """
        cdef cmd_ln_t *cconfig
        if config is None:
            cconfig = NULL
        else:
            self.config = config
            cconfig = config.cmd_ln
        if ps_reinit_feat(self.ps, cconfig) < 0:
            raise RuntimeError("Failed to reinitialize feature extraction")

    def start_stream(self):
        """Reset noise statistics.

        This method can be called at the beginning of a new audio
        stream (but this is not necessary)."""
        cdef int rv = ps_start_stream(self.ps)
        if rv < 0:
            raise RuntimeError("Failed to start audio stream")

    def start_utt(self):
        """Start processing raw audio input.

        This method must be called at the beginning of each separate
        "utterance" of raw audio input.

        Raises:
            RuntimeError: If processing fails to start (usually if it
                          has already been started).
        """
        if ps_start_utt(self.ps) < 0:
            raise RuntimeError, "Failed to start utterance processing"

    def get_in_speech(self):
        """Return speech status of previously passed buffer.

        This method is retained for compatibility, but it will always
        return True as long as `ps_start_utt` has been previously
        called.
        """
        return ps_get_in_speech(self.ps)

    def process_raw(self, data, no_search=False, full_utt=False):
        """Process a block of raw audio.

        Args:
            data(bytes): Raw audio data, a block of 16-bit signed integer binary data.
            no_search(bool): If `True`, do not do any decoding on this data.
            full_utt(bool): If `True`, assume this is the entire utterance, for
                            purposes of acoustic normalization.
        Raises:
            RuntimeError: If processing fails.
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
        Raises:
            RuntimeError: If processing fails.
        """
        cdef const unsigned char[:] cdata = data
        cdef int ncep = fe_get_output_size(ps_get_fe(self.ps))
        cdef int nfr = len(cdata) // (ncep * sizeof(float))
        cdef float **feats = <float **>ckd_alloc_2d_ptr(nfr, ncep, <void *>&cdata[0], sizeof(float))
        rv = ps_process_cep(self.ps, feats, nfr, no_search, full_utt)
        ckd_free(feats)
        if rv < 0:
            raise RuntimeError, "Failed to process %d frames of MFCC data" % nfr

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
            float: Posterior probability of current hypothesis.  This
            will be 1.0 unless the `bestpath` configuration option is
            enabled.

        """
        cdef logmath_t *lmath
        cdef const char *uttid
        lmath = ps_get_logmath(self.ps)
        return logmath_exp(lmath, ps_get_prob(self.ps))

    def add_word(self, str word, str phones, update=True):
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
        Returns:
            int: Word ID of added word.
        Raises:
            RuntimeError: If adding word failed for some reason.
        """
        cdef rv = ps_add_word(self.ps, word.encode("utf-8"),
                              phones.encode("utf-8"), update)
        if rv < 0:
            raise RuntimeError("Failed to add word %s" % word)

    def lookup_word(self, str word):
        """Look up a word in the dictionary and return phone transcription
        for it.

        Args:
            word(str): Text of word to search for.
        Returns:
            str: Space-separated list of phones, or None if not found.
        """
        cdef const char *cphones
        cphones = ps_lookup_word(self.ps, word.encode("utf-8"))
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
        cdef float lw

        lw = cmd_ln_float_r(self.config.cmd_ln, "-lw")
        return FsgModel.readfile(filename, self.get_logmath(), lw)

    def read_jsgf(self, str filename):
        """Read a grammar from a JSGF file.

        The top rule used is the one specified by the "toprule"
        configuration parameter.

        Args:
            filename(str): Path to JSGF file.
        Returns:
            FsgModel: Newly loaded finite-state grammar.
        """
        cdef float lw

        lw = cmd_ln_float_r(self.config.cmd_ln, "-lw")
        return FsgModel.jsgf_read_file(filename, self.get_logmath(), lw)

    def create_fsg(self, str name, int start_state, int final_state, transitions):
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
        Raises:
            ValueError: On invalid input.
        """
        cdef float lw
        cdef int wid

        lw = cmd_ln_float_r(self.config.cmd_ln, "-lw")
        lmath = self.get_logmath()
        n_state = max(itertools.chain(*((t[0], t[1]) for t in transitions))) + 1
        fsg = FsgModel(name, lmath, lw, n_state)
        fsg.set_start_state(start_state)
        fsg.set_final_state(final_state)
        for t in transitions:
            source, dest, prob = t[0:3]
            if len(t) > 3:
                word = t[3]
                wid = fsg.word_add(word)
                if wid == -1:
                    raise ValueError("Failed to add word to FSG: %s" % word)
                fsg.trans_add(source, dest,
                              lmath.log(prob), wid)
            else:
                fsg.null_trans_add(source, dest,
                                   lmath.log(prob))
        return fsg

    def parse_jsgf(self, jsgf_string, toprule=None):
        """Parse a JSGF grammar from bytes or string.

        Because PocketSphinx uses UTF-8 internally, it is more
        efficient to parse from bytes, as a string will get encoded
        and subsequently decoded.

        Args:
            jsgf_string(bytes|str): JSGF grammar as string or UTF-8
                                    encoded bytes.
            toprule(str): Name of starting rule in grammar (will
                          default to first public rule).
        Returns:
            FsgModel: Newly loaded finite-state grammar.
        Raises:
            ValueError: On failure to parse or find `toprule`.
            RuntimeError: If JSGF has no public rules.
        """
        cdef jsgf_t *jsgf
        cdef jsgf_rule_t *rule
        cdef logmath_t *lmath
        cdef float lw

        if not isinstance(jsgf_string, bytes):
            jsgf_string = jsgf_string.encode("utf-8")
        jsgf = jsgf_parse_string(jsgf_string, NULL)
        if jsgf == NULL:
            raise ValueError("Failed to parse JSGF")
        if toprule is not None:
            rule = jsgf_get_rule(jsgf, toprule.encode('utf-8'))
            if rule == NULL:
                jsgf_grammar_free(jsgf)
                raise ValueError("Failed to find top rule %s" % toprule)
        else:
            rule = jsgf_get_public_rule(jsgf)
            if rule == NULL:
                jsgf_grammar_free(jsgf)
                raise RuntimeError("No public rules found in JSGF")
        lw = cmd_ln_float_r(self.config.cmd_ln, "-lw")
        lmath = ps_get_logmath(self.ps)
        cdef fsg_model_t *cfsg = jsgf_build_fsg(jsgf, rule, lmath, lw)
        jsgf_grammar_free(jsgf)
        return FsgModel.create_from_ptr(cfsg)

    def get_fsg(self, str name):
        """Get the actual FsgModel for an FSG search.

        Args:
            name(str): Name of search module for this FSG.
        Returns:
            FsgModel: FSG corresponding to `name`, or None if not found.
        """
        cdef fsg_model_t *fsg = ps_get_fsg(self.ps, name.encode("utf-8"))
        if fsg == NULL:
            return None
        return FsgModel.create_from_ptr(fsg_model_retain(fsg))
    
    def set_fsg(self, str name, FsgModel fsg):
        """Create a search module from an FSG.

        Note that because the API is stupid, you will have to call
        `set_search(name)` in order to actually enable this FSG.

        Args:
            name(str): Search module name to associate to this FSG.
            fsg(FsgModel): Previously loaded or constructed grammar.
        Raises:
            RuntimeError: If adding FSG failed for some reason.
        """
        if ps_set_fsg(self.ps, name.encode("utf-8"), fsg.fsg) != 0:
            raise RuntimeError("Failed to set FSG in decoder")

    def set_jsgf_file(self, name, filename):
        """Create a search module from a JSGF file.

        Note that because the API is stupid, you will have to call
        `set_search(name)` in order to actually enable this grammar.

        Args:
            filename(str): Path to a JSGF file to load.
            name(str): Search module name to associate to this grammar.
        Raises:
            RuntimeError: If adding grammar failed for some reason.
        """
        if ps_set_jsgf_file(self.ps, name.encode("utf-8"),
                            filename.encode()) != 0:
            raise RuntimeError("Failed to set JSGF from %s" % filename)

    def set_jsgf_string(self, name, jsgf_string):
        """Create a search module from JSGF as bytes or string.

        Note that because the API is stupid, you will have to call
        `set_search(name)` in order to actually enable this grammar.

        Args:
            jsgf_string(bytes|str): JSGF grammar as string or UTF-8 encoded
                                    bytes.
            name(str): Search module name to associate to this grammar.
        Raises:
            ValueError: If grammar failed to parse.
        """
        if not isinstance(jsgf_string, bytes):
            jsgf_string = jsgf_string.encode("utf-8")
        if ps_set_jsgf_string(self.ps, name.encode("utf-8"), jsgf_string) != 0:
            raise ValueError("Failed to parse JSGF in decoder")

    def get_kws(self, str name):
        """Get keyphrases as text from a search module.

        Args:
            name(str): Search module name for keywords.
        Returns:
            str: List of keywords as lines (i.e. separated by '\\\\n')
        """
        return ps_get_kws(self.ps, name.encode("utf-8")).decode("utf-8")

    def set_kws(self, str name, str keyfile):
        """Create keyphrase recognition search module from a file.

        Note that because the API is stupid, you will have to call
        `set_search(name)` in order to actually enable keyphrase search.

        Args:
            name(str): Search module name to associate to these keyphrases.
            keyfile(str): Path to file with list of keyphrases (one per line).
        Raises:
            RuntimeError: If adding keyphrases failed for some reason.
        """
        cdef int rv = ps_set_kws(self.ps, name.encode("utf-8"), keyfile.encode())
        if rv < 0:
            return RuntimeError("Failed to set keyword search %s from %s"
                                % (name, keyfile))

    def set_keyphrase(self, str name, str keyphrase):
        """Create search module from a single keyphrase.

        Note that because the API is stupid, you will have to call
        `set_search(name)` in order to actually enable keyphrase search.

        Args:
            name(str): Search module name to associate to this keyphrase.
            keyphrase(str): Keyphrase to add.
        Raises:
            RuntimeError: If adding keyphrase failed for some reason.
        """
        cdef int rv = ps_set_keyphrase(self.ps, name.encode("utf-8"),
                                       keyphrase.encode("utf-8"))
        if rv < 0:
            return RuntimeError("Failed to set keyword search %s from phrase %s"
                                % (name, keyphrase))
    
    def set_allphone_file(self, str name, str lmfile = None):
        """Create a phoneme recognition search module.

        Note that because the API is stupid, you will have to call
        `set_search(name)` in order to actually enable allphone search.

        Args:
            name(str): Search module name to associate to allphone search.
            lmfile(str): Path to phoneme N-Gram file, or None to use
                         uniform probability (default is None)
        Raises:
            RuntimeError: If allphone search init failed for some reason.
        """
        cdef int rv
        if lmfile is None:
            rv = ps_set_allphone_file(self.ps, name.encode("utf-8"), NULL)
        else:
            rv = ps_set_allphone_file(self.ps, name.encode("utf-8"), lmfile.encode())
        if rv < 0:
            return RuntimeError("Failed to set allphone search %s from %s"
                                % (name, lmfile))
    def get_lattice(self):
        """Get word lattice from current recognition result.

        Returns:
            Lattice: Word lattice from current result.
        """
        cdef ps_lattice_t *lattice = ps_get_lattice(self.ps)
        if lattice == NULL:
            return None
        return Lattice.create_from_ptr(ps_lattice_retain(lattice))

    def get_config(self):
        """Get current configuration.

        Returns:
            Config: Current configuration.
        """
        return self.config

    # These two do not belong here but they're here for compatibility
    @staticmethod
    def default_config():
        """Get the default configuration.

        This does the same thing as simply creating a `Config` and is
        here for historical reasons.

        Returns:
            Config: Default configuration.
        """
        return Config()

    @staticmethod
    def file_config(str path):
        """Parse configuration from a file.

        This simply calls `Config.parse_file` and is here for historical
        reasons.

        Args:
            path(str): Path to arguments file.
        Returns:
            Config: Configuration parsed from `path`.
        """
        return Config.parse_file(path)
    
    def load_dict(self, str dict_path, str fdict_path = None, str _format = None):
        """Load dictionary (and possibly noise dictionary) from a file.

        Note that the `format` argument does nothing, never has done
        anything, and never will.  It's only here for historical
        reasons.

        Args:
            dict_path(str): Path to pronunciation dictionary file.
            fdict_path(str): Path to noise dictionary file, or None to keep
                             existing one (default is None)
            _format(str): Useless argument that does nothing.
        Raises:
            RuntimeError: If dictionary loading failed for some reason.
        """
        cdef int rv
        # THIS IS VERY ANNOYING, CYTHON
        cdef const char *cformat = NULL
        cdef const char *cdict = NULL
        cdef const char *cfdict = NULL
        if _format is not None:
            spam = _format.encode("utf-8")
            cformat = spam
        if dict_path is not None:
            eggs = dict_path.encode()
            cdict = eggs
        if fdict_path is not None:
            bacon = fdict_path.encode()
            cfdict = bacon
        rv = ps_load_dict(self.ps, cdict, cfdict, cformat)
        if rv < 0:
            raise RuntimeError("Failed to load dictionary from %s and %s"
                               % (dict_path, fdict_path))

    def save_dict(self, str dict_path, str _format = None):
        """Save dictionary to a file.

        Note that the `format` argument does nothing, never has done
        anything, and never will.  It's only here for historical
        reasons.

        Args:
            dict_path(str): Path to save pronunciation dictionary in.
            _format(str): Useless argument that does nothing.
        Raises:
            RuntimeError: If dictionary saving failed for some reason.
        """
        cdef int rv
        cdef const char *cformat = NULL
        cdef const char *cdict = NULL
        if _format is not None:
            spam = _format.encode("utf-8")
            cformat = spam
        if dict_path is not None:
            eggs = dict_path.encode()
            cdict = eggs
        rv = ps_save_dict(self.ps, cdict, cformat)
        if rv < 0:
            raise RuntimeError("Failed to save dictionary to %s" % dict_path)

    def get_lm(self, str name):
        """Get the N-Gram language model associated with a search module.

        Args:
            name(str): Name of search module for this language model.
        Returns:
            NGramModel: Model corresponding to `name`, or None if not found.
        """
        cdef ngram_model_t *lm = ps_get_lm(self.ps, name.encode("utf-8"))
        if lm == NULL:
            return None
        return NGramModel.create_from_ptr(ngram_model_retain(lm))

    def set_lm(self, str name, NGramModel lm):
        """Create a search module for an N-Gram language model.

        Note that because the API is stupid, you will have to call
        `set_search(name)` in order to actually enable this LM.

        Args:
            name(str): Search module name to associate to this LM.
            lm(NGramModel): Previously loaded language model.
        Raises:
            RuntimeError: If adding LM failed for some reason.
        """
        cdef int rv = ps_set_lm(self.ps, name.encode("utf-8"), lm.lm)
        if rv < 0:
            raise RuntimeError("Failed to set language model %s" % name)

    def set_lm_file(self, str name, str path):
        """Load a language model from a file into the decoder.

        Note that because the API is stupid, you will have to call
        `set_search(name)` in order to actually enable this LM.

        Args:
            name(str): Search module name to associate to this LM.
            path(str): Path to N-Gram language model file.
        Raises:
            RuntimeError: If adding LM failed for some reason.
        """
        cdef int rv = ps_set_lm_file(self.ps, name.encode("utf-8"), path.encode())
        if rv < 0:
            raise RuntimeError("Failed to set language model %s from %s"
                               % (name, path))

    def get_logmath(self):
        """Get the LogMath object for this decoder.

        Returns:
            LogMath: Current log-math computation object.
        """
        cdef logmath_t *lmath = ps_get_logmath(self.ps)
        return LogMath.create_from_ptr(logmath_retain(lmath))

    def set_search(self, str search_name):
        """Actually use a language model or grammar you loaded.

        This activates a "search module" that was created with one of
        the very badly named functions `set_fsg`, `set_lm`,
        `set_lm_file`, `set_allphone_file`, `set_keyphrase`,
        `set_kws`, or `set_align`.

        I might be the one responsible for this bad API. Sorry.

        Args:
            search_name(str): Name of search module to activate.
        Raises:
            KeyError: If `search_name` doesn't actually exist.
        """
        cdef int rv = ps_set_search(self.ps, search_name.encode("utf-8"))
        if rv < 0:
            raise KeyError("Unable to set search %s" % search_name)

    def unset_search(self, str search_name):
        """Remove a search (LM, grammar, etc) freeing resources.

        What a dumb name for a method.  Oh well.

        Args:
            search_name(str): Name of search module to remove.
        Raises:
            KeyError: If `search_name` doesn't actually exist.
        """
        cdef int rv = ps_unset_search(self.ps, search_name.encode("utf-8"))
        if rv < 0:
            raise KeyError("Unable to unset search %s" % search_name)

    def get_search(self):
        """Get the name of the current search (LM, grammar, etc).

        This is pretty important if you want to get the current LM or
        FSG or what have you.

        Returns:
            str: Name of currently active search module.
        """
        return ps_get_search(self.ps).decode("utf-8")

    def n_frames(self):
        """Get the number of frames processed up to this point.

        Returns:
            int: Like it says.
        """
        return ps_get_n_frames(self.ps)

cdef class Vad:
    """Voice activity detection class.

    Args:
      mode(int): Aggressiveness of voice activity detction (0-3)
      sample_rate(int): Sampling rate of input, default is 16000
                          (only 8000, 16000, 32000, and 48000 are
                          supported for the moment)
      frame_length(float): Input frame length in seconds, default
                           is 0.03 (only 0.01, 0.02, and 0.03 are
                           supported for the moment)

    Attributes:
      sample_rate(int): Sampling rate for input (default is 16000)
      frame_bytes(int): Number of bytes in a frame (default
                        is 960, i.e. 30ms of 16-bit samples at 16kHz)
      frame_length(float): Frame length in seconds (default is 0.03)
    """
    cdef ps_vad_t *_vad
    cdef public int sample_rate
    cdef public size_t frame_bytes
    cdef public float frame_length
    LOOSE = PS_VAD_LOOSE
    MEDIUM_LOOSE = PS_VAD_MEDIUM_LOOSE
    MEDIUM_STRICT = PS_VAD_MEDIUM_STRICT
    STRICT = PS_VAD_STRICT
    DEFAULT_SAMPLE_RATE = PS_VAD_DEFAULT_SAMPLE_RATE
    DEFAULT_FRAME_LENGTH = PS_VAD_DEFAULT_FRAME_LENGTH
    
    def __init__(self, mode=PS_VAD_LOOSE,
                 sample_rate=PS_VAD_DEFAULT_SAMPLE_RATE,
                 frame_length=PS_VAD_DEFAULT_FRAME_LENGTH):
        self._vad = ps_vad_init(mode, sample_rate, frame_length)
        if self._vad == NULL:
            raise ValueError("Invalid VAD parameters")
        self.sample_rate = sample_rate
        self.frame_length = frame_length
        self.frame_bytes = ps_vad_frame_size(self._vad) * 2

    def __dealloc__(self):
        ps_vad_free(self._vad)

    def is_speech(self, buf, sample_rate=None):
        """Classify a buffer as speech or not.
        
        Args:
          buf(bytes): Buffer containing speech data (16-bit signed integers)
        Returns:
          (boolean) Classification as speech or not speech.

        """
        cdef const unsigned char[:] cbuf = buf
        cdef Py_ssize_t n_samples = len(cbuf) // 2
        if len(cbuf) != self.frame_bytes:
            raise ValueError("Frame size must be %d bytes" % self.frame_bytes)
        rv = ps_vad_classify(self._vad, <const short *>&cbuf[0])
        if rv < 0:
            raise ValueError("VAD classification failed")
        return rv == PS_VAD_SPEECH
