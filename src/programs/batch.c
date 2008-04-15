/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
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

#include <stdio.h>

#include "pocketsphinx.h"

static const arg_t ps_args_def[] = {
    POCKETSPHINX_OPTIONS,

    /* Various options specific to batch-mode processing. */
    { "-ctl",
      ARG_STRING,
      NULL,
      "Control file listing utterances to be processed" },
    { "-ctloffset",
      ARG_INT32,
      "0",
      "No. of utterances at the beginning of -ctl file to be skipped" },
    { "-ctlcount",
      ARG_INT32,
      "-1",
      "No. of utterances to be processed (after skipping -ctloffset entries)" },
    { "-ctlincr",
      ARG_INT32,
      "1",
      "Do every Nth line in the control file" },
    { "-adcin",
      ARG_BOOLEAN,
      "no",
      "Input is raw audio data" },
    { "-adchdr",
      ARG_INT32,
      "0",
      "Size of audio file header in bytes (headers are ignored)" },
    { "-adcdev",
      ARG_STRING,
      NULL,
      "Device name for audio input (platform-specific)" },
    { "-cepdir",
      ARG_STRING,
      NULL,
      "Input files directory (prefixed to filespecs in control file)" },
    { "-cepext",
      ARG_STRING,
      ".mfc",
      "Input files extension (suffixed to filespecs in control file)" },
    { "-rawlogdir",
      ARG_STRING,
      NULL,
      "Directory for dumping raw audio" },	
    { "-mfclogdir",
      ARG_STRING,
      NULL,
      "Directory for dumping features" },	
    { "-logfn",									
      ARG_STRING,								
      NULL,									
      "Recognition log file name" },						
    { "-backtrace",									
      ARG_BOOLEAN,								
      "yes",									
      "Print back trace of recognition results" },				
    { "-hyp",									
      ARG_STRING,								
      NULL,									
      "Recognition output file name" },						
    { "-hypseg",									
      ARG_STRING,								
      NULL,									
      "Recognition output with segmentation file name" },			
    { "-matchscore",								
      ARG_BOOLEAN,								
      "no",									
      "Report score in hyp file" },						
    { "-reportpron",								
      ARG_BOOLEAN,								
      "no",									
      "Report alternate pronunciations in match file" },			
    { "-nbestdir",									
      ARG_STRING,								
      NULL,									
      "Directory for writing N-best hypothesis lists" },			
    { "-nbestext",									
      ARG_STRING,								
      "hyp",									
      "Extension for N-best hypothesis list files" },				
    { "-nbest",									
      ARG_INT32,								
      "0",									
      "Number of N-best hypotheses to write to -nbestdir" },			
    { "-outlatdir",									
      ARG_STRING,								
      NULL,									
      "Directory for dumping lattices" },
    CMDLN_EMPTY_OPTION
};

int
main(int32 argc, char *argv[])
{
    pocketsphinx_t *ps;
    cmd_ln_t *config;
    int rv = 0;

    config = cmd_ln_parse_r(NULL, pocketsphinx_args(), argc, argv, TRUE);
    if (config == NULL)
        return 1;
    ps = pocketsphinx_init(config);
    if (ps == NULL)
        return 1;

    pocketsphinx_free(ps);
    return rv;
}
