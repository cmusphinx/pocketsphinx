#!/usr/bin/perl
## ====================================================================
##
## Copyright (c) 1996-2000 Carnegie Mellon University.  All rights 
## reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions
## are met:
##
## 1. Redistributions of source code must retain the above copyright
##    notice, this list of conditions and the following disclaimer. 
##
## 2. Redistributions in binary form must reproduce the above copyright
##    notice, this list of conditions and the following disclaimer in
##    the documentation and/or other materials provided with the
##    distribution.
##
## 3. The names "Sphinx" and "Carnegie Mellon" must not be used to
##    endorse or promote products derived from this software without
##    prior written permission. To obtain permission, contact 
##    sphinx@cs.cmu.edu.
##
## 4. Redistributions of any form whatsoever must retain the following
##    acknowledgment:
##    "This product includes software developed by Carnegie
##    Mellon University (http://www.speech.cs.cmu.edu/)."
##
## THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND 
## ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
## THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
## PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
## NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
## SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
## LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
## DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
## THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
## (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
## OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##
## ====================================================================
##
## Author: Evandro Gouvea
##

use File::Path;
use File::Copy;

my $index = 0;
if (lc($ARGV[0]) eq '-cfg') {
    $cfg_file = $ARGV[1];
    $index = 2;
} else {
    $cfg_file = "etc/sphinx_decode.cfg";
}

if (! -s "$cfg_file") {
    print ("unable to find default configuration file, use -cfg file.cfg or create etc/sphinx_decode.cfg for default\n");
    exit -3;
}

require $cfg_file;


#************************************************************************
# this script performs decoding.
# it needs as inputs a set of models in pocketsphinx format
# a mdef file and cepstra with transcription files.
#************************************************************************

$| = 1; # Turn on autoflushing

die "USAGE: $0 <part> <npart>" if (($#ARGV != ($index + 1)) and ($#ARGV != ($index - 1)));

if ($#ARGV == ($index + 1)) {
  $part = $ARGV[$index];
  $npart = $ARGV[$index + 1];
} else {
  $part = 1;
  $npart = 1;
}

$mdefname = $DEC_CFG_MDEF;
$modelname = $DEC_CFG_MODEL_NAME;
$processname = "decode";

$log_dir = "$DEC_CFG_LOG_DIR/$processname";
mkdir ($log_dir,0777) unless -d $log_dir;
$result_dir = "$DEC_CFG_BASE_DIR/result";
mkdir ($result_dir,0777) unless -d $result_dir;

$logfile = "$log_dir/${DEC_CFG_EXPTNAME}-${part}-${npart}.log";
$matchfile = "$result_dir/${DEC_CFG_EXPTNAME}-${part}-${npart}.match";

$statepdeffn = $DEC_CFG_HMM_TYPE; # indicates the type of HMMs
if ($statepdeffn ne ".semi.") {
  die "PocketSphinx requires hmm type '.semi'\n";
}

$hmm_dir = "$DEC_CFG_BASE_DIR/model_parameters/$modelname";

$nlines = 0;
open INPUT, "${DEC_CFG_LISTOFFILES}";
while (<INPUT>) {
    $nlines++;
}
close INPUT;

$ctloffset = int ( ( $nlines * ( $part - 1 ) ) / $npart );
$ctlcount = int ( ( $nlines * $part ) / $npart ) - $ctloffset;

copy "$DEC_CFG_GIF_DIR/green-ball.gif", "$DEC_CFG_BASE_DIR/.decode.$part.state.gif";
&DEC_HTML_Print ("\t" . &DEC_ImgSrc("$DEC_CFG_BASE_DIR/.decode.$part.state.gif") . " ");   
&DEC_Log ("    Decoding $ctlcount segments starting at $ctloffset (part $part of $npart) ");
&DEC_HTML_Print (&DEC_FormatURL("$logfile", "Log File") . "\n");

open LOG,">$logfile";

### now actually start  (this will clobber the previous logfile)
$DECODER = "$DEC_CFG_BIN_DIR/sphinx2_batch";

if (open PIPE, "\"$DECODER\" " .
    "-hmm \"$hmm_dir\" " .
    "-lw $DEC_CFG_LANGUAGEWEIGHT  " .
    "-feat $DEC_CFG_FEATURE " .
    "-beam $DEC_CFG_BEAMWIDTH " .
    "-nwbeam $DEC_CFG_WORDBEAM " .
    "-dict \"$DEC_CFG_DICTIONARY\" " .
    "-lm \"$DEC_CFG_LANGUAGEMODEL\" " .
    "-wip 0.2 " .
    "-ctl \"$DEC_CFG_LISTOFFILES\" " .
    "-ctloffset $ctloffset " .
    "-ctlcount $ctlcount " .
    "-cepdir \"$DEC_CFG_FEATFILES_DIR\" " .
    "-cepext $DEC_CFG_FEATFILE_EXTENSION " .
    "-hyp $matchfile " .
    "-agc $DEC_CFG_AGC " .
    "-cmn $DEC_CFG_CMN " .
    " 2>&1 |") {


    $processed_counter = 0;
    &DEC_Log ("\n        Using $ctl_counter files: ");
    $| = 1;				# Turn on autoflushing
    while (<PIPE>) {
	if (/(ERROR).*/) {
	    &DEC_LogError ($_ . "\n");
	}
	if (/(FATAL).*/) {
	    &DEC_LogError ($_ . "\n");
	    die "Received a fatal error";
	}
	print LOG "$_";
	# Keep track of progress being made.
	$processed_counter++  if (/^\s*SFrm\s+Efrm\s+.*Bestpath.*$/i);
	$percentage = int (($processed_counter / $ctlcount) * 100);
	if (!($percentage % 10)) {
	    &DEC_Log ("${percentage}% ") unless $printed;
	    $printed = 1;
	} else {
	    $printed = 0;
	}
    }
    close PIPE;
    $| = 0;
    $date = localtime;
    print LOG "$date\n";
    close LOG;
    &DEC_Log ("Finished\n");
    exit (0);
}

copy "$DEC_CFG_GIF_DIR/red-ball.gif", "$DEC_CFG_BASE_DIR/.decode.$part.state.gif";
&DEC_LogError ("\tFailed to start $DECODER \n");
exit (-1);
