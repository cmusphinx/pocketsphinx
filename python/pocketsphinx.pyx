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
    Lattice node class.
    """
    def __cinit__(self):
        self.node = NULL

    cdef set_node(LatNode self, ps_lattice_t *dag, ps_latnode_t *node):
        cdef short fef, lef
        self.dag = dag
        self.node = node
        self.word = ps_latnode_word(dag, node)
        self.baseword = ps_latnode_baseword(dag, node)
        self.sf = ps_latnode_times(node, &fef, &lef)
        self.fef = fef
        self.lef = lef
        self.prob = sb.logmath_exp(ps_lattice_get_logmath(dag),
                                   ps_latnode_prob(dag, node, NULL))

    def exits(self):
        cdef LatLinkIterator itor
        cdef ps_latlink_iter_t *citor

        citor = ps_latnode_exits(self.node)
        itor = LatLinkIterator()
        itor.itor = citor
        itor.dag = self.dag
        return itor

    def entries(self):
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
    def __cinit__(self):
        self.itor = NULL
        self.first_node = True

    def __iter__(self):
        return self

    def __next__(self):
        cdef LatNode node
        if self.first_node:
            self.first_node = False
        else:
            self.itor = ps_latnode_iter_next(self.itor)
        if self.itor == NULL:
            raise StopIteration
        node = LatNode()
        node.set_node(self.dag, ps_latnode_iter_node(self.itor))
        return node

cdef class LatLink:
    """
    Lattice link class.
    """
    def __cinit__(self):
        self.link = NULL

    cdef set_link(LatLink self, ps_lattice_t *dag, ps_latlink_t *link):
        cdef short sf
        self.dag = dag
        self.link = link
        self.word = ps_latlink_word(dag, link)
        self.baseword = ps_latlink_baseword(dag, link)
        self.ef = ps_latlink_times(link, &sf)
        self.sf = sf
        self.prob = sb.logmath_exp(ps_lattice_get_logmath(dag),
                                   ps_latlink_prob(dag, link, NULL))

    def nodes(self):
        cdef LatNode src, dest
        cdef ps_latnode_t *csrc, *cdest

        cdest = ps_latlink_nodes(self.link, &csrc)
        src = LatNode()
        src.set_node(self.dag, csrc)
        dest = LatNode()
        dest.set_node(self.dag, cdest)
        return src, dest

cdef class LatLinkIterator:
    """
    Iterator over word lattice nodes.
    """
    def __cinit__(self):
        self.itor = NULL
        self.first_link = True

    def __iter__(self):
        return self

    def __next__(self):
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
    PocketSphinx word lattice class.
    """
    def __cinit__(self):
        self.dag = NULL

    cdef set_dag(Lattice self, ps_lattice_t *dag):
        ps_lattice_retain(dag)
        ps_lattice_free(self.dag)
        self.dag = dag

    def __dealloc__(self):
        ps_lattice_free(self.dag)

    def bestpath(self, NGramModel lmset, float lwf, float ascale):
        ps_lattice_bestpath(self.dag, lmset.lm, lwf, ascale)

    def posterior(self, NGramModel lmset, float ascale):
        cdef logmath_t *lmath
        lmath = ps_lattice_get_logmath(self.dag)
        return sb.logmath_exp(lmath,
                              ps_lattice_posterior(self.dag, lmset.lm, ascale))

    def nodes(self):
        cdef LatNodeIterator itor

        itor = LatNodeIterator()
        itor.dag = self.dag
        itor.itor = ps_latnode_iter(self.dag)
        return itor

cdef class Decoder:
    """
    PocketSphinx decoder class.

    To initialize the PocketSphinx decoder, pass a list of keyword
    arguments to the constructor:

    d = pocketsphinx.Decoder(hmm='/path/to/acoustic/model',
                             lm='/path/to/language/model',
                             dict='/path/to/dictionary',
                             beam='1e-80')
    """
    cdef ps_decoder_t *ps
    cdef char **argv
    cdef int argc

    def __cinit__(self, **kwargs):
        cdef cmd_ln_t *config
        cdef int i
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
        if self.ps == NULL:
            raise RuntimeError, "Failed to initialize PocketSphinx"

    def __dealloc__(self):
        ps_free(self.ps)
        for i from 0 <= i < self.argc:
            sb.ckd_free(self.argv[i])
        sb.ckd_free(self.argv)
        self.argv = NULL
        self.argc = 0

    def decode_raw(self, fh, uttid=None, maxsamps=-1):
        cdef FILE *cfh
        cdef int nsamp
        cdef char *cuttid

        cfh = PyFile_AsFile(fh)
        if uttid == None:
            cuttid = NULL
        else:
            cuttid = uttid
        return ps_decode_raw(self.ps, cfh, cuttid, maxsamps)

    def start_utt(self, uttid=None):
        cdef char *cuttid

        if uttid == None:
            cuttid = NULL
        else:
            cuttid = uttid
        if ps_start_utt(self.ps, cuttid) < 0:
            raise RuntimeError, "Failed to start utterance processing"

    def process_raw(self, data, no_search=False, full_utt=False):
        cdef Py_ssize_t len
        cdef char *cdata
        
        PyString_AsStringAndSize(data, &cdata, &len)
        if ps_process_raw(self.ps, cdata, len, no_search, full_utt) < 0:
            raise RuntimeError, "Failed to process %d samples of audio data" % len

    def ps_end_utt(self):
        if ps_end_utt(self.ps) < 0:
            raise RuntimeError, "Failed to stop utterance processing"

    def get_hyp(self):
        cdef char *hyp, *uttid
        cdef int score

        hyp = ps_get_hyp(self.ps, &score, &uttid)
        return hyp, uttid, score

    def get_lattice(self):
        cdef ps_lattice_t *dag
        cdef Lattice lat

        dag = ps_get_lattice(self.ps)
        if dag == NULL:
            raise RuntimeError, "Failed to create word lattice"
        lat = Lattice()
        lat.set_dag(dag)
        return lat

    def get_lmset(self):
        cdef ngram_model_t *clm
        cdef NGramModel lm

        clm = ps_get_lmset(self.ps)
        lm = NGramModel()
        lm.set_lm(clm)
        lm.set_lmath(ps_get_logmath(self.ps))
        return lm
