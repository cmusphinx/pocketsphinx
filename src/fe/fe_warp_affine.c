/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2006 Carnegie Mellon University.  All rights
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
 * File: fe_warp_affine.c
 *
 * Description:
 * 	Warp the frequency axis according to an affine function, i.e.:
 *
 *		w' = a * w + b
 *	
 *********************************************************************/

/* static char rcsid[] = "@(#)$Id: fe_warp_affine.c,v 1.2 2006/02/17 00:31:34 egouvea Exp $"; */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifdef _MSC_VER
#pragma warning (disable: 4996)
#endif

#include <pocketsphinx.h>

#include "util/strfuncs.h"
#include "fe/fe_warp.h"
#include "fe/fe_warp_affine.h"

#define N_PARAM		2
#define YES             1
#define NO              0

/*
 * params[0] : a
 * params[1] : b
 */
static float params[N_PARAM] = { 1.0f, 0.0f };
static int32 is_neutral = YES;
static char p_str[256] = "";
static float nyquist_frequency = 0.0f;


const char *
fe_warp_affine_doc()
{
    return "affine :== < w' = a * x + b >";
}

uint32
fe_warp_affine_id()
{
    return FE_WARP_ID_AFFINE;
}

uint32
fe_warp_affine_n_param()
{
    return N_PARAM;
}

void
fe_warp_affine_set_parameters(char const *param_str, float sampling_rate)
{
    char *tok;
    char *seps = " \t";
    char temp_param_str[256];
    int param_index = 0;

    nyquist_frequency = sampling_rate / 2;
    if (param_str == NULL) {
        is_neutral = YES;
        return;
    }
    /* The new parameters are the same as the current ones, so do nothing. */
    if (strcmp(param_str, p_str) == 0) {
        return;
    }
    is_neutral = NO;
    strncpy(temp_param_str, param_str, sizeof(temp_param_str) - 1);
    temp_param_str[sizeof(temp_param_str) - 1] = '\0';
    memset(params, 0, N_PARAM * sizeof(float));
    strncpy(p_str, param_str, sizeof(p_str) - 1);
    p_str[sizeof(p_str) - 1] = '\0';
    /* FIXME: strtok() is not re-entrant... */
    tok = strtok(temp_param_str, seps);
    while (tok != NULL) {
        params[param_index++] = (float) atof_c(tok);
        tok = strtok(NULL, seps);
        if (param_index >= N_PARAM) {
            break;
        }
    }
    if (tok != NULL) {
        E_INFO
            ("Affine warping takes up to two arguments, %s ignored.\n",
             tok);
    }

    /* Clamp parameters to reasonable ranges to prevent overflow */
    /* Reasonable range for scaling factor a: 0.1 to 10.0 */
    if (params[0] < 0.1f) {
        params[0] = 0.1f;
        E_WARN("Affine warp parameter 'a' clamped to minimum 0.1\n");
    } else if (params[0] > 10.0f) {
        params[0] = 10.0f;
        E_WARN("Affine warp parameter 'a' clamped to maximum 10.0\n");
    }

    /* Reasonable range for offset b: -nyquist to +nyquist */
    if (params[1] < -nyquist_frequency) {
        params[1] = -nyquist_frequency;
        E_WARN("Affine warp parameter 'b' clamped to minimum -%g\n", nyquist_frequency);
    } else if (params[1] > nyquist_frequency) {
        params[1] = nyquist_frequency;
        E_WARN("Affine warp parameter 'b' clamped to maximum %g\n", nyquist_frequency);
    }

    if (params[0] == 0) {
        is_neutral = YES;
        E_INFO
            ("Affine warping cannot have slope zero, warping not applied.\n");
    }
}

float
fe_warp_affine_warped_to_unwarped(float nonlinear)
{
    if (is_neutral) {
        return nonlinear;
    }
    else {
        /* linear = (nonlinear - b) / a */
        float temp = nonlinear - params[1];
        temp /= params[0];
        if (temp > nyquist_frequency) {
            E_WARN
                ("Warp factor %g results in frequency (%.1f) higher than Nyquist (%.1f)\n",
                 params[0], temp, nyquist_frequency);
        }
        return temp;
    }
}

float
fe_warp_affine_unwarped_to_warped(float linear)
{
    if (is_neutral) {
        return linear;
    }
    else {
        /* nonlinear = a * linear - b */
        float temp = linear * params[0];
        temp += params[1];
        return temp;
    }
}

void
fe_warp_affine_print(const char *label)
{
    uint32 i;

    for (i = 0; i < N_PARAM; i++) {
        printf("%s[%04u]: %6.3f ", label, i, params[i]);
    }
    printf("\n");
}
