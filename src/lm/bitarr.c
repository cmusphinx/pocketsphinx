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

/*
 * bitarr.c -- Bit array manipulations implementation.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <pocketsphinx/err.h>

#include "lm/bitarr.h"
#include "util/byteorder.h"

uint64 bitarr_read_int57(bitarr_address_t address, uint8 length, uint64 mask)
{
    uint64 value64;
    const uint8 *base_off = (const uint8 *)(address.base) + (address.offset >> 3);
    (void)length; /* Yeah, what is this for anyway? */
    memcpy(&value64, base_off, sizeof(value64));
    SWAP_LE_64(&value64);
    return (value64 >> (address.offset & 7)) & mask;
}

void bitarr_write_int57(bitarr_address_t address, uint8 length, uint64 value) 
{
    uint64 value64;
    uint8 *base_off = (uint8 *)(address.base) + (address.offset >> 3);
    (void)length; /* Yeah, what is this for anyway? */
    memcpy(&value64, base_off, sizeof(value64));
    SWAP_LE_64(&value64);
    value64 |= (value << (address.offset & 7));
    SWAP_LE_64(&value64);
    memcpy(base_off, &value64, sizeof(value64));
}

uint32 bitarr_read_int25(bitarr_address_t address, uint8 length, uint32 mask) 
{
    uint32 value32;
    const uint8 *base_off = (const uint8*)(address.base) + (address.offset >> 3);
    (void)length; /* Yeah, what is this for anyway? */
    memcpy(&value32, base_off, sizeof(value32));
    SWAP_LE_32(&value32);
    return (value32 >> (address.offset & 7)) & mask;
}

void bitarr_write_int25(bitarr_address_t address, uint8 length, uint32 value)
{
    uint32 value32;
    uint8 *base_off = (uint8 *)(address.base) + (address.offset >> 3);
    (void)length; /* Yeah, what is this for anyway? */
    memcpy(&value32, base_off, sizeof(value32));
    SWAP_LE_32(&value32);
    value32 |= (value << (address.offset & 7));
    SWAP_LE_32(&value32);
    memcpy(base_off, &value32, sizeof(value32));
}

void bitarr_mask_from_max(bitarr_mask_t *bit_mask, uint32 max_value)
{
    bit_mask->bits = bitarr_required_bits(max_value);
    bit_mask->mask = (uint32)((1ULL << bit_mask->bits) - 1);
}

uint8 bitarr_required_bits(uint32 max_value)
{
    uint8 res;

    if (!max_value) return 0;
    res = 1;
    while (max_value >>= 1) res++;
    return res;
}
