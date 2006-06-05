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
 * HISTORY
 * 
 * 01-Jan-96	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Created by ANONYMOUS.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* default: more_separator = ' ' */
char *get_a_word (line, word, more_separator)
char *line, *word, more_separator;
{
  register int i;

  while (*line == more_separator || isspace(*line)) line++;
  if (*line == '\0') return NULL;
  i = 0;
  do { word[i++] = *line++;} while (!isspace(*line) && *line != more_separator&& *line != '\0');
  word[i] = '\0';
  return line;
}

#if 0 /* Doesn't appear to be used anywhere */
static void
find_sentid (char *file_head, char *sentid)
{
  register int i, j;
  int len, suffix;

  suffix = 0;
  len = strlen (file_head);
  if (file_head[len-1] == 'b' && file_head[len-2] == '-')
  {
    suffix = 1;
    len -= 2;
    file_head[len] = '\0';
  }
  i = len;
  while (file_head[--i] != '/');
  j = 0;
  while ( (sentid[j++] = file_head[++i]) != '\0');
  if (suffix)
  {
    file_head[len] = '-';	file_head[len+1] = 'b';
  }
}
#endif
