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
/*---------------------------------------------------------------------------*
 * This routine reads the text file holding mean, variance and mode 
 * probablities of the mixture density parameters computed by EM. It returns 
 * prob/sqrt(mod var) instead of var to simplify computation in CDCN
 ----------------------------------------------------------------------------*/

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "ckd_alloc.h"
#include "cdcn.h"

int
cdcn_init(char const *filename, CDCN_type *cdcn_variables)
{
    int      i,j,ndim,ncodes;

    float    **codebuff;
    float    **varbuff;
    float    *probbuff;
    float    temp;

    FILE     *codefile;

    /*
     * Initialize run_cdcn flag to 1. If an error occurs this is reset to 0
     */
    cdcn_variables->run_cdcn = TRUE;

    codefile = fopen(filename,"r");
    if (codefile==NULL)
    {
        printf("Unable to open Codebook file\n");
        printf("Unable to run CDCN. Will not process cepstra\n");
        cdcn_variables->run_cdcn = FALSE;
        return(-1);
    }

    if (fscanf(codefile,"%d %d",&(cdcn_variables->num_codes),&(cdcn_variables->n_dim)) == 0)
    {
        printf("Error in format of cdcn statistics file\n");
        printf("Unable to run CDCN. Will not process cepstra\n");
        cdcn_variables->run_cdcn = FALSE;
        return(-1);
    }
    ndim = cdcn_variables->n_dim;
    if (ndim > NUM_COEFF) {
        printf("Error in data dimension in cdcn statistics file\n");
        printf("Unable to run CDCN. Will not process cepstra\n");
        cdcn_variables->run_cdcn = FALSE;
        return(-1);
    }
    if (ndim > N) {
        printf("Error in data dimension in cdcn statistics file\n");
        printf("The dimension %d found in the file can be at most %d.\n", ndim, N);
	printf("Will not process cepstra\n");
        cdcn_variables->run_cdcn = FALSE;
        return(-1);
    }
    ncodes = cdcn_variables->num_codes;
    codebuff = (float **)ckd_calloc_2d(ncodes, ndim, sizeof(float));
    if (codebuff == NULL)
    {
        printf("Unable to allocate space for codebook\n");
        printf("Unable to run CDCN. Will not process cepstra\n");
        cdcn_variables->run_cdcn = FALSE;
        return(-1);
    }
    varbuff = (float **)ckd_calloc_2d(ncodes, ndim, sizeof(float));
    if (varbuff == NULL)
    {
        printf("Unable to allocate space for variances\n");
        printf("Unable to run CDCN. Will not process cepstra\n");
        cdcn_variables->run_cdcn = FALSE;
        return(-1);
    }
    probbuff = (float *)ckd_malloc(ncodes*sizeof(float));
    if (probbuff == NULL)
    {
        printf("Unable to allocate space for mode probabilites\n");
        printf("Unable to run CDCN. Will not process cepstra\n");
        cdcn_variables->run_cdcn = FALSE;
        return(-1);
    }
    for(i=0;i < ncodes ;++i)
    {
        if (fscanf(codefile,"%f",&probbuff[i]) == 0)
        {
            printf("Error in format of cdcn statistics file\n");
            printf("Unable to run CDCN. Will not process cepstra\n");
            cdcn_variables->run_cdcn = FALSE;
            return(-1);
        }
        for (j=0;j<ndim;++j)
        {
            if (fscanf(codefile,"%f",&codebuff[i][j]) == 0)
            {
                printf("Error in format of cdcn statistics file\n");
                printf("Unable to run CDCN. Will not process cepstra\n");
                cdcn_variables->run_cdcn = FALSE;
                return(-1);
            }
        }
	temp = 1;
        for (j=0;j<ndim;++j)
        {
            if (fscanf(codefile,"%f",&varbuff[i][j]) == 0)
            {
                printf("Error in format of cdcn statistics file\n");
                printf("Unable to run CDCN. Will not process cepstra\n");
                cdcn_variables->run_cdcn = FALSE;
                return(-1);
            }
	    temp *= varbuff[i][j];
        }
	if ((temp = (float)sqrt(temp)) == 0)
        {
            printf("Error in format of cdcn statistics file\n");
            printf("Unable to run CDCN. Will not process cepstra\n");
            cdcn_variables->run_cdcn = FALSE;
            return(-1);
        }
	probbuff[i] /= temp;
    }

    fclose(codefile);
    cdcn_variables->means = codebuff;
    cdcn_variables->variance = varbuff;
    cdcn_variables->probs = probbuff;
    cdcn_variables->firstcall = TRUE;

    cdcn_variables->corrbook = 
      (float **) ckd_calloc_2d (ncodes, ndim, sizeof (float));
    if (cdcn_variables->corrbook == NULL)
    {
        printf("Unable to allocate space for correction terms\n");
        printf("Unable to run CDCN. Will not process cepstra\n");
        cdcn_variables->run_cdcn = FALSE;
        return(-1);
    }

    cdcn_variables->tilt = (float *)ckd_calloc(ndim, sizeof(float));
    if (cdcn_variables->tilt == NULL)
    {
        printf("Unable to allocate space for tilt vector\n");
        printf("Unable to run CDCN. Will not process cepstra\n");
        cdcn_variables->run_cdcn = FALSE;
        return(-1);
    }
    cdcn_variables->noise = (float *)ckd_calloc(ndim, sizeof(float));
    if (cdcn_variables->noise == NULL)
    {
        printf("Unable to allocate space for noise vector\n");
        printf("Unable to run CDCN. Will not process cepstra\n");
        cdcn_variables->run_cdcn = FALSE;
        return(-1);
    }
    return(0);
}
