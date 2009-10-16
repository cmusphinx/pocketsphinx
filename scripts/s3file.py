# Copyright (c) 2006 Carnegie Mellon University
#
# You may copy and modify this freely under the same terms as
# Sphinx-III

"""Read/write Sphinx-III binary parameter files.

All the various binary parameter files created by SphinxTrain and used
by Sphinx-III and PocketSphinx share a common file format.  This
module contains some base classes for reading and writing these files.
"""

__author__ = "David Huggins-Daines <dhuggins@cs.cmu.edu>"
__version__ = "$Revision: 8994 $"

from struct import unpack, pack
from numpy import array,reshape,shape,fromstring

class S3File(object):
    "Read Sphinx-III binary files"
    def __init__(self, filename=None, mode="rb"):
        if filename != None:
            self.open(filename, mode)

    def getall(self):
        return self._params

    def __getitem__(self, key):
        return self._params[key]

    def __setitem__(self, key, value):
        self._params[key] = value

    def __delitem__(self, key):
        del self._params[key]

    def __iter__(self):
        return iter(self._params)

    def __len__(self):
        return len(self._params)

    def open(self, filename, mode="rb"):
        self.filename = filename
        self.fh = file(filename, mode)
        self.readheader()

    def readheader(self):
        """
	Read binary header.  Sets the following attributes:

          - fileattr (a dictionary of attribute-value pairs)
          - swap (a byteswap string as used by the struct module)
          - otherend (a flag indicating if the file is wrong-endian
                  for the current platform)
          - data_start (offset of the start of data in the file)
	"""
	spam = self.fh.readline()
        self.fileattr = {}
        if spam != "s3\n":
            raise Exception("File ID not found or invalid: " + spam)
        while True:
            spam = self.fh.readline()
            if spam == "":
                raise Exception("EOF while reading headers")
            if spam.endswith("endhdr\n"):
                break
            sp = spam.find(' ')
            k = spam[0:sp].strip()
            v = spam[sp:].strip()
            self.fileattr[k] = v
        # This is 0x11223344 in the file's byte order
        spam = unpack("<i", self.fh.read(4))[0]
        if spam == 0x11223344:
            self.swap = "<" # little endian
        elif spam == 0x44332211:
            self.swap = ">" # big endian
        else:
            raise Exception("Invalid byte-order mark %08x" % spam)
        # Now determine whether we need to swap to get to native
        # byteorder (shouldn't this be easier???)
        self.otherend = (unpack('=i', pack(self.swap + 'i', spam))[0] != spam)
        self.data_start = self.fh.tell()

    def read3d(self):
        self.d1 = unpack(self.swap + "I", self.fh.read(4))[0]
        self.d2 = unpack(self.swap + "I", self.fh.read(4))[0]
        self.d3 = unpack(self.swap + "I", self.fh.read(4))[0]
        self._nfloats = unpack(self.swap + "I", self.fh.read(4))[0]
        if self._nfloats != self.d1 * self.d2 * self.d3:
            raise Exception(("Number of data points %d doesn't match "
                             + "total %d = %d*%d*%d")
                            %
                            (self._nfloats,
                             self.d1 * self.d2 * self.d3,
                             self.d1, self.d2, self.d3))
        spam = self.fh.read(self._nfloats * 4)
        params = fromstring(spam, 'f')
        if self.otherend:
            params = params.byteswap()
        return reshape(params, (self.d1, self.d2, self.d3)).astype('d')

    def read2d(self):
        self.d1 = unpack(self.swap + "I", self.fh.read(4))[0]
        self.d2 = unpack(self.swap + "I", self.fh.read(4))[0]
        self._nfloats = unpack(self.swap + "I", self.fh.read(4))[0]
        if self._nfloats != self.d1 * self.d2:
            raise Exception(("Number of data points %d doesn't match "
                             + "total %d = %d*%d")
                            %
                            (self._nfloats,
                             self.d1 * self.d2,
                             self.d1, self.d2))
        spam = self.fh.read(self._nfloats * 4)
        params = fromstring(spam, 'f')
        if self.otherend:
            params = params.byteswap()
        return reshape(params, (self.d1, self.d2)).astype('d')

    def read1d(self):
        self.d1 = unpack(self.swap + "I", self.fh.read(4))[0]
        self._nfloats = unpack(self.swap + "I", self.fh.read(4))[0]
        if self._nfloats != self.d1:
            raise Exception(("Number of data points %d doesn't match "
                             + "total %d")
                            %
                            (self._nfloats, self.d1))
        spam = self.fh.read(self._nfloats * 4)
        params = fromstring(spam, 'f')
        if self.otherend:
            params = params.byteswap()
        return params.astype('d')
        
class S3File_write:
    "Write Sphinx-III binary files"
    def __init__(self, filename=None, mode="wb", attr={"version":1.0}):
        self.fileattr = attr
        if filename != None:
            self.open(filename)

    def open(self, filename):
        self.filename = filename
        self.fh = file(filename, "wb")
        self.writeheader()

    def writeheader(self):
        self.fh.write("s3\n")
        for k,v in self.fileattr.iteritems():
            self.fh.write("%s %s\n" % (k,v))
        # Make sure the binary data lives on a 4-byte boundary
        lsb = (self.fh.tell() + len("endhdr\n")) & 3
        if lsb != 0:
            align = 4-lsb
            self.fh.write("%sendhdr\n" % (" " * align))
        else:
            self.fh.write("endhdr\n")
        self.fh.write(pack("=i", 0x11223344))
        self.data_start = self.fh.tell()

    def write3d(self, stuff):
        d1, d2, d3 = shape(stuff)
        self.fh.write(pack("=IIII",
                           d1, d2, d3,
                           d1 * d2 * d3))
        stuff.ravel().astype('f').tofile(self.fh)

    def write2d(self, stuff):
        d1, d2 = shape(stuff)
        self.fh.write(pack("=III",
                           d1, d2,
                           d1 * d2))
        stuff.ravel().astype('f').tofile(self.fh)

    def write1d(self, stuff):
        d1 = len(stuff)
        self.fh.write(pack("=II", d1, d1))
        stuff.ravel().astype('f').tofile(self.fh)

    def __del__(self):
        self.close()

    def close(self):
        self.fh.close()
