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
 * $Log: phone.h,v $
 * Revision 1.1.1.1  2006/05/23 18:45:03  dhuggins
 * re-importation
 *
 * Revision 1.7  2004/12/10 16:48:58  rkm
 * Added continuous density acoustic model handling
 *
 * 
 */


#ifndef _PHONE_H_
#define _PHONE_H_

#define NO_PHONE	-1

#define PT_CIPHONE	0		/* Context independent */
#define PT_CDPHONE	-1		/* Context dependent */
#define PT_WWPHONE	-2		/* With in Word */
#define PT_DIPHONE	-3		/* DiPhone */
#define PT_DIPHONE_S	-4		/* DiPhone Singleton */
#define PT_CDDPHONE	-99		/* Context dependent duration */
#define PT_WWCPHONE	1000		/* With in Word Component phone */

int phone_read(char *filename);
void phone_add_diphones(void);

/* TODO: 'extern inline' most of these if GNU C or C99 is in effect */
int32 phone_to_id(char const *phone_str, int verbose);
char const *phone_from_id(int32 phone_id);
int32 phone_id_to_base_id(int32 phone_id);
int32 phone_type(int32 phone_id);
int32 phone_len(int32 phone_id);
int32 phone_count(void);
int32 phoneCiCount (void);
int32 phoneWdCount (void);
int32 phone_map (int32 pid);
list_t *phoneList (void);

#endif /* _PHONE_H_ */
