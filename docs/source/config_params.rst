Configuration parameters
========================

These are the parameters currently recognized by
`pocketsphinx.Config` and `pocketsphinx.Decoder` along with their
default values.

.. method:: Config(*args, **kwargs)

   Create a PocketSphinx configuration from keyword arguments
   described below.  For example::

        config = Config(hmm="path/to/things", dict="my.dict")

   The same keyword arguments can also be passed directly to the
   constructor for `pocketsphinx.Decoder`.

   Many parameters have default values.  Also, when constructing a
   `Config` directly (as opposed to parsing JSON), `hmm`, `lm`, and
   `dict` are set to the default models (some kind of US English
   models of unknown origin + CMUDict). You can prevent this by
   passing `None` for any of these parameters, e.g.::

       config = Config(lm=None)  # Do not load a language model

   Decoder initialization **will fail** if more than one of `lm`,
   `jsgf`, `fsg`, `keyphrase`, `kws`, `allphone`, or `lmctl` are set
   in the configuration.  To make life easier, and because there is no
   possible case in which you would do this intentionally, if you
   initialize a `Decoder` or `Config` with any of these (and not
   `lm`), the default `lm` value will be removed.  This is not the
   case if you decide to set one of them in an existing `Config`, so
   in that case you must make sure to set `lm` to `None`::

        config["jsgf"] = "spam_eggs_and_spam.gram"
        config["lm"] = None


   :keyword str hmm: Directory containing acoustic model files.
   :keyword bool logspec: Write out logspectral files instead of cepstra, defaults to ``False``
   :keyword bool smoothspec: Write out cepstral-smoothed logspectral files, defaults to ``False``
   :keyword str transform: Which type of transform to use to calculate cepstra (legacy, dct, or htk), defaults to ``legacy``
   :keyword float alpha: Preemphasis parameter, defaults to ``0.97``
   :keyword int samprate: Sampling rate, defaults to ``16000``
   :keyword int frate: Frame rate, defaults to ``100``
   :keyword float wlen: Hamming window length, defaults to ``0.025625``
   :keyword int nfft: Size of FFT, or 0 to set automatically (recommended), defaults to ``0``
   :keyword int nfilt: Number of filter banks, defaults to ``40``
   :keyword float lowerf: Lower edge of filters, defaults to ``133.33334``
   :keyword float upperf: Upper edge of filters, defaults to ``6855.4976``
   :keyword bool unit_area: Normalize mel filters to unit area, defaults to ``True``
   :keyword bool round_filters: Round mel filter frequencies to DFT points, defaults to ``True``
   :keyword int ncep: Number of cep coefficients, defaults to ``13``
   :keyword bool doublebw: Use double bandwidth filters (same center freq), defaults to ``False``
   :keyword int lifter: Length of sin-curve for liftering, or 0 for no liftering., defaults to ``0``
   :keyword str input_endian: Endianness of input data, big or little, ignored if NIST or MS Wav, defaults to ``little``
   :keyword str warp_type: Warping function type (or shape), defaults to ``inverse_linear``
   :keyword str warp_params: Parameters defining the warping function
   :keyword bool dither: Add 1/2-bit noise, defaults to ``False``
   :keyword int seed: Seed for random number generator; if less than zero, pick our own, defaults to ``-1``
   :keyword bool remove_dc: Remove DC offset from each frame, defaults to ``False``
   :keyword bool remove_noise: Remove noise using spectral subtraction, defaults to ``False``
   :keyword bool verbose: Show input filenames, defaults to ``False``
   :keyword str feat: Feature stream type, depends on the acoustic model, defaults to ``1s_c_d_dd``
   :keyword int ceplen: Number of components in the input feature vector, defaults to ``13``
   :keyword str cmn: Cepstral mean normalization scheme ('live', 'batch', or 'none'), defaults to ``live``
   :keyword str cmninit: Initial values (comma-separated) for cepstral mean when 'live' is used, defaults to ``40,3,-1``
   :keyword bool varnorm: Variance normalize each utterance (only if CMN == current), defaults to ``False``
   :keyword str agc: Automatic gain control for c0 ('max', 'emax', 'noise', or 'none'), defaults to ``none``
   :keyword float agcthresh: Initial threshold for automatic gain control, defaults to ``2.0``
   :keyword str lda: File containing transformation matrix to be applied to features (single-stream features only)
   :keyword int ldadim: Dimensionality of output of feature transformation (0 to use entire matrix), defaults to ``0``
   :keyword str svspec: Subvector specification (e.g., 24,0-11/25,12-23/26-38 or 0-12/13-25/26-38)
   :keyword str featparams: File containing feature extraction parameters.
   :keyword str mdef: Model definition input file
   :keyword str senmgau: Senone to codebook mapping input file (usually not needed)
   :keyword str tmat: HMM state transition matrix input file
   :keyword float tmatfloor: HMM state transition probability floor (applied to -tmat file), defaults to ``0.0001``
   :keyword str mean: Mixture gaussian means input file
   :keyword str var: Mixture gaussian variances input file
   :keyword float varfloor: Mixture gaussian variance floor (applied to data from -var file), defaults to ``0.0001``
   :keyword str mixw: Senone mixture weights input file (uncompressed)
   :keyword float mixwfloor: Senone mixture weights floor (applied to data from -mixw file), defaults to ``1e-07``
   :keyword int aw: Inverse weight applied to acoustic scores., defaults to ``1``
   :keyword str sendump: Senone dump (compressed mixture weights) input file
   :keyword str mllr: MLLR transformation to apply to means and variances
   :keyword bool mmap: Use memory-mapped I/O (if possible) for model files, defaults to ``True``
   :keyword int ds: Frame GMM computation downsampling ratio, defaults to ``1``
   :keyword int topn: Maximum number of top Gaussians to use in scoring., defaults to ``4``
   :keyword str topn_beam: Beam width used to determine top-N Gaussians (or a list, per-feature), defaults to ``0``
   :keyword float logbase: Base in which all log-likelihoods calculated, defaults to ``1.0001``
   :keyword float beam: Beam width applied to every frame in Viterbi search (smaller values mean wider beam), defaults to ``1e-48``
   :keyword float wbeam: Beam width applied to word exits, defaults to ``7e-29``
   :keyword float pbeam: Beam width applied to phone transitions, defaults to ``1e-48``
   :keyword float lpbeam: Beam width applied to last phone in words, defaults to ``1e-40``
   :keyword float lponlybeam: Beam width applied to last phone in single-phone words, defaults to ``7e-29``
   :keyword float fwdflatbeam: Beam width applied to every frame in second-pass flat search, defaults to ``1e-64``
   :keyword float fwdflatwbeam: Beam width applied to word exits in second-pass flat search, defaults to ``7e-29``
   :keyword int pl_window: Phoneme lookahead window size, in frames, defaults to ``5``
   :keyword float pl_beam: Beam width applied to phone loop search for lookahead, defaults to ``1e-10``
   :keyword float pl_pbeam: Beam width applied to phone loop transitions for lookahead, defaults to ``1e-10``
   :keyword float pl_pip: Phone insertion penalty for phone loop, defaults to ``1.0``
   :keyword float pl_weight: Weight for phoneme lookahead penalties, defaults to ``3.0``
   :keyword bool compallsen: Compute all senone scores in every frame (can be faster when there are many senones), defaults to ``False``
   :keyword bool fwdtree: Run forward lexicon-tree search (1st pass), defaults to ``True``
   :keyword bool fwdflat: Run forward flat-lexicon search over word lattice (2nd pass), defaults to ``True``
   :keyword bool bestpath: Run bestpath (Dijkstra) search over word lattice (3rd pass), defaults to ``True``
   :keyword bool backtrace: Print results and backtraces to log., defaults to ``False``
   :keyword int latsize: Initial backpointer table size, defaults to ``5000``
   :keyword int maxwpf: Maximum number of distinct word exits at each frame (or -1 for no pruning), defaults to ``-1``
   :keyword int maxhmmpf: Maximum number of active HMMs to maintain at each frame (or -1 for no pruning), defaults to ``30000``
   :keyword int min_endfr: Nodes ignored in lattice construction if they persist for fewer than N frames, defaults to ``0``
   :keyword int fwdflatefwid: Minimum number of end frames for a word to be searched in fwdflat search, defaults to ``4``
   :keyword int fwdflatsfwin: Window of frames in lattice to search for successor words in fwdflat search , defaults to ``25``
   :keyword str dict: Main pronunciation dictionary (lexicon) input file
   :keyword str fdict: Noise word pronunciation dictionary input file
   :keyword bool dictcase: Dictionary is case sensitive (NOTE: case insensitivity applies to ASCII characters only), defaults to ``False``
   :keyword str allphone: Perform phoneme decoding with phonetic lm (given here)
   :keyword bool allphone_ci: Perform phoneme decoding with phonetic lm and context-independent units only, defaults to ``True``
   :keyword str lm: Word trigram language model input file
   :keyword str lmctl: Specify a set of language model
   :keyword str lmname: Which language model in -lmctl to use by default
   :keyword float lw: Language model probability weight, defaults to ``6.5``
   :keyword float fwdflatlw: Language model probability weight for flat lexicon (2nd pass) decoding, defaults to ``8.5``
   :keyword float bestpathlw: Language model probability weight for bestpath search, defaults to ``9.5``
   :keyword float ascale: Inverse of acoustic model scale for confidence score calculation, defaults to ``20.0``
   :keyword float wip: Word insertion penalty, defaults to ``0.65``
   :keyword float nwpen: New word transition penalty, defaults to ``1.0``
   :keyword float pip: Phone insertion penalty, defaults to ``1.0``
   :keyword float uw: Unigram weight, defaults to ``1.0``
   :keyword float silprob: Silence word transition probability, defaults to ``0.005``
   :keyword float fillprob: Filler word transition probability, defaults to ``1e-08``
   :keyword str fsg: Sphinx format finite state grammar file
   :keyword str jsgf: JSGF grammar file
   :keyword str toprule: Start rule for JSGF (first public rule is default)
   :keyword bool fsgusealtpron: Add alternate pronunciations to FSG, defaults to ``True``
   :keyword bool fsgusefiller: Insert filler words at each state., defaults to ``True``
   :keyword str keyphrase: Keyphrase to spot
   :keyword str kws: A file with keyphrases to spot, one per line
   :keyword float kws_plp: Phone loop probability for keyphrase spotting, defaults to ``0.1``
   :keyword int kws_delay: Delay to wait for best detection score, defaults to ``10``
   :keyword float kws_threshold: Threshold for p(hyp)/p(alternatives) ratio, defaults to ``1e-30``
   :keyword str logfn: File to write log messages in
   :keyword str loglevel: Minimum level of log messages (DEBUG, INFO, WARN, ERROR), defaults to ``WARN``
   :keyword str mfclogdir: Directory to log feature files to
   :keyword str rawlogdir: Directory to log raw audio files to
   :keyword str senlogdir: Directory to log senone score files to
