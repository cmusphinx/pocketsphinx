/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2008 Carnegie Mellon University.  All rights
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
 * @file ngram_search_dag.c Word graph search.
 */

/* System headers. */
#include <assert.h>
#include <string.h>

/* SphinxBase headers. */
#include <ckd_alloc.h>
#include <listelem_alloc.h>

/* Local headers. */
#include "pocketsphinx.h" /* To make sure ps_lattice_write() gets exported */
#include "ps_lattice.h"

/*
 * Create a directed link between "from" and "to" nodes, but if a link already exists,
 * choose one with the best link_scr.
 */
void
link_latnodes(ps_lattice_t *dag, latnode_t *from, latnode_t *to, int32 score, int32 ef)
{
    latlink_t *link;

    assert(to->reachable);

    /* Look for an existing link between "from" and "to" nodes */
    for (link = from->links; link && (link->to != to); link = link->next);

    if (!link) {
        /* No link between the two nodes; create a new one */
        link = listelem_malloc(dag->latlink_alloc);

        link->from = from;
        link->to = to;
        link->link_scr = score;
        link->ef = ef;
        link->best_prev = NULL;

        link->next = from->links;
        from->links = link;
    }
    else {
        /* Link already exists; just retain the best link_scr */
        if (link->link_scr < score) {
            link->link_scr = score;
            link->ef = ef;
        }
    }
}

void
ps_lattice_bypass_fillers(ps_lattice_t *dag, int32 silpen, int32 fillpen)
{
    latnode_t *node, *to, *from, *prev_node, *t_node;
    latlink_t *link, *f_link, *t_link, *prev_link;
    rev_latlink_t *revlink, *t_revlink;
    int32 score;

    /* Create reverse links for all links pointing to filler nodes */
    for (node = dag->nodes; node; node = node->next) {
        for (link = node->links; link; link = link->next) {
            to = link->to;
            if (to != dag->end && ISA_FILLER_WORD(dag->search, to->basewid)) {
                revlink = listelem_malloc(dag->rev_latlink_alloc);
                revlink->link = link;
                revlink->next = to->revlinks;
                to->revlinks = revlink;
            }
        }
    }

    /* Bypass filler nodes */
    for (node = dag->nodes; node; node = node->next) {
        if (node == dag->end || !ISA_FILLER_WORD(dag->search, node->basewid))
            continue;

        /* Replace each link entering filler node with links to all its successors */
        for (revlink = node->revlinks; revlink; revlink = revlink->next) {
            link = revlink->link;       /* link entering filler node */
            from = link->from;

            score = (node->basewid == ps_search_silence_wid(dag->search)) ? silpen : fillpen;
            score += link->link_scr;

            /*
             * Make links from predecessor of filler (from) to successors of filler.
             * But if successor is a filler, it has already been eliminated since it
             * appears earlier in latnode_list (see build...).  So it can be skipped.
             * Likewise, no reverse links needed for the new links; none of them
             * points to a filler node.
             */
            for (f_link = node->links; f_link; f_link = f_link->next) {
                if (!ISA_FILLER_WORD(dag->search, f_link->to->basewid))
                    link_latnodes(dag, from, f_link->to,
                                  score + f_link->link_scr, link->ef);
            }
        }
    }

    /* Delete filler nodes and all links and reverse links from it */
    prev_node = NULL;
    for (node = dag->nodes; node; node = t_node) {
        t_node = node->next;
        if (node != dag->end && ISA_FILLER_WORD(dag->search, node->basewid)) {
            for (revlink = node->revlinks; revlink; revlink = t_revlink) {
                t_revlink = revlink->next;
                revlink->link->to = NULL;
                listelem_free(dag->rev_latlink_alloc, revlink);
            }

            for (link = node->links; link; link = t_link) {
                t_link = link->next;
                listelem_free(dag->latlink_alloc, link);
            }

            if (prev_node)
                prev_node->next = t_node;
            else
                dag->nodes = t_node;

            listelem_free(dag->latnode_alloc, node);
        }
        else
            prev_node = node;
    }

    /* Reclaim links pointing nowhere */
    for (node = dag->nodes; node; node = node->next) {
        prev_link = NULL;
        for (link = node->links; link; link = t_link) {
            t_link = link->next;
            if (link->to == NULL) {
                if (prev_link)
                    prev_link->next = t_link;
                else
                    node->links = t_link;
                listelem_free(dag->latlink_alloc, link);
            }
            else
                prev_link = link;
        }
    }
}

void
ps_lattice_delete_unreachable(ps_lattice_t *dag)
{
    latnode_t *node, *t_node, *prev_node;
    latlink_t *link, *t_link;

    prev_node = NULL;
    for (node = dag->nodes; node; node = t_node) {
        t_node = node->next;
        if (!node->reachable) {
            /* Node and its links can be removed */
            if (prev_node)
                prev_node->next = node->next;
            else
                dag->nodes = node->next;
            for (link = node->links; link; link = t_link) {
                t_link = link->next;
                listelem_free(dag->latlink_alloc, link);
            }
            listelem_free(dag->latnode_alloc, node);
        }
        else
            prev_node = node;
    }
}

static int32
latnode_seqid(ps_lattice_t *dag, latnode_t * target)
{
    int32 i;
    latnode_t *d;

    for (i = 0, d = dag->nodes; d && (d != target); d = d->next, i++);
    assert(d);
    return (i);
}

int32
ps_lattice_write(ps_lattice_t *dag, char const *filename)
{
    FILE *fp;
    int32 i;
    latnode_t *d, *initial, *final;

    initial = dag->start;
    final = dag->end;

    E_INFO("Writing lattice file: %s\n", filename);
    if ((fp = fopen(filename, "w")) == NULL) {
        E_ERROR("fopen(%s,w) failed\n", filename);
        return -1;
    }

    fprintf(fp,
            "# Generated by PocketSphinx\n");
    fprintf(fp, "# -logbase %e\n",
            cmd_ln_float32_r(ps_search_config(dag->search), "-logbase"));
    fprintf(fp, "#\n");

    fprintf(fp, "Frames %d\n", dag->n_frames);
    fprintf(fp, "#\n");

    for (i = 0, d = dag->nodes; d; d = d->next, i++);
    fprintf(fp,
            "Nodes %d (NODEID WORD STARTFRAME FIRST-ENDFRAME LAST-ENDFRAME)\n",
            i);
    for (i = 0, d = dag->nodes; d; d = d->next, i++) {
        fprintf(fp, "%d %s %d %d %d\n",
                i, dict_word_str(ps_search_dict(dag->search), d->wid),
                d->sf, d->fef, d->lef);
    }
    fprintf(fp, "#\n");

    fprintf(fp, "Initial %d\nFinal %d\n", latnode_seqid(dag, initial),
            latnode_seqid(dag, final));
    fprintf(fp, "#\n");

    /* Don't bother with this, it's not used by anything. */
    fprintf(fp, "BestSegAscr %d (NODEID ENDFRAME ASCORE)\n",
            0 /* #BPTable entries */ );
    fprintf(fp, "#\n");

    fprintf(fp, "Edges (FROM-NODEID TO-NODEID ASCORE)\n");
    for (d = dag->nodes; d; d = d->next) {
        latlink_t *l;
        for (l = d->links; l; l = l->next)
            fprintf(fp, "%d %d %d\n",
                    latnode_seqid(dag, d), latnode_seqid(dag, l->to),
                    l->link_scr);
    }
    fprintf(fp, "End\n");
    fclose(fp);

    return 0;
}

ps_lattice_t *
ps_lattice_init(ps_search_t *search, int n_frame)
{
    ps_lattice_t *dag;

    dag = ckd_calloc(1, sizeof(*dag));
    dag->search = search;
    dag->n_frames = n_frame;
    dag->latnode_alloc = listelem_alloc_init(sizeof(latnode_t));
    dag->latlink_alloc = listelem_alloc_init(sizeof(latlink_t));
    dag->rev_latlink_alloc = listelem_alloc_init(sizeof(rev_latlink_t));
    return dag;
}

void
ps_lattice_free(ps_lattice_t *dag)
{
    if (dag == NULL)
        return;
    listelem_alloc_free(dag->latnode_alloc);
    listelem_alloc_free(dag->latlink_alloc);
    listelem_alloc_free(dag->rev_latlink_alloc);
    ckd_free(dag->hyp_str);
    ckd_free(dag);
}

char const *
ps_lattice_hyp(ps_lattice_t *dag, latlink_t *link)
{
    latlink_t *l;
    size_t len;
    char *c;

    /* Backtrace once to get hypothesis length. */
    len = 0;
    if (ISA_REAL_WORD(dag->search, link->to->basewid))
        len += strlen(dict_word_str(ps_search_dict(dag->search), link->to->basewid)) + 1;
    for (l = link; l; l = l->best_prev) {
        if (ISA_REAL_WORD(dag->search, l->from->basewid))
            len += strlen(dict_word_str(ps_search_dict(dag->search), l->from->basewid)) + 1;
    }

    /* Backtrace again to construct hypothesis string. */
    ckd_free(dag->hyp_str);
    dag->hyp_str = ckd_calloc(1, len);
    c = dag->hyp_str + len - 1;
    if (ISA_REAL_WORD(dag->search, link->to->basewid)) {
        len = strlen(dict_word_str(ps_search_dict(dag->search), link->to->basewid));
        c -= len;
        memcpy(c, dict_word_str(ps_search_dict(dag->search), link->to->basewid), len);
        if (c > dag->hyp_str) {
            --c;
            *c = ' ';
        }
    }
    for (l = link; l; l = l->best_prev) {
        if (ISA_REAL_WORD(dag->search, l->from->basewid)) {
            len = strlen(dict_word_str(ps_search_dict(dag->search), l->from->basewid));
            c -= len;
            memcpy(c, dict_word_str(ps_search_dict(dag->search), l->from->basewid), len);
            if (c > dag->hyp_str) {
                --c;
                *c = ' ';
            }
        }
    }

    return dag->hyp_str;
}

static void
ngram_search_link2itor(ps_seg_t *seg, latlink_t *link, int to)
{
    latnode_t *node;

    if (to) {
        node = link->to;
        seg->ef = node->lef;
    }
    else {
        node = link->from;
        seg->ef = link->ef;
    }

    seg->word = dict_word_str(ps_search_dict(seg->search), node->wid);
    seg->sf = node->sf;
    seg->prob = 0; /* FIXME: implement forward-backward */
}

static void
ps_lattice_seg_free(ps_seg_t *seg)
{
    dag_seg_t *itor = (dag_seg_t *)seg;
    
    ckd_free(itor->links);
    ckd_free(itor);
}

static ps_seg_t *
ps_lattice_seg_next(ps_seg_t *seg)
{
    dag_seg_t *itor = (dag_seg_t *)seg;

    ++itor->cur;
    if (itor->cur == itor->n_links + 1) {
        ps_lattice_seg_free(seg);
        return NULL;
    }
    else if (itor->cur == itor->n_links) {
        /* Re-use the last link but with the "to" node. */
        ngram_search_link2itor(seg, itor->links[itor->cur - 1], TRUE);
    }
    else {
        ngram_search_link2itor(seg, itor->links[itor->cur], FALSE);
    }

    return seg;
}

static ps_segfuncs_t ps_lattice_segfuncs = {
    /* seg_next */ ps_lattice_seg_next,
    /* seg_free */ ps_lattice_seg_free
};

ps_seg_t *
ps_lattice_iter(ps_lattice_t *dag, latlink_t *link)
{
    dag_seg_t *itor;
    latlink_t *l;
    int cur;

    /* Calling this an "iterator" is a bit of a misnomer since we have
     * to get the entire backtrace in order to produce it.
     */
    itor = ckd_calloc(1, sizeof(*itor));
    itor->base.vt = &ps_lattice_segfuncs;
    itor->base.search = dag->search;
    itor->n_links = 0;

    for (l = link; l; l = l->best_prev) {
        ++itor->n_links;
    }
    itor->links = ckd_calloc(itor->n_links, sizeof(*itor->links));
    cur = itor->n_links - 1;
    for (l = link; l; l = l->best_prev) {
        itor->links[cur] = l;
        --cur;
    }

    ngram_search_link2itor((ps_seg_t *)itor, itor->links[0], FALSE);
    return (ps_seg_t *)itor;
}

/*
 * Find the best score from dag->start to end point of any link and
 * use it to update links further down the path.  This is basically
 * just single-source shortest path search:
 *
 * http://en.wikipedia.org/wiki/Dijkstra's_algorithm
 *
 * The implementation is a bit different though.  Needs documented.
 */
latlink_t *
ps_lattice_bestpath(ps_lattice_t *dag, ngram_model_t *lmset, float32 lwf)
{
    ps_search_t *search;
    latnode_t *node;
    latlink_t *link, *best;
    latlink_t *q_head, *q_tail;
    int32 score;

    search = dag->search;

    /* Initialize node fanin counts and path scores */
    for (node = dag->nodes; node; node = node->next)
        node->info.fanin = 0;
    for (node = dag->nodes; node; node = node->next) {
        for (link = node->links; link; link = link->next) {
            (link->to->info.fanin)++;
            link->path_scr = (int32) 0x80000000;
        }
    }

    /*
     * Form initial queue of optimal partial paths; = links out of start_node.
     * Path score includes LM score for the "to" node of the last link in the
     * path, but not the acoustic score for that node.
     */
    q_head = q_tail = NULL;
    for (link = dag->start->links; link; link = link->next) {
        int32 n_used;

        assert(link->to == dag->end || !ISA_FILLER_WORD(search, link->to->basewid));
        link->path_scr = link->link_scr +
            ngram_bg_score(lmset, link->to->basewid,
                           ps_search_start_wid(search), &n_used) * lwf;
        link->best_prev = NULL;

        if (!q_head)
            q_head = link;
        else
            q_tail->q_next = link;
        q_tail = link;
    }
    q_tail->q_next = NULL;

    /* Extend partial paths in queue as long as queue not empty */
    while (q_head) {
        /* Update path score for all possible links out of q_head->to */
        node = q_head->to;

#if 0
        E_INFO("QHD %s.%d -> %s.%d (%d, %d)\n",
               dict_word_str(search->dict, q_head->from->basewid), q_head->from->sf,
               dict_word_str(search->dict, node->basewid), node->sf,
               q_head->link_scr, q_head->path_scr);
#endif

        for (link = node->links; link; link = link->next) {
            int32 n_used;

            assert(link->to == dag->end || !ISA_FILLER_WORD(search, link->to->basewid));

            score = q_head->path_scr + link->link_scr +
                ngram_tg_score(lmset, link->to->basewid,
                               node->basewid, q_head->from->basewid, &n_used) * lwf;

            if (score > link->path_scr) {
                link->path_scr = score;
                link->best_prev = q_head;
            }
        }

        if (--(node->info.fanin) == 0) {
            /*
             * Links out of node (q_head->to) updated wrt all incoming links at node.
             * They all now have optimal partial path scores; insert them in optimal
             * partial path queue.
             */
            for (link = node->links; link; link = link->next) {
                q_tail->q_next = link;
                q_tail = link;
            }
            q_tail->q_next = NULL;
        }

        q_head = q_head->q_next;
    }

    /*
     * Rescored all paths to dag->end; Find the best link entering
     * dag->end.  We will trace this back to get a hypothesis.
     */
    score = (int32) 0x80000000;
    best = NULL;
    for (node = dag->nodes; node; node = node->next) {
        for (link = node->links; link; link = link->next)
            if ((link->to == dag->end) && (link->path_scr > score)) {
                score = link->path_scr;
                best = link;
            }
    }
    return best;
}

/* Parameters to prune n-best alternatives search */
#define MAX_PATHS	500     /* Max allowed active paths at any time */
#define MAX_HYP_TRIES	10000

/*
 * For each node in any path between from and end of utt, find the
 * best score from "from".sf to end of utt.  (NOTE: Uses bigram probs;
 * this is an estimate of the best score from "from".)  (NOTE #2: yes,
 * this is the "heuristic score" used in A* search)
 */
static int32
best_rem_score(ps_astar_t *nbest, latnode_t * from)
{
    ps_lattice_t *dag;
    latlink_t *link;
    int32 bestscore, score;

    dag = nbest->dag;
    if (from->info.rem_score <= 0)
        return (from->info.rem_score);

    /* Best score from "from" to end of utt not known; compute from successors */
    bestscore = WORST_SCORE;
    for (link = from->links; link; link = link->next) {
        int32 n_used;

        score = best_rem_score(nbest, link->to);
        score += link->link_scr;
        score += ngram_bg_score(nbest->lmset, link->to->basewid,
                                from->basewid, &n_used) * nbest->lwf;
        if (score > bestscore)
            bestscore = score;
    }
    from->info.rem_score = bestscore;

    return bestscore;
}

/*
 * Insert newpath in sorted (by path score) list of paths.  But if newpath is
 * too far down the list, drop it (FIXME: necessary?)
 * total_score = path score (newpath) + rem_score to end of utt.
 */
static void
path_insert(ps_astar_t *nbest, latpath_t *newpath, int32 total_score)
{
    ps_lattice_t *dag;
    latpath_t *prev, *p;
    int32 i;

    dag = nbest->dag;
    prev = NULL;
    for (i = 0, p = nbest->path_list; (i < MAX_PATHS) && p; p = p->next, i++) {
        if ((p->score + p->node->info.rem_score) < total_score)
            break;
        prev = p;
    }

    /* newpath should be inserted between prev and p */
    if (i < MAX_PATHS) {
        /* Insert new partial hyp */
        newpath->next = p;
        if (!prev)
            nbest->path_list = newpath;
        else
            prev->next = newpath;
        if (!p)
            nbest->path_tail = newpath;

        nbest->n_path++;
        nbest->n_hyp_insert++;
        nbest->insert_depth += i;
    }
    else {
        /* newpath score too low; reject it and also prune paths beyond MAX_PATHS */
        nbest->path_tail = prev;
        prev->next = NULL;
        nbest->n_path = MAX_PATHS;
        listelem_free(nbest->latpath_alloc, newpath);

        nbest->n_hyp_reject++;
        for (; p; p = newpath) {
            newpath = p->next;
            listelem_free(nbest->latpath_alloc, p);
            nbest->n_hyp_reject++;
        }
    }
}

/* Find all possible extensions to given partial path */
static void
path_extend(ps_astar_t *nbest, latpath_t * path)
{
    latlink_t *link;
    latpath_t *newpath;
    int32 total_score, tail_score;
    ps_lattice_t *dag;

    dag = nbest->dag;

    /* Consider all successors of path->node */
    for (link = path->node->links; link; link = link->next) {
        int32 n_used;

        /* Skip successor if no path from it reaches the final node */
        if (link->to->info.rem_score <= WORST_SCORE)
            continue;

        /* Create path extension and compute exact score for this extension */
        newpath = listelem_malloc(nbest->latpath_alloc);
        newpath->node = link->to;
        newpath->parent = path;
        newpath->score = path->score + link->link_scr;
        if (path->parent) {
            newpath->score += nbest->lwf
                * ngram_tg_score(nbest->lmset, newpath->node->basewid,
                                 path->node->basewid,
                                 path->parent->node->basewid, &n_used);
        }
        else 
            newpath->score += nbest->lwf
                * ngram_bg_score(nbest->lmset, newpath->node->basewid,
                                 path->node->basewid, &n_used);

        /* Insert new partial path hypothesis into sorted path_list */
        nbest->n_hyp_tried++;
        total_score = newpath->score + newpath->node->info.rem_score;

        /* First see if hyp would be worse than the worst */
        if (nbest->n_path >= MAX_PATHS) {
            tail_score =
                nbest->path_tail->score
                + nbest->path_tail->node->info.rem_score;
            if (total_score < tail_score) {
                listelem_free(nbest->latpath_alloc, newpath);
                nbest->n_hyp_reject++;
                continue;
            }
        }

        path_insert(nbest, newpath, total_score);
    }
}

ps_astar_t *
ps_astar_start(ps_lattice_t *dag,
                  ngram_model_t *lmset,
                  float32 lwf,
                  int sf, int ef,
                  int w1, int w2)
{
    ps_astar_t *nbest;
    latnode_t *node;

    nbest = ckd_calloc(1, sizeof(*nbest));
    nbest->dag = dag;
    nbest->lmset = lmset;
    nbest->lwf = lwf;
    nbest->sf = sf;
    if (ef < 0)
        nbest->ef = dag->n_frames - ef;
    else
        nbest->ef = ef;
    nbest->w1 = w1;
    nbest->w2 = w2;
    nbest->latpath_alloc = listelem_alloc_init(sizeof(latpath_t));

    /* Initialize rem_score (A* heuristic) to default values */
    for (node = dag->nodes; node; node = node->next) {
        if (node == dag->end)
            node->info.rem_score = 0;
        else if (!node->links)
            node->info.rem_score = WORST_SCORE;
        else
            node->info.rem_score = 1;   /* +ve => unknown value */
    }

    /* Create initial partial hypotheses list consisting of nodes starting at sf */
    nbest->path_list = nbest->path_tail = NULL;
    for (node = dag->nodes; node; node = node->next) {
        if (node->sf == sf) {
            latpath_t *path;
            int32 n_used, scr;

            best_rem_score(nbest, node);
            path = listelem_malloc(nbest->latpath_alloc);
            path->node = node;
            path->parent = NULL;
            scr = nbest->lwf *
                (w1 < 0)
                ? ngram_bg_score(nbest->lmset, node->basewid, w2, &n_used)
                : ngram_tg_score(nbest->lmset, node->basewid, w2, w1, &n_used);
            path->score = scr;
            path_insert(nbest, path, scr + node->info.rem_score);
        }
    }

    return nbest;
}

latpath_t *
ps_astar_next(ps_astar_t *nbest)
{
    latpath_t *top;
    ps_lattice_t *dag;

    dag = nbest->dag;

    /* Pop the top (best) partial hypothesis */
    while ((top = nbest->path_list) != NULL) {
        nbest->path_list = nbest->path_list->next;
        if (top == nbest->path_tail)
            nbest->path_tail = NULL;
        nbest->n_path--;

        /* Complete hypothesis? */
        if ((top->node->sf >= nbest->ef)
            || ((top->node == dag->end) &&
                (nbest->ef > dag->end->sf))) {
            /* FIXME: Verify that it is non-empty. */
            return top;
        }
        else {
            if (top->node->fef < nbest->ef)
                path_extend(nbest, top);
        }

        /*
         * Add top to paths already processed; cannot be freed because other paths
         * point to it.
         */
        top->next = nbest->paths_done;
        nbest->paths_done = top;
    }

    /* Did not find any more paths to extend. */
    return NULL;
}

char const *
ps_astar_hyp(ps_astar_t *nbest, latpath_t *path)
{
    ps_search_t *search;
    latpath_t *p;
    size_t len;
    char *c;
    char *hyp;

    search = nbest->dag->search;

    /* Backtrace once to get hypothesis length. */
    len = 0;
    for (p = path; p; p = p->parent) {
        if (ISA_REAL_WORD(search, p->node->basewid))
            len += strlen(dict_word_str(ps_search_dict(search), p->node->basewid)) + 1;
    }

    /* Backtrace again to construct hypothesis string. */
    hyp = ckd_calloc(1, len);
    c = hyp + len - 1;
    for (p = path; p; p = p->parent) {
        if (ISA_REAL_WORD(search, p->node->basewid)) {
            len = strlen(dict_word_str(ps_search_dict(search), p->node->basewid));
            c -= len;
            memcpy(c, dict_word_str(ps_search_dict(search), p->node->basewid), len);
            if (c > hyp) {
                --c;
                *c = ' ';
            }
        }
    }

    nbest->hyps = glist_add_ptr(nbest->hyps, hyp);
    return hyp;
}

void
ps_astar_finish(ps_astar_t *nbest)
{
    gnode_t *gn;

    /* Free all hyps. */
    for (gn = nbest->hyps; gn; gn = gnode_next(gn)) {
        ckd_free(gnode_ptr(gn));
    }
    glist_free(nbest->hyps);
    /* Free all paths. */
    listelem_alloc_free(nbest->latpath_alloc);
    /* Free the Henge. */
    ckd_free(nbest);
}
