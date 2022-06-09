/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2013 Carnegie Mellon University.  All rights
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


%extend LogMath {
    LogMath() {
	return logmath_init(1.0001f, 0, 0);
    }

    ~LogMath() {
        logmath_free($self);
    }
    
    double exp(int prob) {
        return logmath_exp($self, prob);
    }

    int log(double prob) {
        return logmath_log($self, prob);
    }

    int add(int logb_p, int logb_q) {
        return logmath_add($self, logb_p, logb_q);
    }

    int ln_to_log(double log_p) {
        return logmath_ln_to_log($self, log_p);
    }

    double log_to_ln(int logb_p) {
        return logmath_log_to_ln($self, logb_p);
    }

    int log10_to_log(double log_p) {
        return logmath_log10_to_log($self, log_p);
    }

    double log_to_log10(int logb_p) {
        return logmath_log_to_log10($self, logb_p);
    }

    float log10_to_log_float(double log_p) {
        return logmath_log10_to_log_float($self, log_p);
    }

    double log_float_to_log10(float log_p) {
        return logmath_log_float_to_log10($self, log_p);
    }
}
