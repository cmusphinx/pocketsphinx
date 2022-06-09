/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2015 Carnegie Mellon University.  All rights
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

#ifndef _LIBUTIL_BITARR_H_
#define _LIBUTIL_BITARR_H_

#include <string.h>

#include <sphinxbase/prim_type.h>
/* Win32/WinCE DLL gunk */
#include <sphinxbase/sphinxbase_export.h>

/** 
 * @file bitarr.h
 * @brief An implementation bit array - memory efficient storage for digit int and float data.
 * 
 * Implementation of basic operations of read/write digits consuming as little space as possible.
 */

#ifdef __cplusplus
extern "C" {
#endif
#if 0
/* Fool Emacs. */
}
#endif

/**
 * Structure that specifies bits required to efficiently store certain data
 */
typedef struct bitarr_mask_s {
    uint8 bits;
    uint32 mask;
} bitarr_mask_t;

/**
 * Structure that stores address of certain value in bit array
 */
typedef struct bitarr_address_s {
    void *base;
    uint32 offset;
} bitarr_address_t;

/**
 * Read uint64 value from bit array. 
 * Assumes mask == (1 << length) - 1 where length <= 57
 * @param address to read from
 * @param length number of bits for value
 * @param mask of read value
 * @return uint64 value that was read
 */
SPHINXBASE_EXPORT
uint64 bitarr_read_int57(bitarr_address_t address, uint8 length, uint64 mask);

/**
 * Write specified value into bit array.
 * Assumes value < (1 << length) and length <= 57.
 * Assumes the memory is zero initially.
 * @param address to write to
 * @param length amount of active bytes in value to write
 * @param value integer to write
 */
SPHINXBASE_EXPORT
void bitarr_write_int57(bitarr_address_t address, uint8 length, uint64 value);

/**
 * Read uint32 value from bit array. 
 * Assumes mask == (1 << length) - 1 where length <= 25
 * @param address to read from
 * @param length number of bits for value
 * @param mask of read value
 * @return uint32 value that was read
 */
SPHINXBASE_EXPORT
uint32 bitarr_read_int25(bitarr_address_t address, uint8 length, uint32 mask);

/**
 * Write specified value into bit array.
 * Assumes value < (1 << length) and length <= 25.
 * Assumes the memory is zero initially.
 * @param address in bit array ti write to
 * @param length amount of active bytes in value to write
 * @param value integer to write
 */
SPHINXBASE_EXPORT
void bitarr_write_int25(bitarr_address_t address, uint8 length, uint32 value);

/**
 * Fills mask for certain int range according to provided max value
 * @param bit_mask mask that is filled
 * @param max_value bigest integer that is going to be stored using this mask
 */
SPHINXBASE_EXPORT
void bitarr_mask_from_max(bitarr_mask_t *bit_mask, uint32 max_value);

/**
 * Computes amount of bits required ti store integers upto value provided.
 * @param max_value bigest integer that going to be stored using this amount of bits
 * @return amount of bits required to store integers from range with maximum provided
 */
SPHINXBASE_EXPORT
uint8 bitarr_required_bits(uint32 max_value);

#ifdef __cplusplus
}
#endif

#endif /* _LIBUTIL_BITARR_H_ */
