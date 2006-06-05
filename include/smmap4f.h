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
 * global constants and map of locations in sm for hmm sap rb - Nov 87 
 */


#ifndef __S2_SMMAP4F_H__
#define __S2_SMMAP4F_H__	1


#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define DEBUG	1

#define WRS	<
#define BTR	>
#define WRE	<=

#define BESTSCORE	(int32) 0
#define WORSTSCORE	min_log
#define NOSCORE	(int32) 0x80000000	/* largest negative, worse than worst */
#define LASTWORDSTATE	0x80000000	/* both defined in c code */
#define BOUNDARYERROR	0x80000000	
#define MEMSIZEERROR	-200

#define EMPTY	0		/* marks an empty entry in the beam table */
#define	ENDOFLIST	0	/* end of history tree */
#define HON_WORD_END	1	/* marks last phoneme of word in sib */

#define MAXNUMOFNETSTATES	17000
#define	NUMOFHMMS		7600
#define MAXSTATESPERHMM		6
#define NUMOFCODEENTRIES	256
#define NUMDISTRTYPES	5 
/* values to change if BOUNDARYERROR (0x80000000) */
#define NUMOFHISTORYWORDS	1500000	 /* 1600000 */
#define BEAMSIZE	160000  /* 26000 */
#define FINALBEAM	3000	/* 500 */
#define ENDBEAMSIZE	6000	/* 6000 */
/* end of boundary paramters */
#define NUMOFDISTRBUF	4	/* distribution buffers, minimum 2 */
#define MAX_SENT_LENGTH 	1500	/* 15 seconds */
#define MAX_WORDS_PER_SENT	200
#define NUMOFWORDS	1017
#define ENDOFBEAM	-1

/****************************************/
#define USERSMBASE			0

#define INFOSIZE		(sizeof(INFO))

#define INFOBASE		(USERSMBASE)

#define BSCORESIZE		(MAX_SENT_LENGTH * sizeof(BSCORE))

#define BSCOREBASE		(INFOBASE + INFOSIZE)

#define LASTWORDSIZE		(sizeof (LASTWORD) * NUMOFWORDS)

#define LASTWORDBASE		(BSCOREBASE + BSCORESIZE)

#define SCODESIZE		 (sizeof(INPUT_CODES)* MAX_SENT_LENGTH)

#define CODEBASE		(LASTWORDBASE + LASTWORDSIZE)

#define HISTORYSIZE		(sizeof(HISTORY) * NUMOFHISTORYWORDS)

#define HISTORYBASE		(CODEBASE + SCODESIZE)

#define TOTAL_OMATRIX	6240
#define PHONLIKSIZE 	(sizeof(int32) * TOTAL_OMATRIX * NUMOFDISTRBUF)

#define	PHONLIKBASE		(HISTORYBASE + HISTORYSIZE)
 
#define BEAM_ENTRYSIZE		7000000  /* 7000000 */

/*#define BEAM_ENTRYBASE		(HISTORYBASE + HISTORYSIZE) */
#define BEAM_ENTRYBASE		(PHONLIKBASE + PHONLIKSIZE)

#define REQBASE		(BEAM_ENTRYBASE + BEAM_ENTRYSIZE)

#define REQSIZE		(sizeof(REQINFO))

#define ACKBASE		(REQBASE + REQSIZE)

#define ACKSIZE		(sizeof (RETINFO))

#define BPRETBASE	(ACKBASE + ACKSIZE)

#define BPRETSIZE	(FINALBEAM * sizeof(BEAM_POINTERP))

#define SMSIZE	(BPRETBASE + BPRETSIZE - USERSMBASE)


#endif
