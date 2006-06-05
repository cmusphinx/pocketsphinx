/* ====================================================================
 * Copyright (c) 2005 Carnegie Mellon University.  All rights 
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
 * File: s2_mmap.c
 * 
 * Description: mmap() wrappers for Unix/Windows
 * 
 * Author: David Huggins-Daines <dhuggins@cs.cmu.edu>
 * 
 *********************************************************************/

#include "s2types.h"
#include "s2io.h"
#include "err.h"

#include <string.h>
#include <stdlib.h>

#ifdef WIN32
#ifdef GNUWINCE
#include <sys/wcebase.h>
#include <sys/wcetypes.h>
#include <sys/wcememory.h>
#include <sys/wcefile.h>
#else
#include <windows.h>
#endif /* GNUWINCE */
#else /* !WIN32 */
#include <unistd.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/mman.h>
#endif

#if defined(UNDER_CE) || defined(GNUWINCE)
void *s2_mmap(const char *filename)
{
	HANDLE fd;
	WCHAR *wfilename;
	void *rv;
	int len;

	len = mbstowcs(NULL, filename, 0) + 1;
	wfilename = malloc(len * sizeof(WCHAR));
	mbstowcs(wfilename, filename, len);

	if ((fd = CreateFileForMappingW(wfilename, GENERIC_READ, FILE_SHARE_READ,
					NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
					NULL)) == INVALID_HANDLE_VALUE)
		E_FATAL("Failed to CreateFileForMapping(%s): %08x\n",
			filename, GetLastError());
	if ((fd = CreateFileMappingW(fd, NULL, PAGE_READONLY, 0, 0, NULL)) == NULL)
		E_FATAL("Failed to CreateFileMapping: %08x\n", GetLastError());
	rv = MapViewOfFile(fd, FILE_MAP_READ, 0, 0, 0);
	free(wfilename);
	CloseHandle(fd);

	return rv;
}
#elif defined(WIN32)
void *s2_mmap(const char *filename)
{
	HANDLE fd;
	void *rv;

	if ((fd = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ,
			     NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
			     NULL)) == INVALID_HANDLE_VALUE)
		E_FATAL("Failed to CreateFile(%s): %08x\n",
			filename, GetLastError());
	if ((fd = CreateFileMapping(fd, NULL,
				    PAGE_READONLY, 0, 0, NULL)) == NULL)
		E_FATAL("Failed to CreateFileMapping: %08x\n", GetLastError());
	rv = MapViewOfFile(fd, FILE_MAP_READ, 0, 0, 0);
	CloseHandle(fd);

	return rv;
}
#else /* !WIN32 */
void *s2_mmap(const char *filename)
{
	struct stat buf;
	void *rv;
	int fd;

	if ((fd = open(filename, O_RDONLY)) == -1)
		E_FATAL_SYSTEM("Failed to open %s", filename);
	if (fstat(fd, &buf) == -1)
		E_FATAL_SYSTEM("Failed to stat %s", filename);
	rv = mmap(NULL, buf.st_size, PROT_READ, MAP_SHARED, fd, 0);
	close(fd);
	return rv;
}
#endif
