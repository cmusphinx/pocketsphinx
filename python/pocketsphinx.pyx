# Copyright (c) 2008 Carnegie Mellon University. All rights
# reserved.
#
# You may copy, modify, and distribute this code under the same terms
# as PocketSphinx or Python, at your convenience, as long as this
# notice is not removed.
#
# Author: David Huggins-Daines <dhuggins@cs.cmu.edu>

cdef class LatNode:
    """
    Node in a word lattice.

    @ivar word: Word this node corresponds to (with pronunciation variant).
    @type word: str
    @ivar baseword: Base word (no pronunciation variant) this node corresponds to.
    @type baseword: str
    @ivar sf: Start frame for this node.
    @type sf: int
    @ivar fef: First ending frame for this node.
    @type fef: int
    @ivar lef: Last ending frame for this node.
    @type lef: int
    @ivar best_exit: Best scoring exit link from this node
    @type best_exit: LatLink
    @ivar prob: Posterior probability for this node.
    @type prob: float
    """
    def __cinit__(self):
        self.node = NULL

    cdef set_node(LatNode self, ps_lattice_t *dag, ps_latnode_t *node):
        """
        Internal function - binds this to a PocketSphinx lattice node.
        """
        cdef short fef, lef
        cdef ps_latlink_t *best_exit
        self.dag = dag
        self.node = node
        self.word = ps_latnode_word(dag, node)
        self.baseword = ps_latnode_baseword(dag, node)
        self.sf = ps_latnode_times(node, &fef, &lef)
        self.fef = fef
        self.lef = lef
        self.best_exit = None
        best_exit = NULL
        self.prob = sb.logmath_log_to_ln(ps_lattice_get_logmath(dag),
                                         ps_latnode_prob(dag, node, &best_exit))
        if best_exit != NULL:
            self.best_exit = LatLink()
            self.best_exit.set_link(dag, best_exit)

    def exits(self):
        """
        Obtain an iterator over arcs exiting this node.

        @return: Iterator over arcs exiting this node
        @rtype: LatLinkIterator
        """
        cdef LatLinkIterator itor
        cdef ps_latlink_iter_t *citor

        citor = ps_latnode_exits(self.node)
        itor = LatLinkIterator()
        itor.itor = citor
        itor.dag = self.dag
        return itor

    def entries(self):
        """
        Obtain an iterator over arcs entering this node.

        @return: Iterator over arcs entering this node
        @rtype: LatLinkIterator
        """    
        cdef LatLinkIterator itor
        cdef ps_latlink_iter_t *citor

        citor = ps_latnode_entries(self.node)
        itor = LatLinkIterator()
        itor.itor = citor
        itor.dag = self.dag
        return itor

cdef class LatNodeIterator:
    """
    Iterator over word lattice nodes.
    """
    def __init__(self, start, end):
        self.itor = NULL
        self.first_node = True
        self.start = start
        self.end = end

    def __iter__(self):
        return self

    def __next__(self):
        """
        Advance iterator and return the next node.

        @return: Next lattice node in this iterator.
        @rtype: LatNode
        """
        cdef LatNode node
        cdef int start
        cdef ps_latnode_t *cnode

        # Make sure we keep raising exceptions at the end
        if self.itor == NULL:
            raise StopIteration
        # Advance the iterator if this isn't the first item
        if self.first_node:
            self.first_node = False
        else:
            self.itor = ps_latnode_iter_next(self.itor)
            if self.itor == NULL:
                raise StopIteration
        # Look for the next node within the given time range
        cnode = ps_latnode_iter_node(self.itor)
        start = ps_latnode_times(cnode, NULL, NULL)
        while start < self.start or start >= self.end:
            self.itor = ps_latnode_iter_next(self.itor)
            if self.itor == NULL:
                raise StopIteration
            cnode = ps_latnode_iter_node(self.itor)
            start = ps_latnode_times(cnode, NULL, NULL)
        node = LatNode()
        node.set_node(self.dag, cnode)
        return node

cdef class LatLink:
    """
    Link (edge) in a word lattice, connecting two nodes.

    @ivar word: Word (with pronunciation variant) for this link.
    @type word: str
    @ivar baseword: Base word (no pronunciation variant) for this link.
    @type baseword: str
    @ivar sf: Start frame for this link.
    @type sf: int
    @ivar fef: Ending frame for this link.
    @type fef: int
    @ivar prob: Posterior probability for this link.
    @type prob: float
    """
    def __cinit__(self):
        self.link = NULL

    cdef set_link(LatLink self, ps_lattice_t *dag, ps_latlink_t *link):
        """
        Internal function - binds this to a PocketSphinx lattice link.
        """
        cdef short sf
        self.dag = dag
        self.link = link
        self.word = ps_latlink_word(dag, link)
        self.baseword = ps_latlink_baseword(dag, link)
        self.ef = ps_latlink_times(link, &sf)
        self.sf = sf
        self.prob = sb.logmath_log_to_ln(ps_lattice_get_logmath(dag),
                                         ps_latlink_prob(dag, link, NULL))

    def nodes(self):
        """
        Get source and destination nodes for this link.

        @return: Source and destination nodes for this link
        @rtype: (LatNode, LatNode)
        """
        cdef LatNode src, dest
        cdef ps_latnode_t *csrc, *cdest

        cdest = ps_latlink_nodes(self.link, &csrc)
        src = LatNode()
        src.set_node(self.dag, csrc)
        dest = LatNode()
        dest.set_node(self.dag, cdest)
        return src, dest

    def pred(self):
        """
        Get backpointer from this link.

        @return: Backpointer from this link, set by bestpath search.
        @rtype: LatLink
        """
        cdef LatLink pred
        cdef ps_latlink_t *cpred

        cpred = ps_latlink_pred(self.link)
        if cpred == NULL:
            return None
        pred = LatLink()
        pred.set_link(self.dag, cpred)
        return pred

cdef class LatLinkIterator:
    """
    Iterator over word lattice links.
    """
    def __cinit__(self):
        self.itor = NULL
        self.first_link = True

    def __iter__(self):
        return self

    def __next__(self):
        """
        Advance iterator and return the next link.

        @return: Next lattice link in this iterator.
        @rtype: LatLink
        """
        cdef LatLink link
        if self.first_link:
            self.first_link = False
        else:
            self.itor = ps_latlink_iter_next(self.itor)
        if self.itor == NULL:
            raise StopIteration
        link = LatLink()
        link.set_link(self.dag, ps_latlink_iter_link(self.itor))
        return link

cdef class Lattice:
    """
    Word lattice.

    The word lattice is a compact representation of the set of
    hypotheses considered by the decoder when recognizing an
    utterance.

    A lattice object can be constructed either from a lattice file
    on disk or from a 'boxed' object passed in from GStreamer (or,
    in theory, anything else that uses GLib).  In the first case,
    the C{ps} argument is required.

    @param ps: PocketSphinx decoder.
    @type ps: Decoder
    @param latfile: Filename of lattice file to read.
    @type latfile: str
    @param boxed: Boxed pointer from GStreamer containing a lattice
    @type boxed: PyGBoxed

    @ivar n_frames: Number of frames of audio covered by this lattice
    @type n_frames: int
    @ivar start: Start node
    @type start: LatNode
    @ivar end: End node
    @type end: LatNode
    """
    def __init__(self, ps=None, latfile=None, boxed=None):
        self.dag = NULL
        if latfile:
            self.read_dag(ps, latfile)
        if boxed:
            self.set_boxed(boxed)

    cdef read_dag(Lattice self, Decoder ps, latfile):
        if ps:
            self.dag = ps_lattice_read(ps.ps, latfile)
        else:
            self.dag = ps_lattice_read(NULL, latfile)
        self.n_frames = ps_lattice_n_frames(self.dag)
        if self.dag == NULL:
            raise RuntimeError, "Failed to read lattice from %s" % latfile
        
    cdef set_dag(Lattice self, ps_lattice_t *dag):
        ps_lattice_retain(dag)
        ps_lattice_free(self.dag)
        self.dag = dag
        self.n_frames = ps_lattice_n_frames(dag)

    cdef set_boxed(Lattice self, box):
        cdef ps_lattice_t *dag
        dag = <ps_lattice_t *>(<PyGBoxed *>box).boxed
        ps_lattice_retain(dag)
        ps_lattice_free(self.dag)
        self.dag = dag
        self.n_frames = ps_lattice_n_frames(self.dag)

    def __dealloc__(self):
        ps_lattice_free(self.dag)

    def bestpath(self, NGramModel lmset, float lwf, float ascale):
        """
        Find the best path through the lattice, optionally using a
        language model.

        This function performs best-path search on the lattice, and
        returns the final link in the best path found.  The existing
        acoustic scores on the lattice links are used in conjunction
        with an optional language model.  A scaling factor can be
        applied to the acoustic scores to produce more useful
        posterior probabilities (in conjunction with C{posterior()},
        below).

        @param lmset: Language model (set) to use for rescoring
        @type lmset: sphinxbase.NGramModel
        @param lwf: Weight to apply to language model scores (on top
        of any existing language model weight set in C{lmset}).
        @type lwf: float
        @param ascale: Weight to apply to acoustic model scores.
        @type ascale: float
        @return: Final link in best path.
        @rtype: LatLink
        """
        cdef ps_latlink_t *end
        cdef LatLink link
        end = ps_lattice_bestpath(self.dag, lmset.lm, lwf, ascale)
        link = LatLink()
        link.set_link(self.dag, end)
        return link

    def posterior(self, NGramModel lmset, float ascale):
        """
        Calculate posterior probabilities of all links in a lattice.

        This function performs the backward part of forward-backward
        calculation of posterior probabilities for all links in the
        lattice.  It assumes that C{bestpath()} has already been
        called on the lattice.

        @param lmset: Language model (set) to use for rescoring
        @type lmset: sphinxbase.NGramModel
        @param ascale: Weight to apply to acoustic model scores.
        @type ascale: float
        @return: Log-probability of the lattice as a whole.
        @rtype: float
        """
        cdef logmath_t *lmath
        lmath = ps_lattice_get_logmath(self.dag)
        return sb.logmath_log_to_ln(lmath,
                                    ps_lattice_posterior(self.dag, lmset.lm, ascale))

    def nodes(self, start=0, end=-1):
        """
        Get an iterator over all nodes in the lattice.

        @param start: First frame to iterate over.
        @type start: int
        @param end: Last frame to iterate over, or -1 for all remaining
        @type end: int
        @return: Iterator over nodes.
        @rtype: LatNodeIterator
        """
        cdef LatNodeIterator itor

        if end == -1:
            end = ps_lattice_n_frames(self.dag)
        itor = LatNodeIterator(start, end)
        itor.dag = self.dag
        itor.itor = ps_latnode_iter(self.dag)
        return itor

    def write(self, outfile):
        """
        Write the lattice to an output file.

        @param outfile: Name of file to write to.
        @type outfile: str        
        """
        cdef int rv

        rv = ps_lattice_write(self.dag, outfile)
        if rv < 0:
            raise RuntimeError, "Failed to write lattice to %s" % outfile


cdef class Segment:

    def __init__(self):
        self.seg = NULL

    cdef set_seg(self, ps_seg_t *seg):
        self.seg = seg
    
    def word(self):
        return ps_seg_word(self.seg)
        
    def frames(self):
        cdef int sf, ef
        ps_seg_frames(self.seg, &sf, &ef) 
        return(sf, ef)

    def prob(self):
        cdef int32 ascr, lscr, lback
        ps_seg_prob(self.seg, &ascr, &lscr, &lback)
        return (ascr, lscr, lback)
    
cdef class SegmentIterator:
    """
    Iterator for best hypothesis word segments of best hypothesis
    """
    def __init__(self):
        self.itor = NULL
        self.first_seg = False

    cdef set_iter(self, ps_seg_t *seg):
        self.itor = seg
        self.first_seg = True

    def __iter__(self):
        return self
    
    def __next__(self):
        cdef Segment seg
        if self.first_seg:
            self.first_seg = False
        else:
            self.itor = ps_seg_next(self.itor)
        if NULL == self.itor:
            raise StopIteration
        else:
            seg = Segment()
            seg.set_seg(self.itor)
        return seg


cdef class Decoder:
    """
    PocketSphinx speech decoder.

    To initialize the PocketSphinx decoder, pass a list of keyword
    arguments to the constructor::

     d = pocketsphinx.Decoder(hmm='/path/to/acoustic/model',
                              lm='/path/to/language/model',
                              dict='/path/to/dictionary',
                              beam='1e-80')

    If no arguments are passed, the default acoustic and language
    models will be loaded, which may be acceptable for general English
    speech.  Any arguments supported by the PocketSphinx decoder are
    allowed here.  Only the most frequent ones are described below.

    @param boxed: Boxed pointer from GStreamer containing a decoder
    @type boxed: PyGBoxed
    @param hmm: Path to acoustic model directory
    @type hmm: str
    @param dict: Path to dictionary file
    @type dict: str
    @param lm: Path to language model file
    @type lm: str
    @param jsgf: Path to JSGF grammar file
    @type jsgf str
    """
    def __init__(self, **kwargs):
        cdef cmd_ln_t *config
        cdef int i

        # Construct from an existing GObject pointer if given
        if 'boxed' in kwargs:
            self.argc = 0
            self.set_boxed(kwargs['boxed'])
            return

        # A much more concise version of what pocketsphinx_parse_argdict used to do
        self.argc = len(kwargs) * 2
        self.argv = <char **>sb.ckd_calloc(self.argc, sizeof(char *))
        i = 0
        for k, v in kwargs.iteritems():
            if k[0] != '-':
                k = '-' + k
            self.argv[i] = sb.ckd_salloc(k)
            self.argv[i+1] = sb.ckd_salloc(v)
            i = i + 2
        config = sb.cmd_ln_parse_r(NULL, ps_args(), self.argc, self.argv, 0)
        if config == NULL:
            raise RuntimeError, "Failed to parse argument list"
        self.ps = ps_init(config)
        sb.cmd_ln_free_r(config)
        if self.ps == NULL:
            raise RuntimeError, "Failed to initialize PocketSphinx"

    cdef set_boxed(Decoder self, box):
        cdef ps_decoder_t *ps
        ps = <ps_decoder_t *>(<PyGBoxed *>box).boxed
        ps_retain(ps)
        ps_free(self.ps)
        self.ps = ps

    def __dealloc__(self):
        ps_free(self.ps)
        for i from 0 <= i < self.argc:
            sb.ckd_free(self.argv[i])
        sb.ckd_free(self.argv)
        self.argv = NULL
        self.argc = 0

    def decode_raw(self, fh, uttid=None, maxsamps=-1):
        """
        Decode raw audio from a file.

        @param fh: Filehandle to read audio from.
        @type fh: file
        @param uttid: Identifier to give to this utterance.
        @type uttid: str
        @param maxsamps: Maximum number of samples to read.  If not
        specified or -1, the rest of the file will be read.
        @type maxsamps: int
        """
        cdef FILE *cfh
        cdef int nsamp
        cdef char *cuttid

        cfh = PyFile_AsFile(fh)
        if uttid == None:
            cuttid = NULL
        else:
            cuttid = uttid
        return ps_decode_raw(self.ps, cfh, cuttid, maxsamps)

    def decode_senscr(self, fh, uttid=None):
        """
        Decode senone scores from a file.

        @param fh: Filehandle to read senone scores from.
        @type fh: file
        @param uttid: Identifier to give to this utterance.
        @type uttid: str
        """
        cdef FILE *cfh
        cdef char *cuttid

        cfh = PyFile_AsFile(fh)
        if uttid == None:
            cuttid = NULL
        else:
            cuttid = uttid
        return ps_decode_senscr(self.ps, cfh, cuttid)

    def start_utt(self, uttid=None):
        """
        Prepare the decoder to recognize an utterance.

        @param uttid: Identifier to give to this utterance.
        @type uttid: str
        """
        cdef char *cuttid

        if uttid == None:
            cuttid = NULL
        else:
            cuttid = uttid
        if ps_start_utt(self.ps, cuttid) < 0:
            raise RuntimeError, "Failed to start utterance processing"

    def process_raw(self, data, no_search=False, full_utt=False):
        """
        Process (decode) some audio data.

        @param data: Audio data to process.  This is packed binary
        data, which consists of single-channel, 16-bit PCM audio, at
        the sample rate specified when the decoder was initialized.
        @type data: str
        @param no_search: Buffer the data without actually processing it (default is to process the
        data as it is received).
        @type no_search: bool
        @param full_utt: This block of data is an entire utterance.
        Processing an entire utterance at once may improve
        recognition, particularly for the first utterance passed to
        the decoder.
        @type full_utt: bool
        """
        cdef Py_ssize_t len
        cdef char* strdata
        cdef raw_data_ptr cdata
        
        PyString_AsStringAndSize(data, &strdata, &len)
        cdata = strdata
        if ps_process_raw(self.ps, cdata, len / 2, no_search, full_utt) < 0:
            raise RuntimeError, "Failed to process %d samples of audio data" % len / 2

    def end_utt(self):
        """
        Finish processing an utterance.
        """
        if ps_end_utt(self.ps) < 0:
            raise RuntimeError, "Failed to stop utterance processing"

    def get_hyp(self):
        """
        Get a hypothesis string.

        This function returns the text which has been recognized so
        far, or, if C{end_utt()} has been called, the final
        recognition result.

        @return: Hypothesis string, utterance ID, recognition score
        @rtype: (str, str, int)
        """
        cdef const_char_ptr hyp
        cdef const_char_ptr uttid
        cdef int score

        hyp = ps_get_hyp(self.ps, &score, &uttid)

        # No result
        if hyp == NULL:
             return None, None, 0

        return hyp, uttid, score

    def get_prob(self):
        """
	Get a posterior probability.
	
	Returns the posterior in linear scale.
	
	@return: posterior probability of the result
	@rtype: float
	"""
        cdef logmath_t *lmath
        cdef const_char_ptr uttid
        lmath = ps_get_logmath(self.ps)
        return sb.logmath_exp(lmath, ps_get_prob(self.ps, &uttid))

    def get_lattice(self):
        """
        Get the word lattice.

        This function returns all hypotheses which have been
        considered so far, in the form of a word lattice.

        @return: Word lattice
        @rtype: Lattice
        """
        cdef ps_lattice_t *dag
        cdef Lattice lat

        dag = ps_get_lattice(self.ps)
        if dag == NULL:
            raise RuntimeError, "Failed to create word lattice"
        lat = Lattice()
        lat.set_dag(dag)
        return lat

    def get_lmset(self):
        """
        Get the language model set.

        This function returns the language model set, which allows you
        to obtain language model scores or switch language models.

        @return: Language model set
        @rtype: sphinxbase.NGramModel
        """
        cdef ngram_model_t *clm
        cdef logmath_t *lmath
        cdef cmd_ln_t *config
        cdef NGramModel lm

        lm = NGramModel()
        clm = sb.ngram_model_retain(ps_get_lmset(self.ps))
        lm.set_lm(clm)
        lmath = sb.logmath_retain(ps_get_logmath(self.ps))
        lm.set_lmath(lmath)
        config = ps_get_config(self.ps)

        # This is not necessarily true but it will have to do
        lm.lw = sb.cmd_ln_float32_r(config, "-lw")
        lm.wip = sb.cmd_ln_float32_r(config, "-wip")
        lm.uw = sb.cmd_ln_float32_r(config, "-uw")
        return lm

    def update_lmset(self, NGramModel lmset):
        """
        Notifies the decoder that the LMset has been modified.  Primarily used
        after adding/removing LMs or after switching to particular
        LM within the set

        @param lmset: the modified lmset
        @type lmset: sphinxbase.NGramModel

        @return: the lmset
        @rtype: sphinxbase.NGramModel
        """
        ps_update_lmset(self.ps, sb.ngram_model_retain(lmset.lm))
        return self

    def add_word(self, word, phones, update=True):
        """
        Add a word to the dictionary and current language model.

        @param word: Name of the word to add.
        @type word: str
        @param phones: Pronunciation of the word, a space-separated list of phones.
        @type phones: str
        @param update: Update the decoder to recognize this new word.
        If adding a number of words at once you may wish to pass
        C{False} here.
        @type update: bool
        """
        return ps_add_word(self.ps, word, phones, update)

    def load_dict(self, dictfile, fdictfile=None, format=None):
        """
        Load a new pronunciation dictionary.

        @param dictfile: Dictionary filename.
        @type dictfile: str
        @param fdictfile: Filler dictionary filename.
        @type fdictfile: str
        @param format: Dictionary format, currently unused.
        @type format: str
        """
        return ps_load_dict(self.ps, dictfile, fdictfile, format)

    def save_dict(self, dictfile, format=None):
        """
        Save current pronunciation dictionary to a file.

        @param dictfile: Dictionary filename.
        @type dictfile: str
        @param format: Dictionary format, currently unused.
        @type format: str
        """
        return ps_save_dict(self.ps, dictfile, format)

    def segments(self):
        cdef int32 score
        cdef ps_seg_t *first_seg
        cdef SegmentIterator itor
        first_seg = ps_seg_iter(self.ps, &score)
        if first_seg == NULL:
            raise RuntimeError, "Failed to create best path word segment iterator"
        itor = SegmentIterator()
        itor.set_iter(first_seg)
        return (itor, score)

