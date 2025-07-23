/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2024 Carnegie Mellon University.  All rights
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
/**
 * @file thread_local.h
 * @brief Cross-platform thread-local storage abstraction
 *
 * This header provides a portable PS_THREAD_LOCAL macro that maps to
 * the appropriate thread-local storage keyword for the current compiler.
 *
 * For modern systems only - requires:
 * - C11 (_Thread_local) or
 * - C++11 (thread_local) or
 * - GCC 3.3+ (__thread) or
 * - MSVC 2015+ (__declspec(thread))
 */

#ifndef _UTIL_THREAD_LOCAL_H_
#define _UTIL_THREAD_LOCAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Detect and define thread-local storage keyword */
#ifdef PS_USE_THREAD_LOCAL_RNG

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
    /* C11 with threads support */
    #define PS_THREAD_LOCAL _Thread_local
#elif defined(__cplusplus) && __cplusplus >= 201103L
    /* C++11 or later */
    #define PS_THREAD_LOCAL thread_local
#elif defined(__GNUC__) && (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3))
    /* GCC 3.3 or later */
    #define PS_THREAD_LOCAL __thread
#elif defined(_MSC_VER) && _MSC_VER >= 1900
    /* Visual Studio 2015 or later */
    #define PS_THREAD_LOCAL __declspec(thread)
#elif defined(_MSC_VER) && _MSC_VER >= 1300
    /* Older Visual Studio (2003+) - requires Vista+ at runtime */
    #define PS_THREAD_LOCAL __declspec(thread)
    #warning "Thread-local storage on this compiler requires Windows Vista or later"
#else
    #error "Thread-local storage not supported on this compiler. Please disable PS_USE_THREAD_LOCAL_RNG or upgrade your compiler."
#endif

#else /* !PS_USE_THREAD_LOCAL_RNG */

/* When TLS is disabled, define as empty for global state */
#define PS_THREAD_LOCAL /* empty */

#endif /* PS_USE_THREAD_LOCAL_RNG */

#ifdef __cplusplus
}
#endif

#endif /* _UTIL_THREAD_LOCAL_H_ */