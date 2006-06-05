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
 * longword I/O routines
 * 
 * Created October 11, 1989 by Joe Keane.
 * 
 * These routines read and write longwords in a machine-independent format:
 * four bytes, most significant first.  On the Sun this is the same as the
 * internal format, so these routines can use fread and fwrite.  The routines
 * return negative result on error, except for read_long, where the caller
 * has to check for errors.
 */

#include <stdio.h>

long read_long (FILE *stream)
{
  int             c;
  long            word;

  c = getc (stream);
  if (c == EOF)
    return -1;
  word = c;
  c = getc (stream);
  if (c == EOF)
    return -1;
  word = word << 8 | c;
  c = getc (stream);
  if (c == EOF)
    return -1;
  word = word << 8 | c;
  c = getc (stream);
  if (c == EOF)
    return -1;
  return word << 8 | c;
}

int write_long (FILE *stream, long word)
{
  if (putc (word >> 24, stream) == EOF)
    return -1;
  if (putc (word >> 16, stream) == EOF)
    return -1;
  if (putc (word >> 8, stream) == EOF)
    return -1;
  if (putc (word, stream) == EOF)
    return -1;
  return 0;
}

int read_long_array (FILE *stream, long *base, int length)
{
#ifdef sun
  return fread ((char *) base, length * 4, 1, stream) != 1 ? -1 : 0;
#else
  int             counter;
  long           *ptr;

  counter = length;
  ptr = base;
  while (--counter >= 0)
  {
    int             c;
    long            word;

    c = getc (stream);
    if (c == EOF)
      return -1;
    word = c;
    c = getc (stream);
    if (c == EOF)
      return -1;
    word = word << 8 | c;
    c = getc (stream);
    if (c == EOF)
      return -1;
    word = word << 8 | c;
    c = getc (stream);
    if (c == EOF)
      return -1;
    *ptr++ = word << 8 | c;
  }
  return 0;
#endif
}

int write_long_array (FILE *stream, long *base, int length)
{
#ifdef sun
  return fwrite((char *) base, length * 4, 1, stream) != 1 ? -1 : 0;
#else
  int             counter;
  long           *ptr;

  counter = length;
  ptr = base;
  while (--counter >= 0)
  {
    long            word;

    word = *ptr++;
    if (putc (word >> 24, stream) == EOF)
      return -1;
    if (putc (word >> 16, stream) == EOF)
      return -1;
    if (putc (word >> 8, stream) == EOF)
      return -1;
    if (putc (word, stream) == EOF)
      return -1;
  }
  return 0;
#endif
}
