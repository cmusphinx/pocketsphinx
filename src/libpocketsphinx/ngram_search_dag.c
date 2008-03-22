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
#include "ngram_search_dag.h"

/*
 * Create a directed link between "from" and "to" nodes, but if a link already exists,
 * choose one with the best link_scr.
 */
static void
link_latnodes(ngram_dag_t *dag, latnode_t *from, latnode_t *to, int32 score, int32 ef)
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

static void
bypass_filler_nodes(ngram_dag_t *dag)
{
    latnode_t *node, *to, *from, *prev_node, *t_node;
    latlink_t *link, *f_link, *t_link, *prev_link;
    rev_latlink_t *revlink, *t_revlink;
    int32 score;

    /* Create reverse links for all links pointing to filler nodes */
    for (node = dag->nodes; node; node = node->next) {
        for (link = node->links; link; link = link->next) {
            to = link->to;
            if (ISA_FILLER_WORD(dag->ngs, to->wid)) {
                revlink = listelem_malloc(dag->rev_latlink_alloc);
                revlink->link = link;
                revlink->next = to->revlinks;
                to->revlinks = revlink;
            }
        }
    }

    /* Bypass filler nodes */
    for (node = dag->nodes; node; node = node->next) {
        if (!ISA_FILLER_WORD(dag->ngs, node->wid))
            continue;

        /* Replace each link entering filler node with links to all its successors */
        for (revlink = node->revlinks; revlink; revlink = revlink->next) {
            link = revlink->link;       /* link entering filler node */
            from = link->from;

            score = (node->wid == dag->ngs->silence_wid)
                ? dag->ngs->silpen : dag->ngs->fillpen;
            score += link->link_scr;

            /*
             * Make links from predecessor of filler (from) to successors of filler.
             * But if successor is a filler, it has already been eliminated since it
             * appears earlier in latnode_list (see build...).  So it can be skipped.
             * Likewise, no reverse links needed for the new links; none of them
             * points to a filler node.
             */
            for (f_link = node->links; f_link; f_link = f_link->next) {
                if (!ISA_FILLER_WORD(dag->ngs, f_link->to->wid))
                    link_latnodes(dag, from, f_link->to,
                                  score + f_link->link_scr, link->ef);
            }
        }
    }

    /* Delete filler nodes and all links and reverse links from it */
    prev_node = NULL;
    for (node = dag->nodes; node; node = t_node) {
        t_node = node->next;
        if (ISA_FILLER_WORD(dag->ngs, node->wid)) {
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

static void
mark_reachable(latnode_t * from)
{
    latlink_t *link;

    from->reachable = TRUE;
    for (link = from->links; link; link = link->next) {
        if (!link->to->reachable)
            mark_reachable(link->to);
    }
}


static void
delete_unreachable(ngram_dag_t *dag)
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
latnode_seqid(ngram_dag_t *dag, latnode_t * target)
{
    int32 i;
    latnode_t *d;

    for (i = 0, d = dag->nodes; d && (d != target); d = d->next, i++);
    assert(d);
    return (i);
}

int32
ngram_dag_write(ngram_dag_t *dag, char *filename)
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
    fprintf(fp, "# -logbase %e\n", cmd_ln_float32_r(dag->ngs->config, "-logbase"));
    fprintf(fp, "#\n");

    fprintf(fp, "Frames %d\n", dag->n_frames);
    fprintf(fp, "#\n");

    for (i = 0, d = dag->nodes; d; d = d->next, i++);
    fprintf(fp,
            "Nodes %d (NODEID WORD STARTFRAME FIRST-ENDFRAME LAST-ENDFRAME)\n",
            i);
    for (i = 0, d = dag->nodes; d; d = d->next, i++) {
        fprintf(fp, "%d %s %d %d %d\n",
                i, dag->ngs->dict->dict_list[d->wid]->word, d->sf, d->fef, d->lef);
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

static void
create_dag_nodes(ngram_search_t *ngs, ngram_dag_t *dag)
{
    bptbl_t *bp_ptr;
    int32 i;

    for (i = 0, bp_ptr = ngs->bp_table; i < ngs->bp_table_size; ++i, ++bp_ptr) {
        int32 sf, ef, wid;
        latnode_t *node;

        if (!bp_ptr->valid)
            continue;

        sf = (bp_ptr->bp < 0) ? 0 : ngs->bp_table[bp_ptr->bp].frame + 1;
        ef = bp_ptr->frame;
        wid = bp_ptr->wid;

        /* Skip non-final </s> entries. */
        if ((wid == ngs->finish_wid) && (ef < dag->n_frames - 1))
            continue;

        /* Skip if word not in LM */
        if ((!ISA_FILLER_WORD(ngs, wid))
            && (!ngram_model_set_known_wid(ngs->lmset,
                                           ngs->dict->dict_list[wid]->wid)))
            continue;

        /* See if bptbl entry <wid,sf> already in lattice */
        for (node = dag->nodes; node; node = node->next) {
            if ((node->wid == wid) && (node->sf == sf))
                break;
        }

        /* For the moment, store bptbl indices in node.{fef,lef} */
        if (node)
            node->lef = i;
        else {
            /* New node; link to head of list */
            node = listelem_malloc(dag->latnode_alloc);
            node->wid = wid;
            node->sf = sf; /* This is a frame index. */
            node->fef = node->lef = i; /* These are backpointer indices (argh) */
            node->reachable = FALSE;
            node->links = NULL;
            node->revlinks = NULL;

            node->next = dag->nodes;
            dag->nodes = node;
        }
    }

    /*
     * At this point, dag->nodes is ordered such that nodes earlier in the list
     * can follow (in time) those later in the list, but not vice versa.  Now
     * create precedence links and simultanesously mark all nodes that can reach
     * the final </s> node.  (All nodes are reached from <s>.0; no problem there.)
     */
}

static latnode_t *
find_start_node(ngram_search_t *ngs, ngram_dag_t *dag)
{
    latnode_t *node;

    /* Find start node <s>.0 */
    for (node = dag->nodes; node; node = node->next) {
        if ((node->wid == ngs->start_wid) && (node->sf == 0))
            break;
    }
    if (!node) {
        /* This is probably impossible. */
        E_ERROR("Couldn't find <s> in first frame\n");
        return NULL;
    }
    return node;
}

static latnode_t *
find_end_node(ngram_search_t *ngs, ngram_dag_t *dag, float32 lwf)
{
    latnode_t *node;
    int32 ef, bestbp, bp, bestscore;

    /* Find final node </s>.last_frame; nothing can follow this node */
    for (node = dag->nodes; node; node = node->next) {
        int32 lef = ngs->bp_table[node->lef].frame;
        if ((node->wid == ngs->finish_wid) && (lef == dag->n_frames - 1))
            break;
    }
    if (node != NULL)
        return node;

    /* It is quite likely that no </s> exited in the last frame.  So,
     * find the node corresponding to the best exit. */
    /* Find the last frame containing a word exit. */
    for (ef = dag->n_frames - 1;
         ef >= 0 && ngs->bp_table_idx[ef] == ngs->bpidx;
         --ef);
    if (ef < 0) {
        E_ERROR("Empty backpointer table: can not build DAG.\n");
        return NULL;
    }

    /* Find best word exit in that frame. */
    bestscore = WORST_SCORE;
    bestbp = NO_BP;
    for (bp = ngs->bp_table_idx[ef]; bp < ngs->bp_table_idx[ef + 1]; ++bp) {
        int32 n_used, l_scr;
        l_scr = ngram_tg_score(ngs->lmset, ngs->finish_wid,
                               ngs->bp_table[bp].real_wid,
                               ngs->bp_table[bp].prev_real_wid,
                               &n_used);
        l_scr = l_scr * lwf;

        if (ngs->bp_table[bp].score + l_scr > bestscore) {
            bestscore = ngs->bp_table[bp].score + l_scr;
            bestbp = bp;
        }
    }
    E_WARN("</s> not found in last frame, using %s.%d instead\n",
            ngs->dict->dict_list[ngs->bp_table[bestbp].real_wid]->word, ef);

    /* Now find the node that corresponds to it. */
    for (node = dag->nodes; node; node = node->next) {
        if (node->lef == bestbp)
            return node;
    }

    E_ERROR("Failed to find DAG node corresponding to %s.%d\n",
            ngs->dict->dict_list[ngs->bp_table[bestbp].real_wid]->word, ef);
    return NULL;
}

/*
 * Build lattice from bptable.
 */
ngram_dag_t *
ngram_dag_build(ngram_search_t *ngs)
{
    int32 i, ef, lef, score, bss_offset;
    latnode_t *node, *from, *to;
    ngram_dag_t *dag;

    dag = ckd_calloc(1, sizeof(*dag));
    dag->ngs = ngs;
    dag->n_frames = ngs->n_frame;
    dag->latnode_alloc = listelem_alloc_init(sizeof(latnode_t));
    dag->latlink_alloc = listelem_alloc_init(sizeof(latlink_t));
    dag->rev_latlink_alloc = listelem_alloc_init(sizeof(rev_latlink_t));

    ngram_compute_seg_scores(ngs, ngs->bestpath_fwdtree_lw_ratio);
    create_dag_nodes(ngs, dag);
    if ((dag->start = find_start_node(ngs, dag)) == NULL)
        goto error_out;
    if ((dag->end = find_end_node(ngs, dag, ngs->bestpath_fwdtree_lw_ratio)) == NULL)
        goto error_out;
    dag->final_node_ascr = ngs->bp_table[dag->end->lef].ascr;

    /*
     * Create precedence links between all possible pairs of nodes.  However,
     * ignore nodes from which dag->end is not reachable.
     */
    dag->end->reachable = TRUE;
    for (to = dag->end; to; to = to->next) {
        /* Skip if not reachable; it will never be reachable from dag->end */
        if (!to->reachable)
            continue;

        /* Find predecessors of to : from->fef+1 <= to->sf <= from->lef+1 */
        for (from = to->next; from; from = from->next) {
            bptbl_t *bp_ptr;

            ef = ngs->bp_table[from->fef].frame;
            lef = ngs->bp_table[from->lef].frame;

            if ((to->sf <= ef) || (to->sf > lef + 1))
                continue;

            /* Find bptable entry for "from" that exactly precedes "to" */
            i = from->fef;
            bp_ptr = ngs->bp_table + i;
            for (; i <= from->lef; i++, bp_ptr++) {
                if (bp_ptr->wid != from->wid)
                    continue;
                if (bp_ptr->frame >= to->sf - 1)
                    break;
            }

            if ((i > from->lef) || (bp_ptr->frame != to->sf - 1))
                continue;

            /* Find acoustic score from.sf->to.sf-1 with right context = to */
            if (bp_ptr->r_diph >= 0)
                bss_offset =
                    ngs->dict->rcFwdPermTable[bp_ptr->r_diph]
                    [ngs->dict->dict_list[to->wid]->ci_phone_ids[0]];
            else
                bss_offset = 0;
            score =
                (ngs->bscore_stack[bp_ptr->s_idx + bss_offset] - bp_ptr->score) +
                bp_ptr->ascr;
            if (score > WORST_SCORE) {
                link_latnodes(dag, from, to, score, bp_ptr->frame);
                from->reachable = TRUE;
            }
        }
    }

    /* There must be at least one path between dag->start and dag->end */
    if (!dag->start->reachable) {
        E_ERROR("<s> isolated; unreachable\n");
        goto error_out;
    }

    /* Now change node->{fef,lef} from bptbl indices to frames. */
    for (node = dag->nodes; node; node = node->next) {
        node->fef = ngs->bp_table[node->fef].frame;
        node->lef = ngs->bp_table[node->lef].frame;
    }

    /* Change node->wid to base wid. */
    /* FIXME: Not sure we want/need to do this. */
    for (node = dag->nodes; node; node = node->next) {
        node->wid = ngs->dict->dict_list[node->wid]->wid;
    }

    /* Remove SIL and noise nodes from DAG; extend links through them */
    bypass_filler_nodes(dag);

    /* Free nodes unreachable from dag->start and their links */
    delete_unreachable(dag);

    return dag;

error_out:
    ngram_dag_free(dag);
    return NULL;
}

void
ngram_dag_free(ngram_dag_t *dag)
{
    listelem_alloc_free(dag->latnode_alloc);
    listelem_alloc_free(dag->latlink_alloc);
    listelem_alloc_free(dag->rev_latlink_alloc);
    listelem_alloc_free(dag->latpath_alloc);
    ckd_free(dag->hyp_str);
    ckd_free(dag);
}

char const *
ngram_dag_hyp(ngram_dag_t *dag, latlink_t *link)
{
    latlink_t *l;
    size_t len;
    char *c;

    /* Backtrace once to get hypothesis length. */
    l = link;
    len = 0;
    if (ISA_REAL_WORD(dag->ngs, l->to->wid))
        len += strlen(dag->ngs->dict->dict_list[l->to->wid]->word) + 1;
    while (l) {
        if (!ISA_REAL_WORD(dag->ngs, l->from->wid)) {
            l = l->best_prev;
            continue;
        }
        len += strlen(dag->ngs->dict->dict_list[l->from->wid]->word) + 1;
        l = l->best_prev;
    }

    /* Backtrace again to construct hypothesis string. */
    ckd_free(dag->hyp_str);
    dag->hyp_str = ckd_calloc(1, len);
    l = link;
    c = dag->hyp_str + len - 1;
    if (ISA_REAL_WORD(dag->ngs, l->to->wid)) {
        len = strlen(dag->ngs->dict->dict_list[l->to->wid]->word);
        c -= len;
        memcpy(c, dag->ngs->dict->dict_list[l->to->wid]->word, len);
        if (c > dag->hyp_str) {
            --c;
            *c = ' ';
        }
    }
    while (l) {
        if (!ISA_REAL_WORD(dag->ngs, l->from->wid)) {
            l = l->best_prev;
            continue;
        }
        len = strlen(dag->ngs->dict->dict_list[l->from->wid]->word);
        c -= len;
        memcpy(c, dag->ngs->dict->dict_list[l->from->wid]->word, len);
        if (c > dag->hyp_str) {
            --c;
            *c = ' ';
        }
        l = l->best_prev;
    }

    return dag->hyp_str;
}

/*
 * Lattice rescoring:  Goal: Form DAG of nodes based on unique <wid,start-fram> values,
 * and find the best path through the DAG from <<s>,0> to <</s>,last_frame>.
 * Strategy: Find the best score from <<s>,0> to end point of any link and use it to
 * update links further down the path... (reword)
 */
latlink_t *
ngram_dag_bestpath(ngram_dag_t *dag)
{
    ngram_search_t *ngs;
    latnode_t *node;
    latlink_t *link, *best;
    latlink_t *q_head, *q_tail;
    int32 score;
    float32 lwf;

    ngs = dag->ngs;
    lwf = ngs->bestpath_fwdtree_lw_ratio;

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
        assert(!(ISA_FILLER_WORD(ngs, link->to->wid)));

        link->path_scr = link->link_scr +
            ngram_bg_score(ngs->lmset, link->to->wid, ngs->start_wid, &n_used) * lwf;
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
        for (link = node->links; link; link = link->next) {
            int32 n_used;

            assert(!(ISA_FILLER_WORD(ngs, link->to->wid)));

            score = q_head->path_scr + link->link_scr +
                ngram_tg_score(ngs->lmset, link->to->wid,
                               node->wid, q_head->from->wid, &n_used) * lwf;

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
     * Rescored all paths to </s>.last_frame; now traceback optimal path.  First,
     * find the best link entering </s>.last_frame.
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

#if 0
/* Parameters to prune n-best alternatives search */
#define MAX_PATHS	500     /* Max allowed active paths at any time */
#define MAX_HYP_TRIES	10000

/* Back trace from path to root */
static search_hyp_t *
latpath_seg_back_trace(latpath_t * path)
{
    search_hyp_t *head, *h;

    head = NULL;
    for (; path; path = path->parent) {
        h = listelem_malloc(search_hyp_alloc);
        h->wid = path->node->wid;
        h->word = kb_get_word_str(h->wid);
        h->sf = path->node->sf;
        h->ef = path->node->fef;    /* Approximately */

        h->next = head;
        head = h;
    }

    return head;
}

/*
 * For each node in any path between from and end of utt, find the best score
 * from "from".sf to end of utt.  (NOTE: Uses bigram probs; this is an estimate
 * of the best score from "from".
 */
static int32
best_rem_score(latnode_t * from)
{
    latlink_t *link;
    int32 bestscore, score;

    if (from->info.rem_score <= 0)
        return (from->info.rem_score);

    /* Best score from "from" to end of utt not known; compute from successors */
    bestscore = WORST_SCORE;
    for (link = from->links; link; link = link->next) {
        int32 n_used;

        score = best_rem_score(link->to);
        score += link->link_scr;
        score += ngram_bg_score(g_lmset, link->to->wid, from->wid, &n_used) * lw_factor;
        if (score > bestscore)
            bestscore = score;
    }
    from->info.rem_score = bestscore;

    return (bestscore);
}

/*
 * Insert newpath in sorted (by path score) list of paths.  But if newpath is
 * too far down the list, drop it.
 * total_score = path score (newpath) + rem_score to end of utt.
 */
static void
path_insert(latpath_t * newpath, int32 total_score)
{
    latpath_t *prev, *p;
    int32 i;

    prev = NULL;
    for (i = 0, p = path_list; (i < MAX_PATHS) && p; p = p->next, i++) {
        if ((p->score + p->node->info.rem_score) < total_score)
            break;
        prev = p;
    }

    /* newpath should be inserted between prev and p */
    if (i < MAX_PATHS) {
        /* Insert new partial hyp */
        newpath->next = p;
        if (!prev)
            path_list = newpath;
        else
            prev->next = newpath;
        if (!p)
            path_tail = newpath;

        n_path++;
        n_hyp_insert++;
        insert_depth += i;
    }
    else {
        /* newpath score too low; reject it and also prune paths beyond MAX_PATHS */
        path_tail = prev;
        prev->next = NULL;
        n_path = MAX_PATHS;
        listelem_free(latpath_alloc, newpath);

        n_hyp_reject++;
        for (; p; p = newpath) {
            newpath = p->next;
            listelem_free(latpath_alloc, p);
            n_hyp_reject++;
        }
    }
}

/* Find all possible extensions to given partial path */
static void
path_extend(latpath_t * path)
{
    latlink_t *link;
    latpath_t *newpath;
    int32 total_score, tail_score;

    /* Consider all successors of path->node */
    for (link = path->node->links; link; link = link->next) {
        int32 n_used;

        /* Skip successor if no path from it reaches the final node */
        if (link->to->info.rem_score <= WORST_SCORE)
            continue;

        /* Create path extension and compute exact score for this extension */
        newpath = listelem_malloc(latpath_alloc);
        newpath->node = link->to;
        newpath->parent = path;
        newpath->score = path->score + link->link_scr;
        if (path->parent)
            newpath->score += lw_factor
                * ngram_tg_score(g_lmset, newpath->node->wid,
                                 path->node->wid,
                                 path->parent->node->wid, &n_used);
        else 
            newpath->score += lw_factor
                * ngram_bg_score(g_lmset, newpath->node->wid,
                                 path->node->wid, &n_used);

        /* Insert new partial path hypothesis into sorted path_list */
        n_hyp_tried++;
        total_score = newpath->score + newpath->node->info.rem_score;

        /* First see if hyp would be worse than the worst */
        if (n_path >= MAX_PATHS) {
            tail_score =
                path_tail->score + path_tail->node->info.rem_score;
            if (total_score < tail_score) {
                listelem_free(latpath_alloc, newpath);
                n_hyp_reject++;
                continue;
            }
        }

        path_insert(newpath, total_score);
    }
}

void
search_hyp_free(search_hyp_t * h)
{
    search_hyp_t *tmp;

    while (h) {
        tmp = h->next;
        listelem_free(search_hyp_alloc, h);
        h = tmp;
    }
}

static int32
hyp_diff(search_hyp_t * hyp1, search_hyp_t * hyp2)
{
    search_hyp_t *h1, *h2;

    for (h1 = hyp1, h2 = hyp2;
         h1 && h2 && (h1->wid == h2->wid); h1 = h1->next, h2 = h2->next);
    return (h1 || h2) ? 1 : 0;
}

/*
 * Get n best alternatives in lattice for the given frame range [sf..ef].
 * w1 and w2 provide left context; w1 may be -1.
 * NOTE: Clobbers any previously returned hypotheses!!
 * Return values: no. of alternative hypotheses returned in alt; -1 if error.
 */
int32
search_get_alt(int32 n,         /* In: No. of alternatives to look for */
               int32 sf, int32 ef,      /* In: Start/End frame */
               int32 w1, int32 w2,      /* In: context words */
               search_hyp_t *** alt_out)
{                               /* Out: array of alternatives */
    latnode_t *node;
    latpath_t *path, *top;
    int32 i, j, scr, n_alt;
    static search_hyp_t **alt = NULL;
    static int32 max_alt_hyp = 0;

    if (n <= 0)
        return -1;

    for (i = 0; i < max_alt_hyp; i++) {
        search_hyp_free(alt[i]);
        alt[i] = NULL;
    }

    if (n > max_alt_hyp) {
        if (alt)
            free(alt);
        max_alt_hyp = (n + 255) & 0xffffff00;
        alt = ckd_calloc(max_alt_hyp, sizeof(search_hyp_t *));
    }

    *alt_out = NULL;

    /* If no permanent lattice available to be searched, return */
    if (lattice.latnode_list == NULL) {
        /* MessageBoxX("permanent lattice trouble in searchlat.c"); */
        E_ERROR("NULL lattice\n");
        return 0;
    }

    /* Initialize rem_score to default values */
    for (node = lattice.latnode_list; node; node = node->next) {
        if (node == lattice.final_node)
            node->info.rem_score = 0;
        else if (!node->links)
            node->info.rem_score = WORST_SCORE;
        else
            node->info.rem_score = 1;   /* +ve => unknown value */
    }

    /* Create initial partial hypotheses list consisting of nodes starting at sf */
    n_path = 0;
    n_hyp_reject = 0;
    n_hyp_tried = 0;
    n_hyp_insert = 0;
    insert_depth = 0;
    paths_done = NULL;

    path_list = path_tail = NULL;
    for (node = lattice.latnode_list; node; node = node->next) {
        if (node->sf == sf) {
            int32 n_used;
            best_rem_score(node);

            path = listelem_malloc(latpath_alloc);
            path->node = node;
            path->parent = NULL;
            scr =
                (w1 < 0)
                ? ngram_bg_score(g_lmset, node->wid, w2, &n_used)
                : ngram_tg_score(g_lmset, node->wid, w2, w1, &n_used);
            path->score = scr;

            path_insert(path, scr + node->info.rem_score);
        }
    }

    /* Find n hypotheses */
    for (n_alt = 0;
         path_list && (n_alt < n) && (n_hyp_tried < MAX_HYP_TRIES);) {
        /* Pop the top (best) partial hypothesis */
        top = path_list;
        path_list = path_list->next;
        if (top == path_tail)
            path_tail = NULL;
        n_path--;

        if ((top->node->sf >= ef) || ((top->node == lattice.final_node) &&
                                      (ef > lattice.final_node->sf))) {
            /* Complete hypothesis; generate output, but omit last (bracketing) node */
            alt[n_alt] = latpath_seg_back_trace(top->parent);
            /* alt[n_alt] = latpath_seg_back_trace (top); */

            /* Accept hyp only if non-empty */
            if (alt[n_alt]) {
                /* Check if hypothesis already output */
                for (j = 0; (j < n_alt) && (hyp_diff(alt[j], alt[n_alt]));
                     j++);
                if (j >= n_alt) /* New, distinct alternative hypothesis */
                    n_alt++;
                else {
                    search_hyp_free(alt[n_alt]);
                    alt[n_alt] = NULL;
                }
            }
#if 0
            /* Print path here for debugging */
#endif
        }
        else {
            if (top->node->fef < ef)
                path_extend(top);
        }

        /*
         * Add top to paths already processed; cannot be freed because other paths
         * point to it.
         */
        top->next = paths_done;
        paths_done = top;
    }

#if defined(SEARCH_PROFILE)
    E_INFO
        ("#HYP = %d, #INSERT = %d, #REJECT = %d, Avg insert depth = %.2f\n",
         n_hyp_tried, n_hyp_insert, n_hyp_reject,
         insert_depth / ((double) n_hyp_insert + 1.0));
#endif

    /* Clean up */
    while (path_list) {
        top = path_list;
        path_list = path_list->next;
        listelem_free(latpath_alloc, top);
    }
    while (paths_done) {
        top = paths_done;
        paths_done = paths_done->next;
        listelem_free(latpath_alloc, top);
    }

    *alt_out = alt;

    return n_alt;
}
#endif
