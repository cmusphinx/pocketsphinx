#!/usr/bin/perl
# ====================================================================
# Copyright (c) 2000 Carnegie Mellon University.  All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer. 
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
#
# This work was supported in part by funding from the Defense Advanced 
# Research Projects Agency and the National Science Foundation of the 
# United States of America, and the CMU Sphinx Speech Consortium.
#
# THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND 
# ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
# NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# ====================================================================
#
#  Tutorial set up script
#
# ====================================================================

use strict;
use Cwd;
use File::Path;

my $sphinx_dir = getcwd();

my $task = "an4";
if ($#ARGV > -1) {
  $task = $ARGV[0];
}

print "Building task $task\n";

my ($langmodel, $langweight, $beam, $wbeam);

# Make sure that sclite is in your path if you want to use it. sclite
# is available from NIST (http://www.nist.gov/speech/tools/index.htm).

# At CMU, depending on the platform you're using, add
# /afs/cs.cmu.edu/user/robust/archive/nist/sctk/linux/bin or
# /afs/cs.cmu.edu/user/robust/archive/nist/sctk/sunos/bin to your
# path. The platform is `uname -s | tr '[A-Z]' '[a-z]'`.

my $align = "sclite";

if (lc($task) eq "an4") {
  $langmodel = " an4.ug.lm";
  $langweight = 10;
  $beam = 1e-80;
  $wbeam = 1e-48;
} elsif (lc($task) eq "rm1") {
  $langmodel = " rm1.bigram.arpabo";
  $langweight = 10;
  $beam = 1e-80;
  $wbeam = 1e-48;
} else {
  die "Task not previously defined. User has to provide language model and supporting variables\n";
}


my $task_dir = "../$task";
chdir "$task_dir";

system("perl \"$sphinx_dir/scripts/setup_sphinx.pl\" " .
       "-force -sphinxdir \"$sphinx_dir\" -task $task " .
       "-langmod $langmodel -langwt $langweight -bw $beam " .
       "-wbeam $wbeam -align \"$align\"");

mkdir "feat" if (! -e "feat");
open (CTL, "etc/${task}_test.fileids") 
  or die "Could not open control file etc/${task}_test.fileids\n";
while (<CTL>) {
  s/[\/\\][^\/\\]+$//g;
  mkpath ("feat/$_", 0, 0755) if (! -e "feat/$_");
}
close(CTL);

print "\n\nNow, please do:\n";
print "\tcd $task_dir\n";
print "And then, in Unix/Linux:\n";
print "\tperl scripts_pl/make_feats.pl -ctl etc/${task}_test.fileids (if needed)\n";
print "\tperl scripts_pl/decode/slave.pl\n";
print "Or in Windows:\n";
print "\tperl scripts_pl\\make_feats.pl -ctl etc\\${task}_test.fileids (if needed)\n";
print "\tperl scripts_pl\\decode\\slave.pl\n";
