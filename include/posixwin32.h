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
 * posixwin32.h -- Containing PC win32 specific mappings to Unix names.
 * 
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1996 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 */


#ifndef __S2_POSIXWIN32_H__
#define __S2_POSIXWIN32_H__	1


#if ((defined(WIN32) || defined(_WIN32_WCE)) && !defined(__CYGWIN__) && !defined(GNUWINCE))
typedef char * caddr_t;
typedef unsigned long u_long;
typedef unsigned int u_int;
typedef unsigned short u_short;
typedef unsigned short u_int16;
typedef unsigned char u_char;
typedef float float32;
#define NDEBUG 1
#define M_PI 3.1415926535897932385E0
#define popen _popen
#define pclose _pclose
#ifdef FILENAME_MAX
#define MAXPATHLEN FILENAME_MAX
#else
#define MAXPATHLEN 512 /* An arbitrary value for WinCE */
#endif
#endif


#endif
