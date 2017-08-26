/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2010 Carnegie Mellon University.  All rights
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

/* System headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "stdarg.h"
#include <errno.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#if defined(__ADSPBLACKFIN__)
#elif !defined(_WIN32_WCE)
#include <sys/types.h>
#endif

/* SphinxBase headers */
#include <sphinx_config.h>
#include <sphinxbase/cmd_ln.h>
#include <sphinxbase/fixpoint.h>
#include <sphinxbase/ckd_alloc.h>
#include <sphinxbase/bio.h>
#include <sphinxbase/err.h>
#include <sphinxbase/prim_type.h>

/* Local headers */
#include "tied_mgau_common.h"
#include "nn_mgau.h"

static ps_mgaufuncs_t nn_mgau_funcs = {
    "nn",
    nn_mgau_frame_eval,      /* frame_eval */
    nn_mgau_mllr_transform,  /* transform */
    nn_mgau_free             /* free */
};

int Send(int fd, const void *buf, size_t len)
{
    int n;
    if ((n = send(fd, buf, len, 0)) < 0)
    {
        printf("send failed\n");
        return -1;
    }
    return n;
}

int Recv(int fd, void *buf, size_t len)
{
    int n;
    if ((n = recv(fd, buf, len, 0)) < 0)
    {
        printf("send failed\n");
        return -1;
    }
    return n;
}

int open_server_socket(char *my_ip, char *server_ip, int port)
{
    int server_sock;
    struct sockaddr_in my_addr;
    struct sockaddr_in serveraddr;

    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        return -1;
    }

    bzero((char *)&my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    inet_aton(my_ip, &(my_addr.sin_addr));
    my_addr.sin_port = 0;
    if (bind(server_sock, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0)
    {
        printf("open_server_socket: binding failiure: %s\n", strerror(errno));
        return -1;
    }

    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET; 
    inet_pton(AF_INET, server_ip, &(serveraddr.sin_addr)); 
    serveraddr.sin_port = htons((unsigned short)port); 
    if (connect(server_sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
    {
        printf("open_server_socket: connect failed\n");
        return -1;
    }
    return server_sock;
}

void get_senone_scores(float *feat, int n_feats,
                        int16_t *scores, int n_scores,
                        int server_sock){
    int packet_len = (n_feats*sizeof(float));
    
    Send(server_sock,&packet_len,sizeof(int));
    Send(server_sock,feat,n_feats*sizeof(float));
    
    Recv(server_sock,scores,n_scores*2);
}

/**
 * Compute senone scores for the active senones.
 */
int32
nn_mgau_frame_eval(ps_mgau_t *ps,
                    int16 *senone_scores,
                    uint8 *senone_active,
                    int32 n_senone_active,
                    mfcc_t ** featbuf, int32 frame,
                    int32 compallsen)
{
    nn_mgau_t *s = (nn_mgau_t *)ps;
    // slightly hacky, tensorflow might take some time to initialize so the
    // socket might not be open when we get here
    while (s->keras_server_sock == -1){
        s->keras_server_sock = open_server_socket("127.0.0.1","0.0.0.0",9000);
    }
    
    get_senone_scores(featbuf[0],s->n_feats,senone_scores,138,s->keras_server_sock);
    
    return 0;
}

ps_mgau_t *
nn_mgau_init(acmod_t *acmod, bin_mdef_t *mdef)
{
    nn_mgau_t *s;
    ps_mgau_t *ps;

    // get parameters from commandline
    int port = cmd_ln_int32_r(acmod->config, "-nnport");
    char *model_name = cmd_ln_str_r(acmod->config, "-nnmgau");
    char *acwt = cmd_ln_str_r(acmod->config, "-nnacwt");

    if (access(model_name, R_OK) != 0){
        printf("Model file (%s) does not exist or is not readable\n", model_name);
        return NULL;
    }
    if (feat_dimension1(acmod->fcb) != 1){
        printf("Multi-stream feature types not supported yet\n");
        return NULL;
    }

    s = ckd_calloc(1, sizeof(*s));
    s->keras_server_sock = -1;

    // get dimensionality of the feature vectors
    s->n_feats = feat_dimension2(acmod->fcb, 0);
    printf("N_FEATS: %d\n", s->n_feats);
    char *base_comm = "python INSERT_PATH/src/libpocketsphinx/runNN.py";
    //char *base_comm = "python runNN.py";
    char *log_comm = "> INSERT_PATH/nn_logs/keras_server.log &";

    // calculating the number of digits in n_feats
    int len_n_feats = log10(s->n_feats) + 1e-9;
    // calculate the total lenght of command string assuming port number isn't more than 5 digits
    char *comm = calloc(strlen(base_comm) + 
                    1 + strlen(model_name) + 
                    1 + len_n_feats + 
                    1 + 5 + 
                    1 + strlen(acwt) +
                    1 + strlen(log_comm), sizeof(char));
    sprintf(comm, "%s %s %d %s %d %s", base_comm, model_name, s->n_feats, acwt, port, log_comm);
    printf("%s\n", comm);
    // will need to kill python separately after Pocketsphinx finishes execution
    system(comm);
    free(comm);
    
    ps = (ps_mgau_t *)s;
    ps->vt = &nn_mgau_funcs;
    return ps;
}

int
nn_mgau_mllr_transform(ps_mgau_t *ps,
                            ps_mllr_t *mllr)
{
    return 1;
}

void
nn_mgau_free(ps_mgau_t *ps)
{
    nn_mgau_t *s;
    s = (nn_mgau_t *)ps;
    free(s);
}
