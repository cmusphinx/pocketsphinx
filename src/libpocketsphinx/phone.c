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
 * $Log: phone.c,v $
 * Revision 1.1.1.1  2006/05/23 18:45:01  dhuggins
 * re-importation
 *
 * Revision 1.9  2004/12/10 16:48:56  rkm
 * Added continuous density acoustic model handling
 *
 * 
 * 11-Jun-89 Fil Alleva
 *	Fixed BUG that occured when $LPATH is not in the environment
 * 
 * 14-Oct-92 Eric Thayer (eht+@cmu.edu) Carnegie Mellon University
 *	added formal declaration of args to phone_to_id()
 * 
 * 14-Oct-92 Eric Thayer (eht+@cmu.edu) Carnegie Mellon University
 *	added type casts so that calls into hash.c use caddr_t so that
 *	code would work on an Alpha
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef WIN32
#include <fcntl.h>
#else
#include <sys/file.h>
#endif

#include "s2types.h"
#include "CM_macros.h"
#include "strfuncs.h"
#include "list.h"
#include "hash.h"
#include "phone.h"
#include "bin_mdef.h"
#include "c.h"
#include "err.h"
#include "kb.h"


static int32 parse_triphone(const char *instr, char *ciph, char *lc, char *rc, char *pc)
/*------------------------------------------------------------*
 * The ANSI standard scanf can't deal with empty field matches
 * so we have this routine.
 */
{
    const char *lp;
    char *cp;

    ciph[0] = '\0';
    lc[0] = '\0';
    rc[0] = '\0';
    pc[0] = '\0';

    /* parse ci-phone */
    for (lp = instr, cp = ciph; (*lp != '(') && (*lp != '\0'); lp++, cp++)
        *cp = *lp;
    *cp = '\0';
    if (*lp == '\0') {
	return 1;
    }

    /* parse leftcontext */
    for (lp++, cp = lc; (*lp != ',') && (*lp != '\0'); lp++, cp++)
        *cp = *lp;
    *cp = '\0';
    if (*lp == '\0') {
	return 2;
    }

    /* parse rightcontext */
    for (lp++, cp = rc; (*lp != ')') && (*lp != '\0'); lp++, cp++)
        *cp = *lp;
    *cp = '\0';
    if (*lp == '\0') {
	return 3;
    }

    /* parse positioncontext */
    for (lp++, cp = pc; (*lp != '\0'); lp++, cp++)
        *cp = *lp;
    *cp = '\0';
    return 4;    
}

int32 phone_map (int32 pid)
{
	return pid;
}

int32
phone_to_id(char const *phone_str, int verbose)
{
    char *ci, *lc, *rc, *pc;
    int32 cipid, lcpid, rcpid, pid;
    word_posn_t wpos;
    size_t len;
    bin_mdef_t *mdef = kb_mdef();

    /* Play it safe - subparts must be shorter than phone_str */
    len = strlen(phone_str+1);
    /* Do one malloc to avoid fragmentation on WinCE (and yet, this
     * may still be too many). */
    ci = CM_calloc(len*4+1,1);
    lc = ci + len;
    rc = lc + len;
    pc = rc + len;

    len = parse_triphone(phone_str, ci, lc, rc, pc);
    cipid = bin_mdef_ciphone_id(mdef, ci);
    if (cipid == BAD_S3PID) {
	free(ci);
	return NO_PHONE;
    }
    if (len > 1) {
	lcpid = bin_mdef_ciphone_id(mdef, lc);
	rcpid = bin_mdef_ciphone_id(mdef, rc);
	if (lcpid == BAD_S3PID || rcpid == BAD_S3PID) {
	    free(ci);
	    return NO_PHONE;
	}
	if (len == 4) {
	    switch (*pc) {
	    case 'b':
		wpos = WORD_POSN_BEGIN;
		break;
	    case 'e':
		wpos = WORD_POSN_END;
		break;
	    case 's':
		wpos = WORD_POSN_SINGLE;
		break;
	    default:
		wpos = WORD_POSN_INTERNAL;
	    }
	}
	else {
		wpos = WORD_POSN_INTERNAL;
	}
	pid = bin_mdef_phone_id(mdef, cipid, lcpid, rcpid, wpos);
    }
    else
	pid = cipid;

    free(ci);
    return pid;
}

char const *
phone_from_id(int32 phone_id)
{
    static char buf[1024];
    bin_mdef_t *mdef = kb_mdef();
    bin_mdef_phone_str(mdef, phone_id, buf);
    return buf;
}

int32
phone_id_to_base_id(int32 phone_id)
{
    bin_mdef_t *mdef = kb_mdef();
    return bin_mdef_pid2ci(mdef, phone_id);
}

int32
phone_type(int32 phone_id)
{
    bin_mdef_t *mdef = kb_mdef();
    if (bin_mdef_is_ciphone(mdef, phone_id))
	return PT_CIPHONE;
    else
	return PT_CDPHONE; /* No other kind of phone supported in S3. */
}

int32
phone_len(int32 phone_id)
{
    /* This is always 1 for S3 model defs. */
    return 1;
}

int32
phoneCiCount(void)
{
    bin_mdef_t *mdef = kb_mdef();
    return bin_mdef_n_ciphone(mdef);
}

int32
phone_count(void)
{
    bin_mdef_t *mdef = kb_mdef();
    return bin_mdef_n_phone(mdef);
}
