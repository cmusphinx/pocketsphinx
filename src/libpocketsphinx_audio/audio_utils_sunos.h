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
 * audio_utils.h -- From Bob Brennan for Sun audio utilities.
 * 
 * HISTORY
 *
 * 07-Dec-01    K A Lenzo (lenzo@cs.cmu.edu) at Carnegie Mellon University
 *              ifdefed around the sun4/solaris header location diff.
 * 
 * 15-Jul-98	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		sys/audioio.h and sys/filio.h included following Alex 
 *              Rudnicky's remarks.
 * 
 * 02-Aug-95	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Created from Bob Brennan's original.
 */

#ifdef SUN4
#include <sun/audioio.h>
#else
#include <sys/audioio.h>
#endif
#include <sys/filio.h>

int	audioOpen(int rate);
void	audioPause(void);
void	audioFlush(void);
void	audioStartRecord(void);
void	audioStopRecord(void);
void	audioClose(void);
int	audioSetRecordGain(int gain);
