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
 * posixsock.h
 *
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1996 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 
     Brian G. Milnes
     Speech Group
     Carnegie Mellon University 
     5-Mar-95

     An include file to hide posix socket differences.

 * $Log: posixsock.h,v $
 * Revision 1.1.1.1  2006/05/23 18:45:02  dhuggins
 * re-importation
 *
 * Revision 1.6  2004/12/10 16:48:56  rkm
 * Added continuous density acoustic model handling
 *
 * Revision 1.5  2004/07/16 00:57:10  egouvea
 * Added Ravi's implementation of FSG support.
 *
 * Revision 1.2  2004/05/27 14:22:57  rkm
 * FSG cross-word triphones completed (but for single-phone words)
 *
 * Revision 1.4  2001/12/07 17:30:00  lenzo
 * Clean up and remove extra lines.
 *
 * Revision 1.3  2001/12/07 05:14:20  lenzo
 * License 1.2.
 *
 * Revision 1.2  2000/12/05 01:45:11  lenzo
 * Restructuring, hear rationalization, warning removal, ANSIfy
 *
 * Revision 1.1.1.1  2000/01/28 22:09:07  lenzo
 * Initial import of sphinx2
 *
 *
 * 
 * 02-May-95	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University.
 * 		Added errno handling.
 * 
 */

#ifndef _POSIXSOCK_H_
#define _POSIXSOCK_H_

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

/*
 * Windows NT uses a different socket interface and return values from BSD.
 * For compatibility and portability, use the following even with BSD.
 */

#define SOCKET			int32		/* Returned by socket/accept */
#define INVALID_SOCKET		-1		/* Returned by socket/accept */
#define SOCKET_ERROR		-1		/* Returned by most socket operations */
#define SOCKET_ERRNO		errno
#define SOCKET_WOULDBLOCK	EWOULDBLOCK
#define SOCKET_IOCTL		ioctl
#define closesocket		close		/* To close a SOCKET */

#define BOOL int32

#endif
