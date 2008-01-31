/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2004 Carnegie Mellon University.  All rights
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

/* System Headers */
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#if !defined(_WIN32_WCE)
#include <time.h>
#endif
#if !defined(_WIN32)
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/param.h>
#include <unistd.h> /* dup2() */
#endif

/* SphinxBase headers */
#include <sphinx_config.h>
#include <cmd_ln.h>
#include <byteorder.h>
#include <ckd_alloc.h>
#include <err.h>
#include <strfuncs.h>
#include <linklist.h>
#include <feat.h>
#include <fe.h>
#include <fixpoint.h>
#include <prim_type.h>

/* Local headers */
#include "s2_semi_mgau.h"
#include "senscr.h"
#include "search_const.h"
#include "dict.h"
#include "kb.h"
#include "fbs.h"
#include "search.h"
#include "fsg_search.h"
#include "uttproc.h"
#include "posixwin32.h"

#define MAX_UTT_LEN     6000    /* #frames */

typedef enum {
    UTTSTATE_UNDEF = -1,
    UTTSTATE_IDLE = 0,
    UTTSTATE_BEGUN = 1,
    UTTSTATE_ENDED = 2,
    UTTSTATE_STOPPED = 3
} uttstate_t;
static uttstate_t uttstate = UTTSTATE_UNDEF;
/* Used to flag beginning of utterance in livemode. */
static int32 uttstart;

static int32 inputtype;
#define INPUT_UNKNOWN   0
#define INPUT_RAW       1
#define INPUT_MFC       2

static int32 livemode;          /* Iff TRUE, search while input being supplied.  In this
                                   case, CMN, AGC and silence compression cannot be
                                   utterance based */
static int32 utt_ofl;           /* TRUE iff buffer limits overflowed in current utt */
static int32 nosearch = 0;
static int32 fsg_search_mode = FALSE;   /* Using FSM search structure */

/* Feature computation object */
feat_t *fcb;

/* MFC vectors for entire utt */
static mfcc_t **mfcbuf;
static int32 n_cepfr;          /* #input frames */

/* Feature vectors for entire utt */
static mfcc_t ***feat_buf;
static int32 n_featfr;          /* #features frames */
static int32 n_searchfr;

static FILE *matchfp = NULL;
static FILE *matchsegfp = NULL;

static char *rawlogdir = NULL;
static char *mfclogdir = NULL;
static FILE *rawfp = NULL;
static FILE *mfcfp = NULL;
static char rawfilename[4096];

static int32 samp_hist[5];      /* #Samples in 0-4K, 4K-8K, 8K-16K, 16K-24K, 24K-32K */
static int32 max_samp;

static char *uttid;
static char *uttid_prefix = NULL;
#define UTTIDSIZE       4096
static int32 uttno;             /* A running sequence number assigned to every utterance.  Used as
                                   an id for an utterance if uttid is undefined. */

static search_hyp_t *utt_seghyp = NULL;

static float TotalCPUTime, TotalElapsedTime, TotalSpeechTime;

#if defined(_WIN32) && !defined(GNUWINCE) && !defined(CYGWIN)
#include <windows.h>
static float e_start, e_stop;
static HANDLE pid;
static FILETIME t_create, t_exit, kst, ket, ust, uet;
static double lowscale, highscale;
extern double win32_cputime();
#else /* Not Windows */
static struct rusage start, stop;
static struct timeval e_start, e_stop;
#endif

/* searchlat.c */
void searchlat_set_rescore_lm(char const *lmname);

static fsg_search_t *fsg_search;


#if defined(_WIN32) && !defined(GNUWINCE) && !defined(CYGWIN)

/* The FILETIME manual page says: "It is not recommended that you add
 * and subtract values from the FILETIME structure to obtain relative
 * times."
 */
double
win32_cputime(FILETIME * st, FILETIME * et)
{
    double dt;
    ULARGE_INTEGER l_st = *(ULARGE_INTEGER *) st;
    ULARGE_INTEGER l_et = *(ULARGE_INTEGER *) et;
    LONGLONG ltime;

    ltime = l_et.QuadPart - l_st.QuadPart;

    dt = (ltime * lowscale);

    return (dt);
}

#else

double
MakeSeconds(struct timeval const *s, struct timeval const *e)
/*------------------------------------------------------------------------*
 * Compute an elapsed time from two timeval structs
 */
{
    return ((e->tv_sec - s->tv_sec) +
            ((e->tv_usec - s->tv_usec) / 1000000.0));
}

#endif

/*
 * One time initialization
 */
static void
timing_init(void)
{
#if defined(_WIN32) && !defined(GNUWINCE) && !defined(CYGWIN)
    lowscale = 1e-7;
    highscale = 65536.0 * 65536.0 * lowscale;

    pid = GetCurrentProcess();
#endif

    TotalCPUTime = TotalElapsedTime = TotalSpeechTime = 0.0;
}

/*
 * Start of each utterance
 */
static void
timing_start(void)
{
#if !(defined(_WIN32) && !defined(GNUWINCE) && !defined(CYGWIN))
# if !(defined(_HPUX_SOURCE) || defined(GNUWINCE))
    getrusage(RUSAGE_SELF, &start);
# endif
    gettimeofday(&e_start, 0);
#else                           /* _WIN32 */
# ifdef _WIN32_WCE
    e_start = (float) GetTickCount() / 1000;
# else
    e_start = (float) clock() / CLOCKS_PER_SEC;
    GetProcessTimes(pid, &t_create, &t_exit, &kst, &ust);
# endif /* !_WIN32_WCE */
#endif                          /* _WIN32 */
}

/*
 * End of each utterance
 */
static void
timing_stop(int32 nfr)
{
    if (nfr <= 0)
        return;

    E_INFO(" %5.2f SoS", searchFrame() * 0.01);
    TotalSpeechTime += searchFrame() * 0.01f;

#if defined(_WIN32) && !defined(GNUWINCE) && !defined(CYGWIN)
    /* ---------------- _WIN32 ---------------- */
# ifdef _WIN32_WCE
    e_stop = (float) GetTickCount() / 1000;
# else
    e_stop = (float) clock() / CLOCKS_PER_SEC;
    GetProcessTimes(pid, &t_create, &t_exit, &ket, &uet);
# endif

    E_INFOCONT(", %6.2f sec elapsed", (e_stop - e_start));
    E_INFOCONT(", %5.2f xRT", (e_stop - e_start) / (searchFrame() * 0.01));
# ifndef _WIN32_WCE
    E_INFOCONT(", %6.2f sec CPU", win32_cputime(&ust, &uet));
    E_INFOCONT(", %5.2f xRT",
               win32_cputime(&ust, &uet) / (searchFrame() * 0.01));
# endif

    TotalCPUTime += (float) win32_cputime(&ust, &uet);
    TotalElapsedTime += (e_stop - e_start);
#else
    /* ---------------- Unix ---------------- */
#if !(defined(_HPUX_SOURCE) || defined(GNUWINCE))
    getrusage(RUSAGE_SELF, &stop);
#endif
    gettimeofday(&e_stop, 0);

    E_INFOCONT(", %6.2f sec elapsed", MakeSeconds(&e_start, &e_stop));
    E_INFOCONT(", %5.2f xRT",
               MakeSeconds(&e_start, &e_stop) / (searchFrame() * 0.01));

#ifndef _HPUX_SOURCE
    E_INFOCONT(", %6.2f sec CPU",
               MakeSeconds(&start.ru_utime, &stop.ru_utime));
    E_INFOCONT(", %5.2f xRT",
               MakeSeconds(&start.ru_utime,
                           &stop.ru_utime) / (searchFrame() * 0.01));
#endif

    TotalCPUTime += MakeSeconds(&start.ru_utime, &stop.ru_utime);
    TotalElapsedTime += MakeSeconds(&e_start, &e_stop);
#endif

    E_INFOCONT("\n\n");
}

/*
 * One time cleanup before exiting program
 */
static void
timing_end(void)
{
    E_INFO("\n");

    E_INFO("TOTAL Elapsed time %.2f seconds\n", TotalElapsedTime);
#ifndef _HPUX_SOURCE
    E_INFO("TOTAL CPU time %.2f seconds\n", TotalCPUTime);
#endif
    E_INFO("TOTAL Speech %.2f seconds\n", TotalSpeechTime);

    if (TotalSpeechTime > 0.0) {
        E_INFO("AVERAGE %.2f xRT(Elapsed)",
               TotalElapsedTime / TotalSpeechTime);
#ifndef _HPUX_SOURCE
        E_INFOCONT(", %.2f xRT(CPU)", TotalCPUTime / TotalSpeechTime);
#endif
        E_INFOCONT("\n");
    }
}

static void
feat_alloc(void)
{
    if (feat_buf) {
        feat_array_free(feat_buf);
        ckd_free_2d((void **)mfcbuf);
    }
    feat_buf = feat_array_alloc(fcb, MAX_UTT_LEN);
    mfcbuf = (mfcc_t **) ckd_calloc_2d(MAX_UTT_LEN + 10,
                                       feat_cepsize(fcb), sizeof(mfcc_t));
}

static void
warn_notidle(char const *func)
{
    if (uttstate != UTTSTATE_IDLE)
        E_WARN("%s called when not in IDLE state\n", func);
}

int32
uttproc_get_featbuf(mfcc_t ****feat)
{
    *feat = feat_buf;
    return n_featfr;
}

static void
uttproc_fsg_search_fwd(void)
{
    int32 best;

    if (cmd_ln_boolean("-compallsen")) {
        best = senscr_all(feat_buf[n_searchfr], n_searchfr);
    }
    else {
        fsg_search_sen_active(fsg_search);
        best = senscr_active(feat_buf[n_searchfr], n_searchfr);
    }

    /* Note the best senone score for this frame */
    search_set_topsen_score(fsg_search_frame(fsg_search), best);

    fsg_search_frame_fwd(fsg_search);
}


/* Convert all given mfc vectors to feature vectors, and search one frame */
static int32
uttproc_frame(void)
{
    int32 pr, frm;
    char *str;
    search_hyp_t *hyp;

    /* Search one frame */
    if (fsg_search_mode)
        uttproc_fsg_search_fwd();
    else if (cmd_ln_boolean("-fwdtree"))
        search_fwd(feat_buf[n_searchfr]);
    else
        search_fwdflat_frame(feat_buf[n_searchfr]);
    ++n_searchfr;

    pr = cmd_ln_int32("-phypdump");
    if ((pr > 0) && ((n_searchfr % pr) == 0)) {
        /* Report partial result string */
        uttproc_partial_result(&frm, &str);
        printf("PART[%d]: %s\n", frm, str);
        fflush(stdout);
    }

    pr = cmd_ln_int32("-phypsegdump");
    if ((pr > 0) && ((n_searchfr % pr) == 0)) {
        /* Report partial result segmentation */
        uttproc_partial_result_seg(&frm, &hyp);
        printf("PARTSEG[%d]:", frm);
        for (; hyp; hyp = hyp->next)
            printf(" %s %d %d", hyp->word, hyp->sf, hyp->ef);
        printf("\n");
        fflush(stdout);
    }

    return 0;
}

static void
fwdflat_search(int32 n_frames)
{
    int32 i;

    search_fwdflat_start();

    for (i = 0; i < n_frames; ++i)
        search_fwdflat_frame(feat_buf[i]);

    search_fwdflat_finish();
}

static void
write_results(char const *hyp, int32 aborted)
{
    search_hyp_t *seghyp;       /* Hyp with word segmentation information */
    int32 i;

    /* Check if need to autonumber utterances */
    if (matchfp) {
        fprintf(matchfp, "%s (%s %s %d)\n",
                hyp, uttid, aborted ? "[ABORTED]" : "",
                search_get_score());
        fflush(matchfp);
    }

    /* Changed this to use Sphinx3 format */
    if (matchsegfp) {
        seghyp = search_get_hyp();
        fprintf(matchsegfp, "%s S %d T %d A %d L %d", uttid,
                0, /* FIXME: scaling factors not recorded? */
                search_get_score(),
                search_get_score() - search_get_lscr(),
                search_get_lscr());
        for (i = 0; seghyp[i].wid >= 0; i++) {
            fprintf(matchsegfp, " %d %d %d %s",
                    seghyp[i].sf,
                    seghyp[i].ascr,
                    ngram_score_to_prob(lmset, seghyp[i].lscr),
                    kb_get_word_str(seghyp[i].wid));
        }
        fprintf(matchsegfp, " %d\n", searchFrame());
        fflush(matchsegfp);
    }
}

static void
uttproc_windup(int32 * fr, char **hyp)
{
    /* Wind up first pass and run next pass, if necessary */
    if (fsg_search_mode)
        fsg_search_utt_end(fsg_search);
    else {
        if (cmd_ln_boolean("-fwdtree")) {
            search_finish_fwd();

            if (cmd_ln_boolean("-fwdflat") && (searchFrame() > 0))
                fwdflat_search(n_featfr);
        }
        else
            search_fwdflat_finish();

        /* Run bestpath pass if specified */
        /* FIXME: If we are doing N-best we also need to do this. */
        if ((searchFrame() > 0) && cmd_ln_boolean("-bestpath"))
            bestpath_search();
    }

    search_result(fr, hyp);

    write_results(*hyp, 0);

    timing_stop(*fr);

    uttstate = UTTSTATE_IDLE;
}

/*
 * One time initialization
 */

static fe_t *fe;

int32
uttproc_init(void)
{
    char const *fn;

    if (uttstate != UTTSTATE_UNDEF) {
        E_ERROR("uttproc_init called when not in UNDEF state\n");
        return -1;
    }

    /* Make sure linklist functions can operate. */
    linklist_init();

    fe = fe_init_auto();

    if (!fe)
        return -1;

    uttid = ckd_calloc(UTTIDSIZE, 1);

    if ((fn = cmd_ln_str("-hyp")) != NULL) {
        if ((matchfp = fopen(fn, "w")) == NULL)
            E_ERROR("fopen(%s,w) failed\n", fn);
    }
    if ((fn = cmd_ln_str("-hypseg")) != NULL) {
        if ((matchsegfp = fopen(fn, "w")) == NULL)
            E_ERROR("fopen(%s,w) failed\n", fn);
    }

    timing_init();

    uttstate = UTTSTATE_IDLE;
    utt_ofl = 0;
    uttno = 0;

    /* Initialize the FSG search module */
    {
        char *fsgfile;
        char *fsgname;
        char *fsgctlfile;
        FILE *ctlfp;
        char line[16384], word[16384];

        fsg_search = fsg_search_init(cmd_ln_get(), NULL);

        fsgfile = cmd_ln_str("-fsg");

        fsg_search_mode = (fsgfile != NULL);

        if (fsg_search_mode) {
            fsgname = uttproc_load_fsgfile(fsgfile);
            if (!fsgname)
                E_FATAL("Error loading FSG file '%s'\n", fsgfile);

            /* Make this FSG the currently active one */
            if (uttproc_set_fsg(fsgname) < 0)
                E_FATAL("Error setting current FSG to '%s'\n", fsgname);

            E_INFO
                ("FSG Mode; lextree, flat, bestpath searches disabled\n");
        }

        fsgctlfile = cmd_ln_str("-fsgctlfn");
        if (fsgctlfile) {
            if ((ctlfp = fopen(fsgctlfile, "r")) == NULL) {
                /* Should this be E_ERROR?? */
                E_FATAL("fopen(%s,r) failed\n", fsgctlfile);
            }

            while (fgets(line, sizeof(line), ctlfp) != NULL) {
                if ((line[0] == '#')    /* Commented out */
                    ||(sscanf(line, "%s", word) != 1))  /* Blank line */
                    continue;

                fsgfile = word;
                fsgname = uttproc_load_fsgfile(fsgfile);
                if (!fsgname) {
                    /* Should this be E_ERROR?? */
                    E_FATAL("Error loading FSG file '%s'\n", fsgfile);
                }
            }

            fclose(ctlfp);
        }
    }

    return 0;
}

/*
 * One time cleanup
 */
int32
uttproc_end(void)
{
    if (uttstate != UTTSTATE_IDLE) {
        E_ERROR("uttproc_end called when not in IDLE state\n");
        return -1;
    }

    if (matchfp)
        fclose(matchfp);
    if (matchsegfp)
        fclose(matchsegfp);

    timing_end();

    fe_close(fe);
    fe = NULL;

    ckd_free(uttid);
    uttid = NULL;

    if (feat_buf) {
        feat_array_free(feat_buf);
        ckd_free_2d((void **)mfcbuf);
        feat_buf = NULL;
        mfcbuf = NULL;
    }

    uttstate = UTTSTATE_UNDEF;
    return 0;
}

int32
uttproc_begin_utt(char const *id)
{
    char filename[1024];
    int32 i;

    for (i = 0; i < 5; i++)
        samp_hist[i] = 0;
    max_samp = 0;

    if (uttstate != UTTSTATE_IDLE) {
        E_ERROR("uttproc_begin_utt called when not in IDLE state\n");
        return -1;
    }

    if (fe_start_utt(fe) < 0)
        return -1;

    inputtype = INPUT_UNKNOWN;

    livemode = !(nosearch ||
                 (fcb->cmn == CMN_CURRENT) ||
                 ((fcb->agc != AGC_EMAX) && (fcb->agc != AGC_NONE)));
    E_INFO("%s\n", livemode ? "Livemode" : "Batchmode");

    n_cepfr = n_featfr = n_searchfr = 0;
    utt_ofl = 0;

    uttno++;
    if (!id)
        sprintf(uttid, "%s%08d", uttid_prefix ? uttid_prefix : "", uttno);
    else
        strcpy(uttid, id);

    if (rawlogdir) {
        sprintf(filename, "%s/%s.raw", rawlogdir, uttid);
        if ((rawfp = fopen(filename, "wb")) == NULL)
            E_ERROR("fopen(%s,wb) failed\n", filename);
        else {
            strcpy(rawfilename, filename);
            E_INFO("Rawfile: %s\n", filename);
        }
    }
    if (mfclogdir) {
        int32 k = 0;

        sprintf(filename, "%s/%s.mfc", mfclogdir, uttid);
        if ((mfcfp = fopen(filename, "wb")) == NULL)
            E_ERROR("fopen(%s,wb) failed\n", filename);
        else
            fwrite(&k, sizeof(int32), 1, mfcfp);
    }

    timing_start();

    if (!nosearch) {
        if (fsg_search_mode)
            fsg_search_utt_start(fsg_search);
        else if (cmd_ln_boolean("-fwdtree"))
            search_start_fwd();
        else
            search_fwdflat_start();
    }

    uttstate = UTTSTATE_BEGUN;
    uttstart = TRUE;

    return 0;
}

static int32
discard_start_frames(feat_t *fcb, mfcc_t ***feat_buf, int32 n_cepfr, int32 nfr)
{
    int32 i, j, discard; /* How many to throw away */

    discard = feat_window_size(fcb) - n_cepfr;
    if (discard > nfr)
        discard = nfr;
    /* Move some memory around. */
    for (i = 0; i < nfr - discard; ++i) {
        for (j = 0; j < feat_n_stream(fcb); ++j) {
            memcpy(feat_buf[i][j], feat_buf[i + discard][j],
                   feat_stream_len(fcb, j) * sizeof(mfcc_t));
        }
    }
    nfr -= discard;
    return nfr;
}

int32
uttproc_rawdata(int16 * raw, int32 len, int32 block)
{
    int32 i, k, v, nfr;
    mfcc_t **temp_mfc;

    for (i = 0; i < len; i++) {
        v = raw[i];
        if (v < 0)
            v = -v;
        if (v > max_samp)
            max_samp = v;

        if (v < 4096)
            samp_hist[0]++;
        else if (v < 8192)
            samp_hist[1]++;
        else if (v < 16384)
            samp_hist[2]++;
        else if (v < 30720)
            samp_hist[3]++;
        else
            samp_hist[4]++;
    }

    if (uttstate != UTTSTATE_BEGUN) {
        E_ERROR("uttproc_rawdata called when utterance not begun\n");
        return -1;
    }
    if (inputtype == INPUT_MFC) {
        E_ERROR
            ("uttproc_rawdata mixed with uttproc_cepdata in same utterance??\n");
        return -1;
    }
    inputtype = INPUT_RAW;

    if (utt_ofl)
        return -1;

    /* FRAME_SHIFT is SAMPLING_RATE/FRAME_RATE, thus resulting in
     * number of sample per frame.
     */
    k = (MAX_UTT_LEN - n_cepfr) * cmd_ln_float32("-samprate") / cmd_ln_int32("-frate");
    if (len > k) {
        len = k;
        utt_ofl = 1;
        E_ERROR("Utterance too long; truncating to about %d frames\n",
                MAX_UTT_LEN);
    }

    if (rawfp && (len > 0))
        fwrite(raw, sizeof(int16), len, rawfp);

    k = fe_process_utt(fe, raw, len, &temp_mfc, &nfr);
    if (k != FE_SUCCESS) {
        if (k == FE_ZERO_ENERGY_ERROR) {
            E_WARN("uttproc_rawdata processed some frames with zero energy. "
                   "Consider using -dither.\n");
        } else {
            return -1;
        }
    }

    if (nfr > 0)
        memcpy(mfcbuf[n_cepfr], temp_mfc[0], nfr * feat_cepsize(fcb) * sizeof(mfcc_t));

    if (mfcfp && (nfr > 0)) {
        fe_mfcc_to_float(fe, temp_mfc, (float32 **) temp_mfc, nfr);
        fwrite(temp_mfc[0], sizeof(float), nfr * feat_cepsize(fcb), mfcfp);
    }
    fe_free_2d(temp_mfc);

    if (livemode) {
        nfr = feat_s2mfc2feat_block(fcb, mfcbuf + n_cepfr, nfr,
                                    uttstart, FALSE,
                                    feat_buf + n_featfr);
        /* Be (bug?) compatible with Sphinx2, and discard the first
         * frames, since their dynamic coefficients are somewhat
         * bogus. */
        if (n_cepfr < feat_window_size(fcb))
            nfr = discard_start_frames(fcb, feat_buf, n_cepfr, nfr);
        uttstart = FALSE;
        n_cepfr += nfr;
        n_featfr += nfr;

        if (n_searchfr < n_featfr)
            uttproc_frame();

        if (block) {
            while (n_searchfr < n_featfr)
                uttproc_frame();
        }
    }
    else
        n_cepfr += nfr;

    return (n_featfr - n_searchfr);
}

int32
uttproc_cepdata(float32 ** cep, int32 nfr, int32 block)
{
    int32 i, k;

    if (uttstate != UTTSTATE_BEGUN) {
        E_ERROR("uttproc_cepdata called when utterance not begun\n");
        return -1;
    }
    if (inputtype == INPUT_RAW) {
        E_ERROR
            ("uttproc_cepdata mixed with uttproc_rawdata in same utterance??\n");
        return -1;
    }
    inputtype = INPUT_MFC;

    if (utt_ofl)
        return -1;

    k = MAX_UTT_LEN - n_cepfr;
    if (nfr > k) {
        nfr = k;
        utt_ofl = 1;
        E_ERROR("Utterance too long; truncating to about %d frames\n",
                MAX_UTT_LEN);
    }

    for (i = 0; i < nfr; i++) {
#ifdef FIXED_POINT
        int j;
        for (j = 0; j < feat_cepsize(fcb); ++j)
            mfcbuf[n_cepfr + i][j] = FLOAT2FIX(cep[i][j]);
#else
        memcpy(mfcbuf[i + n_cepfr], cep[i], feat_cepsize(fcb) * sizeof(float));
#endif
        if (mfcfp && (nfr > 0))
            fwrite(cep[i], sizeof(float32), feat_cepsize(fcb), mfcfp);
    }

    if (livemode) {
        nfr = feat_s2mfc2feat_block(fcb, mfcbuf + n_cepfr, nfr,
                                    uttstart, FALSE,
                                    feat_buf + n_featfr);
        /* Be (bug?) compatible with Sphinx2, and discard the first
         * frames, since their dynamic coefficients are somewhat
         * bogus. */
        if (n_cepfr < feat_window_size(fcb))
            nfr = discard_start_frames(fcb, feat_buf, n_cepfr, nfr);
        uttstart = FALSE;
        n_cepfr += nfr;
        n_featfr += nfr;

        if (n_searchfr < n_featfr)
            uttproc_frame();

        if (block) {
            while (n_searchfr < n_featfr)
                uttproc_frame();
        }
    }
    else
        n_cepfr += nfr;

    return (n_featfr - n_searchfr);
}

int32
uttproc_end_utt(void)
{
    int32 i, k, nfr;
    mfcc_t *leftover_cep;

    /* kal */
    leftover_cep = ckd_calloc(feat_cepsize(fcb), sizeof(mfcc_t));

    /* Dump samples histogram */
    k = 0;
    for (i = 0; i < 5; i++)
        k += samp_hist[i];
    if (k > 0) {
        E_INFO("Samples histogram (%s) (4/8/16/30/32K):",
               uttproc_get_uttid());
        for (i = 0; i < 5; i++)
            E_INFOCONT(" %.1f%%(%d)", samp_hist[i] * 100.0 / k,
                       samp_hist[i]);
        E_INFOCONT("; max: %d\n", max_samp);
    }

    if (uttstate != UTTSTATE_BEGUN) {
        E_ERROR("uttproc_end_utt called when utterance not begun\n");
        return -1;
    }

    uttstate = nosearch ? UTTSTATE_IDLE : UTTSTATE_ENDED;

    if (inputtype == INPUT_RAW) {
        fe_end_utt(fe, leftover_cep, &nfr);
        if (nfr && mfcfp) {
#ifdef FIXED_POINT
			float32 *leftover_cep_fl = ckd_calloc(feat_cepsize(fcb), sizeof(float32));
			fe_mfcc_to_float(fe, &leftover_cep, &leftover_cep_fl, nfr);
			fwrite(leftover_cep_fl, sizeof(float32), nfr * feat_cepsize(fcb), mfcfp);
			free(leftover_cep_fl);
#else
            fwrite(leftover_cep, sizeof(float32), nfr * feat_cepsize(fcb), mfcfp);
#endif
        }

        if (livemode) {
            nfr = feat_s2mfc2feat_block(fcb, &leftover_cep, nfr,
                                        uttstart, TRUE,
                                        feat_buf + n_featfr);
            /* Be (bug?) compatible with Sphinx2, and discard the last
             * frames since their dynamic coefficients are somewhat
             * bogus. */
            n_featfr += nfr - feat_window_size(fcb);
            if (n_featfr < 0)
                n_featfr = 0;
            uttstart = FALSE;
        }
        else {
            if (nfr) {
                memcpy(mfcbuf[i + n_cepfr], leftover_cep,
                       nfr * feat_cepsize(fcb) * sizeof(mfcc_t));
                n_cepfr += nfr;
            }
        }
    }

    /* Do feature computation if not in livemode. */
    if (!livemode) {
        /* If we had file input, n_cepfr will be zero. */
        if (n_cepfr) {
            nfr = feat_s2mfc2feat_block(fcb, mfcbuf, n_cepfr,
                                        TRUE, TRUE, feat_buf);
            /* Be (bug?) compatible with Sphinx2, and discard the
             * first and last frames */
            nfr -= feat_window_size(fcb) * 2;
            if (nfr < 0)
                nfr = 0;
            n_searchfr += feat_window_size(fcb);
            n_featfr += nfr;
        }
    }

    /* Do any further searching necessary. */
    if (!nosearch) {
        while (n_searchfr < n_featfr)
            uttproc_frame();
    }

    if (rawfp) {
        fclose(rawfp);
        rawfp = NULL;
    }
    if (mfcfp) {
        int32 k;

        fflush(mfcfp);
        fseek(mfcfp, 0, SEEK_SET);
        k = n_cepfr * feat_cepsize(fcb);
        fwrite(&k, sizeof(int32), 1, mfcfp);

        fclose(mfcfp);
        mfcfp = NULL;
    }

    free(leftover_cep);

    return 0;
}

int32
uttproc_abort_utt(void)
{
    int32 fr;
    char *hyp;

    if (uttproc_end_utt() < 0)
        return -1;

    /* Truncate utterance to the portion already processed */
    n_featfr = n_searchfr;

    uttstate = UTTSTATE_IDLE;

    if (!nosearch) {
        if (fsg_search_mode)
            fsg_search_utt_end(fsg_search);
        else {
            if (cmd_ln_boolean("-fwdtree"))
                search_finish_fwd();
            else
                search_fwdflat_finish();

            search_result(&fr, &hyp);

            write_results(hyp, 1);
        }
        timing_stop(fr);
    }

    return 0;
}

int32
uttproc_stop_utt(void)
{
    if (uttstate != UTTSTATE_BEGUN) {
        E_ERROR("uttproc_stop_utt called when utterance not begun\n");
        return -1;
    }

    uttstate = UTTSTATE_STOPPED;

    if (!nosearch) {
        if (fsg_search_mode)
            fsg_search_utt_end(fsg_search);
        else {
            if (cmd_ln_boolean("-fwdtree"))
                search_finish_fwd();
            else
                search_fwdflat_finish();
        }
    }

    return 0;
}

int32
uttproc_restart_utt(void)
{
    if (uttstate != UTTSTATE_STOPPED) {
        E_ERROR("uttproc_restart_utt called when decoding not stopped\n");
        return -1;
    }

    uttstate = UTTSTATE_BEGUN;

    if (!nosearch) {
        if (fsg_search_mode)
            fsg_search_utt_start(fsg_search);
        else if (cmd_ln_boolean("-fwdtree"))
            search_start_fwd();
        else
            search_fwdflat_start();

        n_searchfr = 0;
        n_searchfr = 0;
    }

    return 0;
}

int32
uttproc_partial_result(int32 * fr, char **hyp)
{
    if ((uttstate != UTTSTATE_BEGUN) && (uttstate != UTTSTATE_ENDED)) {
        E_ERROR("uttproc_partial_result called outside utterance\n");
        *fr = -1;
        *hyp = NULL;
        return -1;
    }

    if (fsg_search_mode) {
        fsg_search_history_backtrace(fsg_search, FALSE);
        search_result(fr, hyp);
    }
    else
        search_partial_result(fr, hyp);

    return 0;
}

int32
uttproc_result(int32 * fr, char **hyp, int32 block)
{
    if (uttstate != UTTSTATE_ENDED) {
        E_ERROR("uttproc_result called when utterance not ended\n");
        *hyp = NULL;
        *fr = -1;

        return -1;
    }

    if (n_searchfr < n_featfr)
        uttproc_frame();

    if (block) {
        while (n_searchfr < n_featfr)
            uttproc_frame();
    }

    if (n_searchfr < n_featfr)
        return (n_featfr - n_searchfr);

    uttproc_windup(fr, hyp);

    return 0;
}

void
utt_seghyp_free(search_hyp_t * h)
{
    search_hyp_t *tmp;

    while (h) {
        tmp = h->next;
        listelem_free(h, sizeof(search_hyp_t));
        h = tmp;
    }
}

static void
build_utt_seghyp(void)
{
    int32 i;
    search_hyp_t *seghyp, *last, *newhyp;

    /* Obtain word segmentation result */
    seghyp = search_get_hyp();

    /* Fill in missing details and build segmentation linked list */
    last = NULL;
    for (i = 0; seghyp[i].wid >= 0; i++) {
        newhyp = (search_hyp_t *) listelem_alloc(sizeof(search_hyp_t));
        newhyp->wid = seghyp[i].wid;
        newhyp->word = kb_get_word_str(newhyp->wid);
        newhyp->sf = seghyp[i].sf;
        newhyp->ef = seghyp[i].ef;
        newhyp->latden = seghyp[i].latden;
        newhyp->next = NULL;

        if (!last)
            utt_seghyp = newhyp;
        else
            last->next = newhyp;
        last = newhyp;
    }
}

int32
uttproc_partial_result_seg(int32 * fr, search_hyp_t ** hyp)
{
    char *str;

    /* Free any previous segmentation result */
    utt_seghyp_free(utt_seghyp);
    utt_seghyp = NULL;

    if ((uttstate != UTTSTATE_BEGUN) && (uttstate != UTTSTATE_ENDED)) {
        E_ERROR("uttproc_partial_result called outside utterance\n");
        *fr = -1;
        *hyp = NULL;
        return -1;
    }

    if (fsg_search_mode) {
        fsg_search_history_backtrace(fsg_search, FALSE);
        search_result(fr, &str);
    }
    else
        search_partial_result(fr, &str);        /* Internally makes partial result */

    build_utt_seghyp();
    *hyp = utt_seghyp;

    return 0;
}

int32
uttproc_result_seg(int32 * fr, search_hyp_t ** hyp, int32 block)
{
    char *str;
    int32 res;

    if (uttstate == UTTSTATE_ENDED) {
        if ((res = uttproc_result(fr, &str, block)) != 0)
            return res;             /* Not done yet; or ERROR */
    }
    else if (uttstate != UTTSTATE_IDLE) {
        E_ERROR("uttproc_result_seg() called when utterance not finished yet");
        return -1;
    }

    /* Free any previous segmentation result */
    utt_seghyp_free(utt_seghyp);
    utt_seghyp = NULL;

    build_utt_seghyp();
    *hyp = utt_seghyp;

    return 0;
}

int32
uttproc_lmupdate(char const *lmname)
{
    ngram_model_t *lm, *cur_lm;

    warn_notidle("uttproc_lmupdate");

    if ((lm = ngram_model_set_lookup(lmset, lmname)) == NULL)
        return -1;

    cur_lm = ngram_model_set_lookup(lmset, NULL);
    if (lm == cur_lm)
        search_set_current_lm();

    return 0;
}

int32
uttproc_set_context(char const *wd1, char const *wd2)
{
    int32 w1, w2;

    warn_notidle("uttproc_set_context");

    if (wd1) {
        w1 = kb_get_word_id(wd1);
        if ((w1 < 0) || (!ngram_model_set_known_wid(lmset, w1))) {
            E_ERROR("Unknown word: %s\n", wd1);
            search_set_context(-1, -1);

            return -1;
        }
    }
    else
        w1 = -1;

    if (wd2) {
        w2 = kb_get_word_id(wd2);
        if ((w2 < 0) || (!ngram_model_set_known_wid(lmset, w2))) {
            E_ERROR("Unknown word: %s\n", wd2);
            search_set_context(-1, -1);

            return -1;
        }
    }
    else
        w2 = -1;

    if (w2 < 0) {
        search_set_context(-1, -1);
        return ((w1 >= 0) ? -1 : 0);
    }
    else {
        /* Because of the perverse way search_set_context was defined... */
        if (w1 < 0)
            search_set_context(w2, -1);
        else
            search_set_context(w1, w2);
    }

    return 0;
}

int32
uttproc_set_lm(char const *lmname)
{
    warn_notidle("uttproc_set_lm");
    if (lmname == NULL) {
        E_ERROR("uttproc_set_lm called with NULL argument\n");
        return -1;
    }
    if (ngram_model_set_select(lmset, lmname) == NULL)
        return -1;

    E_INFO("LM= \"%s\"\n", lmname);
    fsg_search_mode = FALSE;
    search_set_current_lm();

    return 0;
}

char const *
uttproc_get_lm(void)
{
    return ngram_model_set_current(lmset);
}

int32
uttproc_add_word(char const *word,
                 char const *phones,
                 int update)
{
    int32 wid, lmwid;
    char *pron;

    pron = ckd_salloc(phones);
    /* Add the word to the dictionary. */
    if ((wid = dict_add_word(word_dict, word, pron)) == -1) {
        ckd_free(pron);
        return -1;
    }
    ckd_free(pron);

    /* FIXME: There is a way more efficient way to do this, since all
     * we did was replace a placeholder string with the new word
     * string - therefore what we ought to do is add it directly to
     * the current LM, then update the mapping without reallocating
     * everything. */
    /* Add it to the LM set (meaning, the current LM).  In a perfect
     * world, this would result in the same WID, but because of the
     * weird way that word IDs are handled, it doesn't. */
    if ((lmwid = ngram_model_add_word(lmset, word, 1.0)) == NGRAM_INVALID_WID)
        return -1;

    /* Rebuild the widmap and search tree if requested. */
    if (update) {
        kb_update_widmap();
        search_set_current_lm();
    }
    return wid;
}

int32
uttproc_load_fsg(s2_fsg_t * fsg,
                 int32 use_altpron,
                 int32 use_filler,
                 float32 silprob, float32 fillprob, float32 lw)
{
    word_fsg_t *word_fsg;

    word_fsg =
        word_fsg_load(fsg, use_altpron, use_filler, silprob, fillprob, lw);

    if (!word_fsg)
        return 0;

    if (!fsg_search_add_fsg(fsg_search, word_fsg)) {
        E_ERROR("Failed to add FSG '%s' to system\n",
                word_fsg_name(word_fsg));
        word_fsg_free(word_fsg);
        return 0;
    }

    return 1;
}


char *
uttproc_load_fsgfile(char *fsgfile)
{
    word_fsg_t *fsg;

    fsg = word_fsg_readfile(fsgfile,
                            cmd_ln_boolean("-fsgusealtpron"),
                            cmd_ln_boolean("-fsgusefiller"),
                            cmd_ln_float32("-silpen"),
                            cmd_ln_float32("-fillpen"),
                            cmd_ln_float32("-lw"));
    if (!fsg)
        return NULL;

    if (!fsg_search_add_fsg(fsg_search, fsg)) {
        E_ERROR("Failed to add FSG '%s' to system\n", word_fsg_name(fsg));
        word_fsg_free(fsg);
        return NULL;
    }

    return fsg->name;
}


int32
uttproc_del_fsg(char *fsgname)
{
    warn_notidle("uttproc_del_fsg");

    if (fsgname == NULL) {
        E_ERROR("uttproc_del_fsg called with NULL argument\n");
        return -1;
    }

    if (!fsg_search_del_fsg_byname(fsg_search, fsgname))
        return -1;

    return 0;
}


int32
uttproc_set_fsg(char *fsgname)
{
    warn_notidle("uttproc_set_fsg");

    if (fsgname == NULL) {
        E_ERROR("uttproc_set_fsg called with NULL argument\n");
        return -1;
    }

    if (!fsg_search_set_current_fsg(fsg_search, fsgname))
        return -1;

    fsg_search_mode = TRUE;

    E_INFO("FSG= \"%s\"\n", fsgname);

    return 0;
}



int32
uttproc_get_fsg_start_state(void)
{
    return fsg_search_get_start_state(fsg_search);
}


int32
uttproc_get_fsg_final_state(void)
{
    return fsg_search_get_final_state(fsg_search);
}


int32
uttproc_set_fsg_start_state(int32 state)
{
    return fsg_search_set_start_state(fsg_search, state);
}


int32
uttproc_set_fsg_final_state(int32 state)
{
    return fsg_search_set_final_state(fsg_search, state);
}


boolean
uttproc_fsg_search_mode(void)
{
    return fsg_search_mode;
}


int32
uttproc_set_rescore_lm(char const *lmname)
{
    searchlat_set_rescore_lm(lmname);
    return 0;
}

void
uttproc_set_feat(feat_t *new_fcb)
{
    if (fcb)
        feat_free(fcb);
    fcb = new_fcb;
    feat_alloc();
}

#if 0
int32
uttproc_set_uttid(char const *id)
{
    warn_notidle("uttproc_set_uttid");

    assert(strlen(id) < UTTIDSIZE);
    strcpy(uttid, id);

    return 0;
}
#endif

char const *
uttproc_get_uttid(void)
{
    return uttid;
}

int32
uttproc_set_auto_uttid_prefix(char const *prefix)
{
    if (uttid_prefix)
        free(uttid_prefix);
    uttid_prefix = ckd_salloc(prefix);
    uttno = 0;

    return 0;
}

void
uttproc_cepmean_set(mfcc_t * cep)
{
    warn_notidle("uttproc_cepmean_set");
    cmn_prior_set(fcb->cmn_struct, cep);
}

void
uttproc_cepmean_get(mfcc_t * cep)
{
    cmn_prior_get(fcb->cmn_struct, cep);
}

void
uttproc_agcemax_set(float32 c0max)
{
    warn_notidle("uttproc_agcemax_set");
    agc_emax_set(fcb->agc_struct, c0max);
}

float32
uttproc_agcemax_get(void)
{
    return agc_emax_get(fcb->agc_struct);
}

int32
uttproc_nosearch(int32 flag)
{
    warn_notidle("uttproc_nosearch");

    nosearch = flag;
    return 0;
}

int32
uttproc_set_rawlogdir(char const *dir)
{
    warn_notidle("uttproc_set_rawlogdir");

    if (!rawlogdir) {
        if ((rawlogdir = calloc(1024, 1)) == NULL) {
            E_ERROR("calloc(1024,1) failed\n");
            return -1;
        }
    }
    if (rawlogdir)
        strcpy(rawlogdir, dir);

    return 0;
}

int32
uttproc_set_mfclogdir(char const *dir)
{
    warn_notidle("uttproc_set_mfclogdir");

    if (!mfclogdir) {
        if ((mfclogdir = calloc(1024, 1)) == NULL) {
            E_ERROR("calloc(1024,1) failed\n");
            return -1;
        }
    }
    if (mfclogdir)
        strcpy(mfclogdir, dir);

    return 0;
}

int32
uttproc_parse_ctlfile_entry(char *line,
                            char *filename, int32 * sf, int32 * ef,
                            char *idspec)
{
    int32 k;

    /* Default; process entire file */
    *sf = 0;
    *ef = -1;

    if ((k = sscanf(line, "%s %d %d %s", filename, sf, ef, idspec)) <= 0)
        return -1;

    if (k == 1)
        strcpy(idspec, filename);
    else {
        if ((k == 2) || (*sf < 0) || (*ef <= *sf)) {
            E_ERROR("Bad ctlfile entry: %s\n", line);
            return -1;
        }
        if (k == 3)
            sprintf(idspec, "%s_%d_%d", filename, *sf, *ef);
    }

    return 0;
}

/* FIXME... use I/O stuff in sphinxbase */
static int32 adc_endian;

FILE *
adcfile_open(char const *filename)
{
    const char *adc_ext, *data_directory;
    FILE *uttfp;
    char *inputfile;
    int32 n, l, adc_hdr;

    adc_ext = cmd_ln_str("-cepext");
    adc_hdr = cmd_ln_int32("-adchdr");
    adc_endian = strcmp(cmd_ln_str("-input_endian"), "big");
    data_directory = cmd_ln_str("-cepdir");

    /* Build input filename */
    n = strlen(adc_ext);
    l = strlen(filename);
    if ((n <= l) && (0 == strcmp(filename + l - n, adc_ext)))
        adc_ext = "";          /* Extension already exists */
    if (data_directory) {
        inputfile = string_join(data_directory, "/", filename, adc_ext, NULL);
    }
    else {
        inputfile = string_join(filename, adc_ext, NULL);
    }

    if ((uttfp = fopen(inputfile, "rb")) == NULL) {
        E_FATAL("fopen(%s,rb) failed\n", inputfile);
    }
    if (adc_hdr > 0) {
        if (fseek(uttfp, adc_hdr, SEEK_SET) < 0) {
            E_ERROR("fseek(%s,%d) failed\n", inputfile, adc_hdr);
            fclose(uttfp);
            ckd_free(inputfile);
            return NULL;
        }
    }
#ifdef WORDS_BIGENDIAN
    if (adc_endian == 1)    /* Little endian adc file */
        E_INFO("Byte-reversing %s\n", inputfile);
#else
    if (adc_endian == 0)    /* Big endian adc file */
        E_INFO("Byte-reversing %s\n", inputfile);
#endif
    ckd_free(inputfile);

    return uttfp;
}

int32
adc_file_read(FILE *uttfp, int16 * buf, int32 max)
{
    int32 i, n;

    if (uttfp == NULL)
        return -1;

    if ((n = fread(buf, sizeof(int16), max, uttfp)) <= 0)
        return -1;

    /* Byte swap if necessary */
#ifdef WORDS_BIGENDIAN
    if (adc_endian == 1) {      /* Little endian adc file */
        for (i = 0; i < n; i++)
            SWAP_INT16(&buf[i]);
    }
#else
    if (adc_endian == 0) {      /* Big endian adc file */
        for (i = 0; i < n; i++)
            SWAP_INT16(&buf[i]);
    }
#endif

    return n;
}

int32
uttproc_decode_raw_file(const char *filename, 
                        const char *uttid,
                        int32 sf, int32 ef, int32 nosearch)
{
    int16 *adbuf;
    FILE *uttfp;
    int32 k;

    inputtype = INPUT_RAW;
    if ((uttfp = adcfile_open(filename)) == NULL)
        return -1;

    if (uttproc_nosearch(nosearch) < 0) {
        fclose(uttfp);
        return -1;
    }

    if (uttproc_begin_utt(uttid) < 0) {
        fclose(uttfp);
        return -1;
    }

    adbuf = ckd_calloc(4096, sizeof(int16));
    while ((k = adc_file_read(uttfp, adbuf, 4096)) >= 0) {
        if (uttproc_rawdata(adbuf, k, 1) < 0) {
            fclose(uttfp);
            ckd_free(adbuf);
            return -1;
        }
    }
    fclose(uttfp);
    ckd_free(adbuf);

    if (uttproc_end_utt() < 0)
        return -1;

    return n_featfr;
}

int32
uttproc_decode_cep_file(const char *filename,
                        const char *uttid,
                        int32 sf, int32 ef, int32 nosearch)
{
    if (uttproc_nosearch(nosearch) < 0)
        return -1;

    if (uttproc_begin_utt(uttid) < 0)
        return -1;

    n_cepfr = 0;
    n_featfr = feat_s2mfc2feat(fcb, filename,
                               cmd_ln_str("-cepdir"),
                               cmd_ln_str("-cepext"),
                               sf, ef, feat_buf, MAX_UTT_LEN);

    if (nosearch == FALSE) {
        while (n_searchfr < n_featfr)
            uttproc_frame();
    }

    if (uttproc_end_utt() < 0)
        return -1;

    return n_featfr;
}

static FILE *logfp;
static char logfile[MAXPATHLEN]; /* FIXME buffer */
int32
uttproc_set_logfile(char const *file)
{
    FILE *fp;

    E_INFO("uttproc_set_logfile(%s)\n", file);

    if ((fp = fopen(file, "w")) == NULL) {
        E_ERROR("fopen(%s,w) failed; logfile unchanged\n", file);
        return -1;
    }
    else {
        if (logfp)
            fclose(logfp);

        logfp = fp;
        /* 
         * Rolled back the dup2() bug fix for windows only. In
         * Microsoft Visual C, dup2 seems to cause problems in some
         * applications: the files are opened, but nothing is written
         * to it.
         */
#if defined(_WIN32) || defined(GNUWINCE)
#ifndef _WIN32_WCE /* FIXME: Possible? */
        *stdout = *logfp;
        *stderr = *logfp;
#endif
#else
        dup2(fileno(logfp), 1);
        dup2(fileno(logfp), 2);
#endif

        E_INFO("Previous logfile: '%s'\n", logfile);
        strcpy(logfile, file);
    }

    return 0;
}
