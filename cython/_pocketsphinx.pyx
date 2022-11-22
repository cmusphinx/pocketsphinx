# cython: embedsignature=True, language_level=3
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
import pocketsphinx
import warnings
import os
cimport _pocketsphinx

LOGGER = logging.getLogger("pocketsphinx")

cdef class Config:
    """Configuration object for PocketSphinx.

    The PocketSphinx recognizer can be configured either implicitly,
    by passing keyword arguments to `Decoder`, or by creating and
    manipulating `Config` objects.  There are a large number of
    parameters, most of which are not important or subject to change.

    A `Config` can be initialized with keyword arguments::

        config = Config(hmm="path/to/things", dict="my.dict")

    It can also be initialized by parsing JSON (either as bytes or str)::

        config = Config.parse_json('''{"hmm": "path/to/things",
                                       "dict": "my.dict"}''')

    The "parser" is very much not strict, so you can also pass a sort
    of pseudo-YAML to it, e.g.::

        config = Config.parse_json("hmm: path/to/things, dict: my.dict")

    You can also initialize an empty `Config` and set arguments in it
    directly::

        config = Config()
        config["hmm"] = "path/to/things"

    In general, a `Config` mostly acts like a dictionary, and can be
    iterated over in the same fashion.  However, attempting to access
    a parameter that does not already exist will raise a `KeyError`.

    Many parameters have default values.  Also, when constructing a
    `Config` directly (as opposed to parsing JSON), `hmm`, `lm`, and
    `dict` are set to the default models (some kind of US English
    models of unknown origin + CMUDict). You can prevent this by
    passing `None` for any of these parameters, e.g.::

        config = Config(lm=None)  # Do not load a language model

    Decoder initialization **will fail** if more than one of `lm`,
    `jsgf`, `fsg`, `keyphrase`, `kws`, `allphone`, or `lmctl` are set
    in the configuration.  To make life easier, and because there is
    no possible case in which you would do this intentionally, if you
    initialize a `Decoder` or `Config` with any of these (and not
    `lm`), the default `lm` value will be removed.  This is not the
    case if you decide to set one of them in an existing `Config`, so
    in that case you must make sure to set `lm` to `None`::

        config["jsgf"] = "spam_eggs_and_spam.gram"
        config["lm"] = None

    You may also call `default_search_args()` after the fact to set
    `hmm`, `lm`, and `dict` to the system defaults.  Note that this
    will set them unconditionally.

    See :doc:`config_params` for a description of existing parameters.

 """
    cdef ps_config_t *config

    # This is __init__ so we can bypass it if necessary
    def __init__(self, *args, **kwargs):
        cdef char **argv
        # Undocumented command-line parsing
        if args:
            args = [str(k).encode('utf-8')
                    for k in args]
            argv = <char **> malloc((len(args) + 1) * sizeof(char *))
            argv[len(args)] = NULL
            for i, buf in enumerate(args):
                if buf is None:
                    argv[i] = NULL
                else:
                    argv[i] = buf
            self.config = ps_config_parse_args(NULL, len(args), argv)
            free(argv)
        else:
            self.config = ps_config_init(NULL)
        # Set default search arguments
        self.default_search_args()
        # Now override them from kwargs (including None)
        if kwargs:
            # Remove lm if a different search was specified
            for s in ("jsgf", "fsg", "kws", "keyphrase",
                      "allphone", "lmctl"):
                if s in kwargs:
                    ps_config_set_str(self.config, "lm", NULL)
                    break
            for k, v in kwargs.items():
                # Note that all this is quite inefficient as we end up
                # calling _normalize_key repeatedly.
                ckey = self._normalize_key(k)
                # Special dispensation to support the thing which was
                # documented but never actually worked, i.e. setting a
                # string value to False (should be None) to remove the
                # default.
                if ps_config_typeof(self.config, ckey) & ARG_STRING:
                    if v is False:
                        v = None
                self[ckey] = v

    def default_search_args(self):
        """Set arguments for the default acoustic and language model.

        Set `hmm`, `lm`, and `dict` to the default ones (some kind of
        US English models of unknown origin + CMUDict).  This will
        overwrite any previous values for these parameters, and does
        not check if the files exist.
        """
        default_am = pocketsphinx.get_model_path("en-us/en-us")
        self.set_string("hmm", default_am)
        default_lm = pocketsphinx.get_model_path("en-us/en-us.lm.bin")
        self.set_string("lm", default_lm)
        default_dict = pocketsphinx.get_model_path("en-us/cmudict-en-us.dict")
        self.set_string("dict", default_dict)

    @staticmethod
    cdef create_from_ptr(ps_config_t *config):
        cdef Config self = Config.__new__(Config)
        self.config = config
        return self

    @staticmethod
    def parse_file(str path):
        """DEPRECATED: Parse a config file.

        This reads a configuration file in "command-line" format, for example::

            -arg1 value -arg2 value
            -arg3 value

        Args:
            path(str): Path to configuration file.
        Returns:
            Config: Parsed config, or None on error.
        """
        cdef ps_config_t *config = cmd_ln_parse_file_r(NULL, ps_args(),
                                                       path.encode(), False)
        warnings.warn("parse_file() is deprecated, use JSON configuration instead",
                      DeprecationWarning)
        if config == NULL:
            return None
        return Config.create_from_ptr(config)

    @staticmethod
    def parse_json(json):
        """Parse JSON (or pseudo-YAML) configuration

        Args:
            json(bytes|str): JSON data.
        Returns:
            Config: Parsed config, or None on error.
        """
        cdef ps_config_t *config
        if not isinstance(json, bytes):
            json = json.encode("utf-8")
        config = ps_config_parse_json(NULL, json)
        if config == NULL:
            return None
        return Config.create_from_ptr(config)

    def dumps(self):
        """Serialize configuration to a JSON-formatted `str`.

        This produces JSON from a configuration object, with default
        values included.

        Returns:
            str: Serialized JSON
        Raises:
            RuntimeError: if serialization fails somehow.
        """
        cdef const char *json = ps_config_serialize_json(self.config)
        if json == NULL:
            raise RuntimeError("JSON serialization failed")
        return json.decode("utf-8")

    def __dealloc__(self):
        ps_config_free(self.config)

    def get_float(self, key):
        return ps_config_float(self.config, self._normalize_key(key))

    def get_int(self, key):
        return ps_config_int(self.config, self._normalize_key(key))

    def get_string(self, key):
        cdef const char *val = ps_config_str(self.config,
                                             self._normalize_key(key))
        if val == NULL:
            return None
        else:
            return val.decode('utf-8')

    def get_boolean(self, key):
        return ps_config_bool(self.config, self._normalize_key(key))

    def set_float(self, key, double val):
        ps_config_set_float(self.config, self._normalize_key(key), val)

    def set_int(self, key, long val):
        ps_config_set_int(self.config, self._normalize_key(key), val)

    def set_boolean(self, key, val):
        ps_config_set_bool(self.config, self._normalize_key(key), bool(val))

    def set_string(self, key, val):
        if val == None:
            ps_config_set_str(self.config, self._normalize_key(key), NULL)
        else:
            ps_config_set_str(self.config, self._normalize_key(key), val.encode('utf-8'))

    def set_string_extra(self, key, val):
        if val == None:
            cmd_ln_set_str_extra_r(self.config, self._normalize_key(key), NULL)
        else:
            cmd_ln_set_str_extra_r(self.config, self._normalize_key(key), val.encode('utf-8'))

    def exists(self, key):
        return key in self

    cdef _normalize_key(self, key):
        if isinstance(key, bytes):
            # Assume already normalized
            return key
        else:
            if key[0] in "-_":
                key = key[1:]
            return key.encode('utf-8')

    def __contains__(self, key):
        return ps_config_typeof(self.config, self._normalize_key(key)) != 0

    def __getitem__(self, key):
        cdef const char *cval
        cdef const anytype_t *at;
        cdef int t

        ckey = self._normalize_key(key)
        at = ps_config_get(self.config, ckey)
        if at == NULL:
            raise KeyError("Unknown key %s" % key)
        t = ps_config_typeof(self.config, ckey)
        if t & ARG_STRING:
            cval = <const char *>at.ptr
            if cval == NULL:
                return None
            else:
                return cval.decode('utf-8')
        elif t & ARG_INTEGER:
            return at.i
        elif t & ARG_FLOATING:
            return at.fl
        elif t & ARG_BOOLEAN:
            return bool(at.i)
        else:
            raise ValueError("Unable to handle parameter type %d" % t)

    def __setitem__(self, key, val):
        cdef int t
        ckey = self._normalize_key(key)
        t = ps_config_typeof(self.config, ckey)
        if t == 0:
            raise KeyError("Unknown key %s" % key)
        if t & ARG_STRING:
            if val is None:
                ps_config_set_str(self.config, ckey, NULL)
            else:
                ps_config_set_str(self.config, ckey, str(val).encode('utf-8'))
        elif t & ARG_INTEGER:
            ps_config_set_int(self.config, ckey, int(val))
        elif t & ARG_FLOATING:
            ps_config_set_float(self.config, ckey, float(val))
        elif t & ARG_BOOLEAN:
            ps_config_set_bool(self.config, ckey, bool(val))
        else:
            raise ValueError("Unable to handle parameter type %d" % t)

    def __iter__(self):
        cdef hash_table_t *ht = self.config.ht
        cdef hash_iter_t *itor
        itor = hash_table_iter(self.config.ht)
        while itor != NULL:
            ckey = hash_entry_key(itor.ent)
            yield ckey.decode('utf-8')
            itor = hash_table_iter_next(itor)

    def items(self):
        for key in self:
            yield (key, self[key])

    def __len__(self):
        # Incredibly, the only way to do this
        return sum(1 for _ in self)

    def describe(self):
        """Iterate over parameter descriptions.

        This function returns a generator over the parameters defined
        in a configuration, as `Arg` objects.

        Returns:
            Iterable[Arg]: Descriptions of parameters including their
            default values and documentation

        """
        cdef const ps_arg_t *arg = self.config.defn
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
            yield pocketsphinx.Arg(name=name, default=default, doc=doc,
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
      end_frame(int): Index of end frame (inclusive!)
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
        cdef ngram_model_t *lm = ngram_model_read(config.config,
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

    def accept(self, words):
        return fsg_model_accept(self.fsg, words.encode("utf-8")) != 0

    def word_add(self, word):
        if not isinstance(word, bytes):
            word = word.encode("utf-8")
        return fsg_model_word_add(self.fsg, word)

    def set_start_state(self, state):
        self.fsg.start_state = state

    def set_final_state(self, state):
        self.fsg.final_state = state

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

    Note that, as described in `Config`, `hmm`, `lm`, and `dict` are
    set to the default ones (some kind of US English models of unknown
    origin + CMUDict) if not defined.  You can prevent this by passing
    `None` for any of these parameters, e.g.::

        ps = Decoder(lm=None)  # Do not load a language model

    Decoder initialization **will fail** if more than one of `lm`,
    `jsgf`, `fsg`, `keyphrase`, `kws`, `allphone`, or `lmctl` are set
    in the configuration.  To make life easier, and because there is
    no possible case in which you would do this intentionally, if you
    initialize a `Decoder` or `Config` with any of these (and not
    `lm`), the default `lm` value will be removed.

    You can also pass a pre-defined `Config` object as the only
    argument to the constructor, e.g.::

        config = Config.parse_json(json)
        ps = Decoder(config)

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
        samprate(int): Sampling rate for raw audio data.
        loglevel(str): Logging level, one of "INFO", "ERROR", "FATAL".
        logfn(str): File to write log messages to.
    Raises:
        ValueError: On invalid configuration or argument list.
        RuntimeError: On invalid configuration or other failure to
                      reinitialize decoder.
    """
    cdef ps_decoder_t *_ps
    cdef Config _config

    def __init__(self, *args, **kwargs):
        if len(args) == 1 and isinstance(args[0], Config):
            self._config = args[0]
        else:
            self._config = Config(*args, **kwargs)
        if self._config is None:
            raise ValueError, "Failed to parse argument list"
        self._ps = ps_init(self._config.config)
        if self._ps == NULL:
            raise RuntimeError, "Failed to initialize PocketSphinx"

    def __dealloc__(self):
        ps_free(self._ps)

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
        cdef ps_config_t *cconfig
        if config is None:
            cconfig = NULL
        else:
            self._config = config
            cconfig = config.config
        if ps_reinit(self._ps, cconfig) != 0:
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
        cdef ps_config_t *cconfig
        if config is None:
            cconfig = NULL
        else:
            self._config = config
            cconfig = config.config
        if ps_reinit_feat(self._ps, cconfig) < 0:
            raise RuntimeError("Failed to reinitialize feature extraction")

    def get_cmn(self, update=False):
        """Get current cepstral mean.

        Args:
          update(boolean): Update the mean based on current utterance.
        Returns:
          str: Cepstral mean as a comma-separated list of numbers.
        """
        cdef const char *cmn = ps_get_cmn(self._ps, update)
        return cmn.decode("utf-8")

    def set_cmn(self, cmn):
        """Get current cepstral mean.

        Args:
          cmn(str): Cepstral mean as a comma-separated list of numbers.
        """
        cdef int rv = ps_set_cmn(self._ps, cmn.encode("utf-8"))
        if rv != 0:
            raise ValueError("Invalid CMN string")

    def start_stream(self):
        """Reset noise statistics.

        This method can be called at the beginning of a new audio
        stream (but this is not necessary)."""
        cdef int rv = ps_start_stream(self._ps)
        warnings.warn("start_stream() is deprecated and unnecessary",
                      DeprecationWarning)
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
        if ps_start_utt(self._ps) < 0:
            raise RuntimeError, "Failed to start utterance processing"

    def get_in_speech(self):
        """Return speech status.

        This method is retained for compatibility, but it will always
        return True as long as `ps_start_utt` has been previously
        called.
        """
        warnings.warn("get_in_speech() is deprecated and does nothing useful",
                      DeprecationWarning)
        return ps_get_in_speech(self._ps)

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
        if ps_process_raw(self._ps, <const short *>&cdata[0],
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
        cdef int ncep = self._config["ceplen"]
        cdef int nfr = len(cdata) // (ncep * sizeof(float))
        cdef float **feats = <float **>ckd_alloc_2d_ptr(nfr, ncep, <void *>&cdata[0], sizeof(float))
        rv = ps_process_cep(self._ps, feats, nfr, no_search, full_utt)
        ckd_free(feats)
        if rv < 0:
            raise RuntimeError, "Failed to process %d frames of MFCC data" % nfr

    def end_utt(self):
        """Finish processing raw audio input.

        This method must be called at the end of each separate
        "utterance" of raw audio input.  It takes care of flushing any
        internal buffers and finalizing recognition results.

        """
        if ps_end_utt(self._ps) < 0:
            raise RuntimeError, "Failed to stop utterance processing"

    def hyp(self):
        """Get current recognition hypothesis.

        Returns:
            Hypothesis: Current recognition output.
        """
        cdef const char *hyp
        cdef logmath_t *lmath
        cdef int score

        hyp = ps_get_hyp(self._ps, &score)
        if hyp == NULL:
             return None
        lmath = ps_get_logmath(self._ps)
        prob = ps_get_prob(self._ps)
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
        lmath = ps_get_logmath(self._ps)
        return logmath_exp(lmath, ps_get_prob(self._ps))

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
        cdef rv = ps_add_word(self._ps, word.encode("utf-8"),
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
        cphones = ps_lookup_word(self._ps, word.encode("utf-8"))
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
        itor = ps_seg_iter(self._ps)
        if itor == NULL:
            return
        lmath = ps_get_logmath(self._ps)
        return SegmentList.create(itor, lmath)


    def nbest(self):
        """Get N-Best hypotheses.

        Returns:
            Iterable[Hypothesis]: Generator over N-Best recognition results
        """
        cdef ps_nbest_t *itor
        cdef logmath_t *lmath
        itor = ps_nbest(self._ps)
        if itor == NULL:
            return
        lmath = ps_get_logmath(self._ps)
        return NBestList.create(itor, lmath)


    def read_fsg(self, filename):
        """Read a grammar from an FSG file.

        Args:
            filename(str): Path to FSG file.

        Returns:
            FsgModel: Newly loaded finite-state grammar.
        """
        cdef float lw

        lw = ps_config_float(self._config.config, "lw")
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

        lw = ps_config_float(self._config.config, "lw")
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

        lw = ps_config_float(self._config.config, "lw")
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
        lw = ps_config_float(self._config.config, "lw")
        lmath = ps_get_logmath(self._ps)
        cdef fsg_model_t *cfsg = jsgf_build_fsg(jsgf, rule, lmath, lw)
        jsgf_grammar_free(jsgf)
        return FsgModel.create_from_ptr(cfsg)

    def get_fsg(self, str name = None):
        """Get the currently active FsgModel or the model for a
        specific search module.

        Args:
            name(str): Name of search module for this FSG.  If this is
                       None (the default), the currently active FSG will be
                       returned.
        Returns:
            FsgModel: FSG corresponding to `name`, or None if not found.
        """
        cdef fsg_model_t *fsg
        if name is None:
            fsg = ps_get_fsg(self._ps, NULL)
        else:
            fsg = ps_get_fsg(self._ps, name.encode("utf-8"))
        if fsg == NULL:
            return None
        else:
            return FsgModel.create_from_ptr(fsg_model_retain(fsg))

    def add_fsg(self, str name, FsgModel fsg):
        """Create (but do not activate) a search module for a finite-state
        grammar.

        Args:
            name(str): Search module name to associate to this FSG.
            fsg(FsgModel): Previously loaded or constructed grammar.
        Raises:
            RuntimeError: If adding FSG failed for some reason.

        """
        if ps_add_fsg(self._ps, name.encode("utf-8"), fsg.fsg) != 0:
            raise RuntimeError("Failed to set FSG in decoder")

    def set_fsg(self, str name, FsgModel fsg):
        warnings.warn("set_fsg() is deprecated, use add_fsg() instead",
                      DeprecationWarning)
        self.add_fsg(name, fsg)

    def add_jsgf_file(self, name, filename):
        """Create (but do not activate) a search module from a JSGF file.

        Args:
            filename(str): Path to a JSGF file to load.
            name(str): Search module name to associate to this grammar.
        Raises:
            RuntimeError: If adding grammar failed for some reason.
        """
        if ps_add_jsgf_file(self._ps, name.encode("utf-8"),
                            filename.encode()) != 0:
            raise RuntimeError("Failed to set JSGF from %s" % filename)

    def set_jsgf_file(self, name, filename):
        warnings.warn("set_jsgf_file() is deprecated, use add_jsgf_file() instead",
                      DeprecationWarning)
        self.add_jsgf_file(name, filename)

    def add_jsgf_string(self, name, jsgf_string):
        """Create (but do not activate) a search module from JSGF
        as bytes or string.

        Args:
            jsgf_string(bytes|str): JSGF grammar as string or UTF-8 encoded
                                    bytes.
            name(str): Search module name to associate to this grammar.
        Raises:
            ValueError: If grammar failed to parse.
        """
        if not isinstance(jsgf_string, bytes):
            jsgf_string = jsgf_string.encode("utf-8")
        if ps_add_jsgf_string(self._ps, name.encode("utf-8"), jsgf_string) != 0:
            raise ValueError("Failed to parse JSGF in decoder")

    def set_jsgf_string(self, name, jsgf_string):
        warnings.warn("set_jsgf_string() is deprecated, use add_jsgf_string() instead",
                      DeprecationWarning)
        self.add_jsgf_string(name, jsgf_string)

    def get_kws(self, str name = None):
        """Get keyphrases as text from current or specified search module.

        Args:
            name(str): Search module name for keywords.  If this is
            None, the currently active keywords are returned if
            keyword search is active.
        Returns:
            str: List of keywords as lines (i.e. separated by '\\\\n'),
            or None if the specified search could not be found, or if
            `name` is None and keyword search is not currently active.
        """
        cdef const char *kws
        if name is None:
            kws = ps_get_kws(self._ps, NULL)
        else:
            kws = ps_get_kws(self._ps, name.encode("utf-8"))
        if kws == NULL:
            return None
        else:
            return kws.decode("utf-8")

    def add_kws(self, str name, str keyfile):
        """Create (but do not activate) keyphrase recognition search module
        from a file.

        Args:
            name(str): Search module name to associate to these keyphrases.
            keyfile(str): Path to file with list of keyphrases (one per line).
        Raises:
            RuntimeError: If adding keyphrases failed for some reason.
        """
        cdef int rv = ps_add_kws(self._ps, name.encode("utf-8"), keyfile.encode())
        if rv < 0:
            return RuntimeError("Failed to set keyword search %s from %s"
                                % (name, keyfile))

    def set_kws(self, str name, str keyfile):
        warnings.warn("set_kws() is deprecated, use add_kws() instead",
                      DeprecationWarning)
        self.add_kws(name, keyfile)

    def add_keyphrase(self, str name, str keyphrase):
        """Create (but do not activate) search module from a single keyphrase.

        Args:
            name(str): Search module name to associate to this keyphrase.
            keyphrase(str): Keyphrase to add.
        Raises:
            RuntimeError: If adding keyphrase failed for some reason.
        """
        cdef int rv = ps_add_keyphrase(self._ps, name.encode("utf-8"),
                                       keyphrase.encode("utf-8"))
        if rv < 0:
            return RuntimeError("Failed to set keyword search %s from phrase %s"
                                % (name, keyphrase))

    def set_keyphrase(self, str name, str keyphrase):
        warnings.warn("set_keyphrase() is deprecated, use add_keyphrase() instead",
                      DeprecationWarning)
        self.add_keyphrase(name, keyphrase)

    def add_allphone_file(self, str name, str lmfile = None):
        """Create (but do not activate) a phoneme recognition search module.

        Args:
            name(str): Search module name to associate to allphone search.
            lmfile(str): Path to phoneme N-Gram file, or None to use
                         uniform probability (default is None)
        Raises:
            RuntimeError: If allphone search init failed for some reason.
        """
        cdef int rv
        if lmfile is None:
            rv = ps_add_allphone_file(self._ps, name.encode("utf-8"), NULL)
        else:
            rv = ps_add_allphone_file(self._ps, name.encode("utf-8"), lmfile.encode())
        if rv < 0:
            return RuntimeError("Failed to set allphone search %s from %s"
                                % (name, lmfile))

    def set_allphone_file(self, str name, str keyfile):
        warnings.warn("set_allphone_file() is deprecated, use add_allphone_file() instead",
                      DeprecationWarning)
        self.add_allphone_file(name, keyfile)

    def get_lattice(self):
        """Get word lattice from current recognition result.

        Returns:
            Lattice: Word lattice from current result.
        """
        cdef ps_lattice_t *lattice = ps_get_lattice(self._ps)
        if lattice == NULL:
            return None
        return Lattice.create_from_ptr(ps_lattice_retain(lattice))

    @property
    def config(self):
        """Read-only property containing configuration object."""
        return self._config

    def get_config(self):
        """Get current configuration.

        DEPRECATED: This does the same thing as simply accessing
        `config` and is here for historical reasons.

        Returns:
            Config: Current configuration.

        """
        return self._config

    # These two do not belong here but they're here for compatibility
    @staticmethod
    def default_config():
        """Get the default configuration.

        DEPRECATED: This does the same thing as simply creating a
        `Config` and is here for historical reasons.

        Returns:
            Config: Default configuration.
        """
        warnings.warn("default_config() is deprecated, just call Config() constructor",
                      DeprecationWarning)
        return Config()

    @staticmethod
    def file_config(str path):
        """Parse configuration from a file.

        DEPRECATED: This simply calls `Config.parse_file` and is here
        for historical reasons.

        Args:
            path(str): Path to arguments file.
        Returns:
            Config: Configuration parsed from `path`.
        """
        warnings.warn("file_config() is deprecated, use JSON configuration please",
                      DeprecationWarning)
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
        rv = ps_load_dict(self._ps, cdict, cfdict, cformat)
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
        rv = ps_save_dict(self._ps, cdict, cformat)
        if rv < 0:
            raise RuntimeError("Failed to save dictionary to %s" % dict_path)

    def get_lm(self, str name = None):
        """Get the current N-Gram language model or the one associated with a
        search module.

        Args:
            name(str): Name of search module for this language model.  If this
                       is None (default) the current LM will be returned.
        Returns:
            NGramModel: Model corresponding to `name`, or None if not found.

        """
        cdef ngram_model_t *lm
        if name is None:
            lm = ps_get_lm(self._ps, NULL)
        else:
            lm = ps_get_lm(self._ps, name.encode("utf-8"))
        if lm == NULL:
            return None
        return NGramModel.create_from_ptr(ngram_model_retain(lm))

    def add_lm(self, str name, NGramModel lm):
        """Create (but do not activate) a search module for an N-Gram language
        model.

        Args:
            name(str): Search module name to associate to this LM.
            lm(NGramModel): Previously loaded language model.
        Raises:
            RuntimeError: If adding LM failed for some reason.
        """
        cdef int rv = ps_add_lm(self._ps, name.encode("utf-8"), lm.lm)
        if rv < 0:
            raise RuntimeError("Failed to set language model %s" % name)

    def set_lm(self, str name, NGramModel lm):
        warnings.warn("set_lm() is deprecated, use add_lm() instead",
                      DeprecationWarning)
        self.add_lm(name, lm)

    def add_lm_file(self, str name, str path):
        """Load (but do not activate a language model from a file into the
        decoder.

        Args:
            name(str): Search module name to associate to this LM.
            path(str): Path to N-Gram language model file.
        Raises:
            RuntimeError: If adding LM failed for some reason.
        """
        cdef int rv = ps_add_lm_file(self._ps, name.encode("utf-8"), path.encode())
        if rv < 0:
            raise RuntimeError("Failed to set language model %s from %s"
                               % (name, path))

    def set_lm_file(self, str name, str path):
        warnings.warn("set_lm_file() is deprecated, use add_lm_file() instead",
                      DeprecationWarning)
        self.add_lm_file(name, path)

    @property
    def logmath(self):
        """Read-only property containing LogMath object for this decoder."""
        return self.get_logmath()

    def get_logmath(self):
        """Get the LogMath object for this decoder.

        DEPRECATED: This does the same thing as simply accessing
        `logmath` and is here for historical reasons.

        Returns:
            LogMath: Current log-math computation object.
        """
        cdef logmath_t *lmath = ps_get_logmath(self._ps)
        return LogMath.create_from_ptr(logmath_retain(lmath))

    def activate_search(self, str search_name = None):
        """Activate a search module

        This activates a "search module" that was created with the
        methods `add_fsg`, `add_lm`, `add_lm_file`,
        `add_allphone_file`, `add_keyphrase`, or `add_kws`.

        This API is still bad, but at least the method names make
        sense now.

        Args:
            search_name(str): Name of search module to activate.  If
            None (or not given), then the default search module, the
            one created with the Decoder, for instance, will be
            (re-)activated.

        Raises:
            KeyError: If `search_name` doesn't actually exist.

        """
        cdef int rv
        if search_name is None:
            rv = ps_activate_search(self._ps, NULL)
        else:
            rv = ps_activate_search(self._ps, search_name.encode("utf-8"))
        if rv < 0:
            raise KeyError("Unable to set search %s" % search_name)

    def set_search(self, str search_name):
        warnings.warn("set_search() is deprecated, use activate_search() instead",
                      DeprecationWarning)
        self.activate_search(search_name)

    def remove_search(self, str search_name):
        """Remove a search (LM, grammar, etc) freeing resources.

        Args:
            search_name(str): Name of search module to remove.
        Raises:
            KeyError: If `search_name` doesn't actually exist.
        """
        cdef int rv = ps_remove_search(self._ps, search_name.encode("utf-8"))
        if rv < 0:
            raise KeyError("Unable to unset search %s" % search_name)

    def unset_search(self, str search_name):
        warnings.warn("unset_search() is deprecated, use remove_search() instead",
                      DeprecationWarning)
        self.remove_search(search_name)

    def current_search(self):
        """Get the name of the current search (LM, grammar, etc).

        Returns:
            str: Name of currently active search module.
        """
        return ps_current_search(self._ps).decode("utf-8")

    def get_search(self):
        warnings.warn("get_search() is deprecated, use current_search() instead",
                      DeprecationWarning)
        return self.current_search()

    def set_align_text(self, text):
        """Set a word sequence for alignment *and* enable alignment mode.

        Unlike the `add_*` methods and the deprecated, badly-named
        `set_*` methods, this really does immediately enable the
        resulting search module.  This is because alignment is
        typically a one-shot deal, i.e. you are not likely to create a
        list of different alignments and keep them around.  If you
        really want to do that, perhaps you should use FSG search
        instead.  Or let me know and perhaps I'll add an
        `add_align_text` method.

        You must do any text normalization yourself.  For word-level
        alignment, once you call this, simply decode and get the
        segmentation in the usual manner.  For phone-level alignment,
        see `set_alignment` and `get_alignment`.

        Args:
            text(str): Sentence to align, as whitespace-separated
                       words.  All words must be present in the
                       dictionary.
        Raises:
            RuntimeError: If text is invalid somehow.
        """
        cdef int rv = ps_set_align_text(self._ps, text.encode("utf-8"))
        if rv < 0:
            raise RuntimeError("Failed to set up alignment of %s" % (text))

    def set_alignment(self, Alignment alignment = None):
        """Set up *and* activate sub-word alignment mode.

        For efficiency reasons, decoding and word-level alignment (as
        done by `set_align_text`) do not track alignments at the
        sub-word level.  This is fine for a lot of use cases, but
        obviously not all of them.  If you want to obtain phone or
        state level alignments, you must run a second pass of
        alignment, which is what this function sets you up to do.  The
        sequence is something like this::

            decoder.set_align_text("hello world")
            decoder.start_utt()
            decoder.process_raw(data, full_utt=True)
            decoder.end_utt()
            decoder.set_alignment()
            decoder.start_utt()
            decoder.process_raw(data, full_utt=True)
            decoder.end_utt()
            for word in decoder.get_alignment():
                for phone in word:
                    for state in phone:
                        print(word, phone, state)

        That's a lot of code, so it may get simplified, either here or
        in a derived class, before release.

        Note that if you are using this with N-Gram or FSG decoding,
        you can restore the default search module afterwards by
        calling activate_search() with no argument.

        Args:
            alignment(Alignment): Pre-constructed `Alignment` object.
                  Currently you can't actually do anything with this.
        Raises:
            RuntimeError: If current hypothesis cannot be aligned (such
                          as when using keyphrase or allphone search).

        """
        cdef int rv
        if alignment is not None:
            rv = ps_set_alignment(self._ps, alignment._al)
        else:
            rv = ps_set_alignment(self._ps, NULL)
        if rv < 0:
            raise RuntimeError("Failed to set up sub-word alignment")

    def get_alignment(self):
        """Get the current sub-word alignment, if any.

        This will return something if `ps_set_alignment` has been
        called, but it will not contain an actual *alignment*
        (i.e. phone and state durations) unless a second pass of
        decoding has been run.

        If the decoder is not in sub-word alignment mode then it will
        return None.

        Returns:
            Alignment - if an alignment exists.
        """
        cdef ps_alignment_t *al = ps_get_alignment(self._ps)
        if al == NULL:
            return None
        return Alignment.create_from_ptr(ps_alignment_retain(al))

    def n_frames(self):
        """Get the number of frames processed up to this point.

        Returns:
            int: Like it says.
        """
        return ps_get_n_frames(self._ps)

cdef class Vad:
    """Voice activity detection class.

    Args:
      mode(int): Aggressiveness of voice activity detction (0-3)
      sample_rate(int): Sampling rate of input, default is 16000.
                        Rates other than 8000, 16000, 32000, 48000
                        are only approximately supported, see note
                        in `frame_length`.  Outlandish sampling
                        rates like 3924 and 115200 will raise a
                        `ValueError`.
      frame_length(float): Desired input frame length in seconds,
                           default is 0.03.  The *actual* frame
                           length may be different if an
                           approximately supported sampling rate is
                           requested.  You must *always* use the
                           `frame_bytes` and `frame_length`
                           attributes to determine the input size.

    Raises:
      ValueError: Invalid input parameter (see above).
    """
    cdef ps_vad_t *_vad
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

    def __dealloc__(self):
        ps_vad_free(self._vad)

    @property
    def frame_bytes(self):
        """int: Number of bytes (not samples) required in an input frame.

        You *must* pass input of this size, as `bytes`, to the `Vad`.
        """
        return ps_vad_frame_size(self._vad) * 2

    @property
    def frame_length(self):
        """float: Length of a frame in seconds (*may be different from the one
        requested in the constructor*!)"""
        return ps_vad_frame_length(self._vad)

    @property
    def sample_rate(self):
        """int: Sampling rate of input data."""
        return ps_vad_sample_rate(self._vad)

    def is_speech(self, frame, sample_rate=None):
        """Classify a frame as speech or not.

        Args:
          frame(bytes): Buffer containing speech data (16-bit signed
                        integers).  Must be of length `frame_bytes`
                        (in bytes).
        Returns:
          boolean: Classification as speech or not speech.
        Raises:
          IndexError: `buf` is of invalid size.
          ValueError: Other internal VAD error.
        """
        cdef const unsigned char[:] cframe = frame
        cdef Py_ssize_t n_samples = len(cframe) // 2
        if len(cframe) != self.frame_bytes:
            raise IndexError("Frame size must be %d bytes" % self.frame_bytes)
        rv = ps_vad_classify(self._vad, <const short *>&cframe[0])
        if rv < 0:
            raise ValueError("VAD classification failed")
        return rv == PS_VAD_SPEECH

cdef class Endpointer:
    """Simple endpointer using voice activity detection.

    Args:
      window(float): Length in seconds of window for decision.
      ratio(float): Fraction of window that must be speech or
                    non-speech to make a transition.
      mode(int): Aggressiveness of voice activity detction (0-3)
      sample_rate(int): Sampling rate of input, default is 16000.
                        Rates other than 8000, 16000, 32000, 48000
                        are only approximately supported, see note
                        in `frame_length`.  Outlandish sampling
                        rates like 3924 and 115200 will raise a
                        `ValueError`.
      frame_length(float): Desired input frame length in seconds,
                           default is 0.03.  The *actual* frame
                           length may be different if an
                           approximately supported sampling rate is
                           requested.  You must *always* use the
                           `frame_bytes` and `frame_length`
                           attributes to determine the input size.

    Raises:
      ValueError: Invalid input parameter.  Also raised if the ratio
                  makes it impossible to do endpointing (i.e. it
                  is more than N-1 or less than 1 frame).
    """
    cdef ps_endpointer_t *_ep
    DEFAULT_WINDOW = PS_ENDPOINTER_DEFAULT_WINDOW
    DEFAULT_RATIO = PS_ENDPOINTER_DEFAULT_RATIO
    def __init__(
        self,
        window=0.3,
        ratio=0.9,
        vad_mode=Vad.LOOSE,
        sample_rate=Vad.DEFAULT_SAMPLE_RATE,
        frame_length=Vad.DEFAULT_FRAME_LENGTH,
    ):
        self._ep = ps_endpointer_init(window, ratio,
                                      vad_mode, sample_rate, frame_length)
        if (self._ep == NULL):
            raise ValueError("Invalid endpointer or VAD parameters")

    @property
    def frame_bytes(self):
        """int: Number of bytes (not samples) required in an input frame.

        You *must* pass input of this size, as `bytes`, to the `Endpointer`.
        """
        return ps_endpointer_frame_size(self._ep) * 2

    @property
    def frame_length(self):
        """float: Length of a frame in secondsq (*may be different from the one
        requested in the constructor*!)"""
        return ps_endpointer_frame_length(self._ep)

    @property
    def sample_rate(self):
        """int: Sampling rate of input data."""
        return ps_endpointer_sample_rate(self._ep)

    @property
    def in_speech(self):
        """bool: Is the endpointer currently in a speech segment?

        To detect transitions from non-speech to speech, check this
        before `process`.  If it was `False` but `process` returns
        data, then speech has started::

            prev_in_speech = ep.in_speech
            speech = ep.process(frame)
            if speech is not None:
                if prev_in_speech:
                    print("Speech started at", ep.speech_start)

        Likewise, to detect transitions from speech to non-speech,
        call this *after* `process`.  If `process` returned data but
        this returns `False`, then speech has stopped::

            speech = ep.process(frame)
            if speech is not None:
                if not ep.in_speech:
                    print("Speech ended at", ep.speech_end)
        """
        return ps_endpointer_in_speech(self._ep)

    @property
    def speech_start(self):
        """float: Start time of current speech region."""
        return ps_endpointer_speech_start(self._ep)

    @property
    def speech_end(self):
        """float: End time of current speech region."""
        return ps_endpointer_speech_end(self._ep)

    def process(self, frame):
        """Read a frame of data and return speech if detected.

        Args:
          frame(bytes): Buffer containing speech data (16-bit signed
                        integers).  Must be of length `frame_bytes`
                        (in bytes).
        Returns:
          bytes: Frame of speech data, or None if none detected.
        Raises:
          IndexError: `buf` is of invalid size.
          ValueError: Other internal VAD error.
        """
        cdef const unsigned char[:] cframe = frame
        cdef Py_ssize_t n_samples = len(cframe) // 2
        cdef const short *outframe
        if len(cframe) != self.frame_bytes:
            raise IndexError("Frame size must be %d bytes" % self.frame_bytes)
        outframe = ps_endpointer_process(self._ep,
                                         <const short *>&cframe[0])
        if outframe == NULL:
            return None
        return (<const unsigned char *>&outframe[0])[:n_samples * 2]

    def end_stream(self, frame):
        """Read a final frame of data and return speech if any.

        This function should only be called at the end of the input
        stream (and then, only if you are currently in a speech
        region).  It will return any remaining speech data detected by
        the endpointer.

        Args:
          frame(bytes): Buffer containing speech data (16-bit signed
                        integers).  Must be of length `frame_bytes`
                        (in bytes) *or less*.
        Returns:
          bytes: Remaining speech data (could be more than one frame),
          or None if none detected.
        Raises:
          IndexError: `buf` is of invalid size.
          ValueError: Other internal VAD error.

        """
        cdef const unsigned char[:] cframe = frame
        cdef Py_ssize_t n_samples = len(cframe) // 2
        cdef const short *outbuf
        cdef size_t out_n_samples
        if len(cframe) > self.frame_bytes:
            raise IndexError("Frame size must be %d bytes or less" % self.frame_bytes)
        outbuf = ps_endpointer_end_stream(self._ep,
                                          <const short *>&cframe[0],
                                          n_samples,
                                          &out_n_samples)
        if outbuf == NULL:
            return None
        return (<const unsigned char *>&outbuf[0])[:out_n_samples * 2]

cdef class AlignmentEntry:
    """Entry (word, phone, state) in an alignment.

    Iterating over this will iterate over its children (i.e. the
    phones in a word or the states in a phone) if any.  For example::

        for word in decoder.get_alignment():
            print("%s from %.2f to %.2f" % (word.name, word.start,
                                            word.start + word.duration))
            for phone in word:
                print("%s at %.2f duration %.2f" %
                      (phone.name, phone.start, phone.duration))

    Attributes:
      name(str): Name of segment (word, phone name, state id)
      start(int): Index of start frame.
      duration(int): Duration in frames.
      score(float): Acoustic score (density).
    """
    cdef public int start
    cdef public int duration
    cdef public int score
    cdef public str name
    # DANGER! Not retained!
    cdef ps_alignment_iter_t *itor
    @staticmethod
    cdef create_from_iter(ps_alignment_iter_t *itor):
        cdef AlignmentEntry self
        self = AlignmentEntry.__new__(AlignmentEntry)
        self.score = ps_alignment_iter_seg(itor, &self.start, &self.duration)
        self.name = ps_alignment_iter_name(itor).decode('utf-8')
        self.itor = itor  # DANGER! DANGER!
        return self

    def __iter__(self):
        cdef ps_alignment_iter_t *itor = ps_alignment_iter_children(self.itor)
        while itor != NULL:
            c = AlignmentEntry.create_from_iter(itor)
            yield c
            itor = ps_alignment_iter_next(itor)
        # FIXME: will leak memory if iteration stopped short!

cdef class Alignment:
    """Sub-word alignment as returned by `get_alignment`.

    For the moment this is read-only.  You are able to iterate over
    the words, phones, or states in it, as well as sub-iterating over
    each of their children, as described in `AlignmentEntry`.
    """
    cdef ps_alignment_t *_al

    @staticmethod
    cdef create_from_ptr(ps_alignment_t *al):
        cdef Alignment self = Alignment.__new__(Alignment)
        self._al = al
        return self

    def __dealloc__(self):
        if self._al != NULL:
            ps_alignment_free(self._al)

    def __iter__(self):
        return self.words()

    def words(self):
        """Iterate over words in the alignment."""
        cdef ps_alignment_iter_t *itor = ps_alignment_words(self._al)
        while itor != NULL:
            w = AlignmentEntry.create_from_iter(itor)
            yield w
            itor = ps_alignment_iter_next(itor)
        # FIXME: will leak memory if iteration stopped short!

    def phones(self):
        """Iterate over phones in the alignment."""
        cdef ps_alignment_iter_t *itor = ps_alignment_phones(self._al)
        while itor != NULL:
            p = AlignmentEntry.create_from_iter(itor)
            yield p
            itor = ps_alignment_iter_next(itor)

    def states(self):
        """Iterate over states in the alignment."""
        cdef ps_alignment_iter_t *itor = ps_alignment_states(self._al)
        while itor != NULL:
            s = AlignmentEntry.create_from_iter(itor)
            yield s
            itor = ps_alignment_iter_next(itor)

def set_loglevel(level):
    """Set internal log level of PocketSphinx.

    Args:
      level(str): one of "DEBUG", "INFO", "ERROR", "FATAL".
    Raises:
      ValueError: Invalid log level string.
    """
    cdef const char *prev_level
    prev_level = err_set_loglevel_str(level.encode('utf-8'))
    if prev_level == NULL:
        raise ValueError("Invalid log level %s" % level)

def _ps_default_modeldir():
    """Get the system default model path from the PocketSphinx library.

    Do not use this function directly, use
    pocketsphinx.get_model_path() instead.

    Returns:
      str: System default model path from PocketSphinx library.
    """
    dirbytes = ps_default_modeldir()
    if dirbytes == NULL:
        return None
    else:
        return dirbytes.decode()
