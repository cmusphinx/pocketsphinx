import numpy
cimport numpy

def mixw_kmeans_iter(numpy.ndarray[numpy.float64_t, ndim=1] lmw not None,
                     numpy.ndarray[numpy.float64_t, ndim=1] cb not None):
    cdef numpy.ndarray[numpy.float64_t, ndim=1] cbacc = numpy.zeros(len(cb))
    cdef numpy.ndarray[numpy.float64_t, ndim=1] cbcnt = numpy.zeros(len(cb))
    cdef numpy.ndarray[numpy.float64_t, ndim=1] dist = numpy.zeros(len(cb))
    cdef numpy.float64_t tdist = 0.0, m, mdist
    cdef Py_ssize_t i, k, cw

    for i in range(lmw.shape[0]):
        m = lmw[i]
        mdist = 1e+50
        cw = 0
        for k in range(dist.shape[0]):
            dist[k] = cb[k] - m
            dist[k] *= dist[k]
            if dist[k] < mdist:
                mdist = dist[k]
                cw = k
        tdist += mdist
        cbacc[cw] += m
        cbcnt[cw] += 1
    cb[:] = cbacc / cbcnt
    return tdist
    
def map_mixw_cb(numpy.ndarray[numpy.float64_t, ndim=3] mixw not None,
                numpy.ndarray[numpy.float64_t, ndim=1] cb not None,
                numpy.float64_t zero=0.0):
    cdef numpy.ndarray[numpy.uint8_t, ndim=3] mwmap \
         = numpy.zeros((mixw.shape[0], mixw.shape[1], mixw.shape[2]), 'uint8')
    cdef numpy.ndarray[numpy.float64_t, ndim=3] lmw
    cdef Py_ssize_t s, f, g, i, cw
    cdef numpy.float64_t x, mdist, dist
    
    lmw = numpy.log(mixw)
    for s in range(lmw.shape[0]):
        for f in range(lmw.shape[1]):
            for g in range(lmw.shape[2]):
                x = mixw[s,f,g]
                if x <= zero:
                    mwmap[s,f,g] = len(cb)
                else:
                    cw = 0
                    mdist = 1e+50
                    for i in range(len(cb)):
                        dist = cb[i] - lmw[s,f,g]
                        dist *= dist
                        if dist < mdist:
                            mdist = dist
                            cw = i
                    mwmap[s,f,g] = cw
    return mwmap

def mixw_freq(numpy.ndarray[numpy.uint8_t, ndim=3] mwmap):
    cdef numpy.ndarray[numpy.int32_t, ndim=1] hist \
         = numpy.zeros(mwmap.max() + 1, 'int32')
    cdef Py_ssize_t i, j, k, cw
    for i in range(mwmap.shape[0]):
        for j in range(mwmap.shape[1]):
            for k in range(mwmap.shape[2]):
                cw = mwmap[i,j,k]
                hist[cw] += 1
    return hist

