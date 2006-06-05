/* ====================================================================
 * Copyright (c) 1999-2001 Carnegie Mellon University.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * This work was supported in part by funding from the Defense Advanced 
 * Research Projects Agency and the National Science Foundation of the 
 * United States of America, and the CMU Sphinx Speech Consortium.
 *
 * THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND 
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
 * NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ====================================================================
 *
 */
/*
	30 May 1989 David R. Fulmer (drf) updated to do byte order
		conversions when necessary.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(WIN32) && !defined(GNUWINCE) && !defined(CYGWIN)
#include <io.h>
#else
#include <sys/file.h>
#include <unistd.h>
#endif
#include "byteorder.h"
#include "bio.h"
#include "err.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

int
areadint (char *file, int **data_ref, int *length_ref)
{
  int             fd;
  int             length;
  int             size, k;
  int             offset;
  char           *data;
  struct stat     st_buf;
  int             swap = 0;

  fd = open(file, O_RDONLY|O_BINARY, 0644);

  if (fd < 0)
  {
    fprintf (stderr, "areadint: %s: can't open: %s\n", file, strerror(errno));
    return -1;
  }
  if (read (fd, (char *) &length, 4) != 4)
  {
    fprintf (stderr, "areadint: %s: can't read length: %s\n", file, strerror(errno));
    close (fd);
    return -1;
  }
  if (fstat (fd, &st_buf) < 0)
  {
    fprintf (stderr, "areadint: %s: can't get stat: %s\n", file, strerror(errno));
    close(fd);
    return -1;
  }

  if (length * sizeof(int) + 4 != st_buf.st_size) {
    E_INFO("Byte reversing %s\n", file);
    swap = 1;
    SWAP_INT32(&length);
  }

  if (length * sizeof(int) + 4 != st_buf.st_size) {
    E_ERROR("Header count %d = %d bytes does not match file size %d\n",
	    length, length*sizeof(int) + 4, st_buf.st_size);
    return -1;
  }

  size = length * sizeof (int);
  if (!(data = malloc ((unsigned) size)))
  {
    fprintf (stderr, "areadint: %s: can't alloc data\n", file);
    close (fd);
    return -1;
  }
  if ((k = read (fd, data, size)) != size)
  {
    fprintf (stderr, "areadint: %s: Expected size (%d) different from size read(%d)\n",
	     file, size, k);
    close (fd);
    free (data);
    return -1;
  }
  close (fd);
  *data_ref = (int *) data;

  if (swap)
    for(offset = 0; offset < length; offset++)
      SWAP_INT32(*data_ref + offset);

  *length_ref = length;
  return length;
}
