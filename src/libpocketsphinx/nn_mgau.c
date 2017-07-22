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
    // printf("%s\n", "sending request...");
    Send(server_sock,&packet_len,sizeof(int));
    Send(server_sock,feat,n_feats*sizeof(float));
    // printf("%s\n", "receiving scores...");
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
    while (s->keras_server_sock == -1){
        s->keras_server_sock = open_server_socket("127.0.0.1","0.0.0.0",9000);
    }
    // printf("%s\n", "KERAS_SERVER_SOCK opened");
    // printf("KERAS_SERVER_SOCK: %d\n", s->keras_server_sock);
    get_senone_scores(featbuf[0],s->n_feats,senone_scores,138,s->keras_server_sock);
    // printf("\r%d", frame);
    // fflush(stdout);
    return 0;
}

ps_mgau_t *
nn_mgau_init(acmod_t *acmod, bin_mdef_t *mdef)
{
    nn_mgau_t *s;
    ps_mgau_t *ps;
    s = ckd_calloc(1, sizeof(*s));
    system("python /home/mshah1/sphinx/pocketsphinx/src/libpocketsphinx/runNN.py > /home/mshah1/wsj/wsj0/keras_server_log &");
    s->keras_server_sock = -1;
    s->n_feats = feat_dimension2(acmod->fcb, 0);
    printf("N_FEATS: %d\n", s->n_feats);
    // s->keras_server_sock = open_server_socket("127.0.0.1","0.0.0.0",9000);
    // printf("KERAS_SERVER_SOCK: %d\n", s->keras_server_sock);
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
    
}
