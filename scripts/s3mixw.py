# Copyright (c) 2006 Carnegie Mellon University
#
# You may copy and modify this freely under the same terms as
# Sphinx-III

"""Read/write Sphinx-III Gaussian mixture weight files.

This module reads and writes the Gaussian mixture weight files used by
SphinxTrain, Sphinx-III, and PocketSphinx.
"""

__author__ = "David Huggins-Daines <dhuggins@cs.cmu.edu>"
__version__ = "$Revision: 8994 $"

from s3file import S3File, S3File_write

def open(filename, mode="rb"):
    if mode in ("r", "rb"):
        return S3MixwFile(filename, mode)
    elif mode in ("w", "wb"):
        return S3MixwFile_write(filename, mode)
    else:
        raise Exception, "mode must be 'r', 'rb', 'w', or 'wb'"

class S3MixwFile(S3File):
    "Read Sphinx-III format mixture weight files"
    def __init__(self, file, mode):
        S3File.__init__(self, file, mode)
        self._params = self._load()

    def readgauheader(self):
        if self.fileattr["version"] != "1.0":
            raise Exception("Version mismatch: must be 1.0 but is "
                            + self.fileattr["version"])

    def _load(self):
        self.readgauheader()
        self.fh.seek(self.data_start, 0)
        return self.read3d()

class S3MixwFile_write(S3File_write):
    "Write Sphinx-III format mixture weight files"

    def writeall(self, stuff):
        self.write3d(stuff)
