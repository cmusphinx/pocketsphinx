/* ====================================================================
 * Copyright (c) 1998-2000 Carnegie Mellon University.  All rights 
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
/*********************************************************************
 *
 * File: cmd_ln_defn.h
 * 
 * Description: 
 *      Command line argument definition
 *
 * Author: 
 *      
 *********************************************************************/

#ifndef CMD_LN_DEFN_H
#define CMD_LN_DEFN_H

#include <sphinxbase/cmd_ln.h>
#include <sphinxbase/fe.h>

const char helpstr[] =
  "Description: \n\
Extract acoustic features form from audio file.\n\
\n\
The main parameters that affect the final output, with typical values, are:\n\
\n\
samprate, typically 8000, 11025, or 16000\n\
lowerf, 130, 200, 130, for the respective sampling rates above\n\
upperf, 3700, 5200, 6800, for the respective sampling rates above\n\
nfilt, 31, 37, 40, for the respective sampling rates above\n\
nfft, 256 or 512\n\
format, raw or nist or mswav\n\
\"";

const char examplestr[] =
  "Example: \n\
This example creates a cepstral file named \"output.mfc\" from an input audio file named \"input.raw\", which is a raw audio file (no header information), which was originally sampled at 16kHz. \n\
\n\
sphinx_fe -i  input.raw \n\
        -o   output.mfc \n\
        -input_endian little \n\
        -samprate  16000 \n\
        -lowerf    130 \n\
        -upperf    6800 \n\
        -nfilt     40 \n\
        -nfft      512";

static arg_t defn[] = {
  { "-help",
    ARG_BOOLEAN,
    "no",
    "Shows the usage of the tool"},
  
  { "-example",
    ARG_BOOLEAN,
    "no",
    "Shows example of how to use the tool"},

  waveform_to_cepstral_command_line_macro(),

  { "-argfile",
    ARG_STRING,
    NULL,
    "Argument file (e.g. feat.params from an acoustic model) to read parameters from.  This will override anything set in other command line arguments." },
  
  { "-i",
    ARG_STRING,
    NULL,
    "Single audio input file" },
  
  { "-o",
    ARG_STRING,
    NULL,
    "Single cepstral output file" },
  
  { "-c",
    ARG_STRING,
    NULL,
    "Control file for batch processing" },
  
  { "-nskip",
    ARG_INT32,
    "0",
    "If a control file was specified, the number of utterances to skip at the head of the file" },
  
  { "-runlen",
    ARG_INT32,
    "-1",
    "If a control file was specified, the number of utterances to process, or -1 for all" },

  { "-part",
    ARG_INT32,
    "0",
    "Index of the part to run (supersedes -nskip and -runlen if non-zero)" },
  
  { "-npart",
    ARG_INT32,
    "0",
    "Number of parts to run in (supersedes -nskip and -runlen if non-zero)" },
  
  { "-di",
    ARG_STRING,
    NULL,
    "Input directory, input file names are relative to this, if defined" },
  
  { "-ei",
    ARG_STRING,
    NULL,
    "Input extension to be applied to all input files" },
  
  { "-do",
    ARG_STRING,
    NULL,
    "Output directory, output files are relative to this" },
  
  { "-eo",
    ARG_STRING,
    NULL,
    "Output extension to be applied to all output files" },
  
  { "-build_outdirs",
    ARG_BOOLEAN,
    "yes",
    "Create missing subdirectories in output directory" },

  { "-sph2pipe",
    ARG_BOOLEAN,
    "no",
    "Input is NIST sphere (possibly with Shorten), use sph2pipe to convert" },

  { "-nist",
    ARG_BOOLEAN,
    "no",
    "Defines input format as NIST sphere" },
  
  { "-raw",
    ARG_BOOLEAN,
    "no",
    "Defines input format as raw binary data" },
  
  { "-mswav",
    ARG_BOOLEAN,
    "no",
    "Defines input format as Microsoft Wav (RIFF)" },
  
  { "-nchans",
    ARG_INT32,
    "1",
    "Number of channels of data (interlaced samples assumed)" },
  
  { "-whichchan",
    ARG_INT32,
    "0",
    "Channel to process (numbered from 1), or 0 to mix all channels" },
  
  { "-ofmt",
    ARG_STRING,
    "sphinx",
    "Format of output files - one of sphinx, htk, text." },
  
  { "-mach_endian",
    ARG_STRING,
#ifdef WORDS_BIGENDIAN
    "big",
#else
    "little",
#endif
    "Endianness of machine, big or little" },
  
  { "-blocksize",
    ARG_INT32,
    "2048",
    "Number of samples to read at a time." },

  { "-spec2cep",
    ARG_BOOLEAN,
    "no",
    "Input is log spectral files, output is cepstral files" },

  { "-cep2spec",
    ARG_BOOLEAN,
    "no",
    "Input is cepstral files, output is log spectral files" },

  { NULL, 0, NULL, NULL }
};

    
#define CMD_LN_DEFN_H

#endif /* CMD_LN_DEFN_H */ 
