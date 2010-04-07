/* -*- c-basic-offset:4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2010 Carnegie Mellon University.  All rights
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
 * @file glextree.c
 * @brief Generic lexicon trees for token-fsg search (and maybe elsewhere)
 * @author David Huggins-Daines <dhuggins@cs.cmu.edu>
 */

#include "glextree.h"

glexroot_t *
glexroot_add_child(glextree_t *tree, glexroot_t *root, int32 phone)
{
    glexroot_t *newroot;

    newroot = listelem_malloc(tree->root_alloc);
    newroot->phone = phone;
    newroot->n_down = 0;
    newroot->down.kids = NULL;
    newroot->sibs = root->down.kids;
    root->down.kids = newroot;
    ++root->n_down;
    ++tree->nlexroot;
    return newroot;
}

glexroot_t *
glexroot_find_child(glexroot_t *root, int32 phone)
{
    glexroot_t *start = root->down.kids;
    while (start) {
        if (start->phone == phone)
            return start;
        start = start->sibs;
    }
    return NULL;
}

glexroot_t *
glextree_find_roots(glextree_t *tree, int lc, int ci)
{
    glexroot_t *lcroot;

    if ((lcroot = glexroot_find_child(&tree->root, lc)) == NULL)
        return NULL;
    return glexroot_find_child(lcroot, ci);
}

static glexnode_t *
glextree_add_node(glextree_t *tree, int pid, int nhmm)
{
    glexnode_t *node = listelem_malloc(tree->node_alloc);
    node->hmms = ckd_calloc(nhmm, sizeof(*node->hmms));
    node->kids = NULL;
    node->wid = -1;
    tree->nlexhmm += nhmm;
    ++tree->nlexnode;
    return node;
}

static void
glextree_add_single_phone_word(glextree_t *tree, glexroot_t *ciroot, int lc, int32 w)
{
    int ci, rc;

    ci = dict_first_phone(tree->dict, w);
    for (rc = 0; rc < bin_mdef_n_ciphone(tree->d2p->mdef); ++rc) {
        s3ssid_t ssid = dict2pid_lrdiph_rc(tree->d2p, ci, lc, rc);
        glexroot_t *rcroot;
        glexnode_t *node;
        glexlink_t *link;

        if (ssid == BAD_S3SSID)
            continue;
        /* Find or create the root node for this triphone. */
        if ((rcroot = glexroot_find_child(ciroot, rc)) == NULL)
            rcroot = glexroot_add_child(tree, ciroot, rc);
        /* Create a root-leaf node (FIXME: this is kind of inefficient
         * to have one for each right context - would be better to
         * allocate a single one under ciroot somehow). */
        node = glextree_add_node(tree, ci, 1);
        hmm_init(tree->ctx, node->hmms, FALSE, ssid, ci);
        node->wid = w;
        /* Link it in. */
        link = listelem_malloc(tree->link_alloc);
        link->dest = node;
        link->next = rcroot->down.nodes;
        rcroot->down.nodes = link;
    }
}

static glexnode_t *
internal_node(glextree_t *tree, glexlink_t **cur, s3ssid_t ssid, int ci)
{
    glexlink_t *link;

    /* Look for a node with the same ssid which is not a leaf node. */
    /* E_INFO("Looking for ssid %d in link %p -> %p\n", ssid, cur, *cur); */
    for (link = *cur; link; link = link->next)
        if (hmm_nonmpx_ssid(link->dest->hmms) == ssid
            && link->dest->wid == -1)
            break;
    /* Create it if not... */
    if (link == NULL) {
        glexnode_t *node;

        node = glextree_add_node(tree, ci, 1);
        hmm_init(tree->ctx, node->hmms, FALSE, ssid, ci);
        link = listelem_malloc(tree->link_alloc);
        link->dest = node;
        link->next = *cur;
        /* E_INFO("Created link %p/%p\n", node, *cur); */
        *cur = link;
    }

    return link->dest;
}

/**
 * Find a first non-root (i.e. second phone) node corresponding to ssid.
 */
static glexnode_t *
find_first_internal_node(glextree_t *tree, s3ssid_t ssid)
{
    void *val;

    if (hash_table_lookup_bkey(tree->internal, (char const *)&ssid,
                               sizeof(ssid), &val) < 0)
        return NULL;

    return (glexnode_t *)val;
}

static void
build_subtree_over_2phones(glextree_t *tree, glexlink_t **cur, int32 w)
{
    glexnode_t *node;
    glexlink_t *link;
    xwdssid_t *rssid;
    s3ssid_t ssid;
    int pos = 1;

    ssid = dict2pid_internal(tree->d2p, w, 1);
    /* Try to find an existing first internal node. */
    if ((node = find_first_internal_node(tree, ssid)) != NULL) {
        /* E_INFO("Found internal node %p for ssid %d\n", node, ssid); */
        /* Link it into cur, which is the root node's successors (fan-in). */
        link = listelem_malloc(tree->link_alloc);
        link->dest = node;
        link->next = *cur;
        *cur = link;
        /* Now move down a level. */
        cur = &node->kids;
    }
    else {
        /* Create a new node under cur and move down a level. */
        node = internal_node(tree, cur, ssid, dict_pron(tree->dict, w, pos));
        /* E_INFO("Created internal node %p for ssid %d\n", node, ssid); */
        cur = &node->kids;
        /* Enter node in the internal hash table.  NOTE:
         * SphinxBase hash tables are a real pain and require
         * their keys to be externally allocated, so we
         * pre-allocate all ssids in a big array and index into
         * that. */
        hash_table_enter_bkey(tree->internal,
                              (char const *)(tree->ssidbuf + ssid),
                              sizeof(ssid), node);
    }
    ++pos;
    /* Now walk down to the penultimate phone, building new nodes as
     * necessary. */
    while (pos < dict_pronlen(tree->dict, w) - 1) {
        ssid = dict2pid_internal(tree->d2p, w, pos);
        node = internal_node(tree, cur, ssid, dict_pron(tree->dict, w, pos));
        cur = &node->kids;
        ++pos;
    }

    /* Now node is the penultimate phone and cur points to its
     * children.  There is probably an existing leaf node. */
    rssid = dict2pid_rssid(tree->d2p,
                           dict_last_phone(tree->dict, w),
                           dict_second_last_phone(tree->dict, w));
    for (link = *cur; link; link = link->next)
        if (link->dest->wid == w)
            break;
    if (link == NULL) {
        int rc;
        E_INFO("Allocating leaf node for %s - %d HMMs\n",
               dict_wordstr(tree->dict, w), rssid->n_ssid);
        node = glextree_add_node(tree,
                                 dict_last_phone(tree->dict, w),
                                 rssid->n_ssid);
        for (rc = 0; rc < rssid->n_ssid; ++rc) {
            hmm_init(tree->ctx, node->hmms + rc, FALSE, rssid->ssid[rc],
                     dict_last_phone(tree->dict, w));
        }
        node->wid = w;
        link = listelem_malloc(tree->link_alloc);
        link->dest = node;
        link->next = *cur;
        *cur = link;
    }
}

static void
build_subtree_2phones(glextree_t *tree, glexlink_t **cur, int32 w)
{
    xwdssid_t *rssid;
    glexnode_t *node;
    glexlink_t *link;
    void *val;
    int rc;

    /* Every two-phone word gets a unique leaf node, for obvious
       reasons.  However, at least for triphones, there should be only
       one of them.  So look it up. */
    if (hash_table_lookup_bkey(tree->internal_leaf, (char const *)&w,
                               sizeof(w), &val) == 0) {
        /* Found one, link it into the root node (fan-in) */
        link = listelem_malloc(tree->link_alloc);
        link->dest = val;
        link->next = *cur;
        *cur = link;
        /* Nothing else to do. */
        /* E_INFO("Reusing leaf node for %s\n", dict_wordstr(tree->dict, w)); */
        return;
    }

    /* Create a new one. */
    rssid = dict2pid_rssid(tree->d2p,
                           dict_last_phone(tree->dict, w),
                           dict_second_last_phone(tree->dict, w));
    E_INFO("Allocating leaf node for %s - %d HMMs\n",
           dict_wordstr(tree->dict, w), rssid->n_ssid);
    node = glextree_add_node(tree,
                             dict_last_phone(tree->dict, w),
                             rssid->n_ssid);
    /* Enter it into the hash table and associated list. */
    tree->internal_leaf_wids = glist_add_int32(tree->internal_leaf_wids, w);
    hash_table_enter_bkey(tree->internal_leaf,
                          (char const *)&gnode_int32(tree->internal_leaf_wids),
                          sizeof(int32), node);
    for (rc = 0; rc < rssid->n_ssid; ++rc) {
        hmm_init(tree->ctx, node->hmms + rc, FALSE, rssid->ssid[rc],
                 dict_last_phone(tree->dict, w));
    }
    node->wid = w;
    link = listelem_malloc(tree->link_alloc);
    link->dest = node;
    link->next = *cur;
    *cur = link;
}

static void
glextree_add_multi_phone_word(glextree_t *tree, glexroot_t *ciroot, int lc, int32 w)
{
    glexroot_t *rcroot;
    glexnode_t *node;
    s3ssid_t ssid;
    int rc;

    /* Find leaf corresponding to root node. */
    rc = dict_second_phone(tree->dict, w);
    if ((rcroot = glexroot_find_child(ciroot, rc)) == NULL)
        rcroot = glexroot_add_child(tree, ciroot, rc);

    /* Find or create a root node for the initial triphone. */
    ssid = dict2pid_ldiph_lc(tree->d2p, ciroot->phone, rc, lc);
    assert(ssid != BAD_S3SSID); /* Should never happen (which is a problem). */
    /* This is the first phone's node.  It may have siblings which are
     * single-phone words (but should have none for multi-phones). */
    node = internal_node(tree, &rcroot->down.nodes, ssid, ciroot->phone);

    /* For triphones we just need to treat 2-phone words separately. */
    if (dict_pronlen(tree->dict, w) == 2)
        build_subtree_2phones(tree, &node->kids, w);
    else
        build_subtree_over_2phones(tree, &node->kids, w);
}

void
glextree_add_word(glextree_t *tree, int32 w)
{
    glexroot_t *lcroot;

    /* Find or create root nodes for this word for all left contexts. */
    for (lcroot = tree->root.down.kids; lcroot; lcroot = lcroot->sibs) {
        glexroot_t *ciroot;

        /* Is this a possible lc-ci combination?  FIXME: Actually
         * there's no way to find out! */
        if ((ciroot = glexroot_find_child
             (lcroot, dict_first_phone(tree->dict, w))) == NULL) {
            ciroot = glexroot_add_child(tree, lcroot,
                                        dict_first_phone(tree->dict, w));
        }
        /* If it is a single phone word, then create root-leaf nodes
         * for every possible right context. */
        if (dict_pronlen(tree->dict, w) == 1) {
            glextree_add_single_phone_word(tree, ciroot, lcroot->phone, w);
        }
        /* Otherwise, find or create a root node and add it to the tree. */
        else {
            glextree_add_multi_phone_word(tree, ciroot, lcroot->phone, w);
        }
    }
    E_INFO("Added %s - %d lex roots, %d nodes, %d hmms\n",
           dict_wordstr(tree->dict, w), tree->nlexroot, tree->nlexnode, tree->nlexhmm);
}

glextree_t *
glextree_build(hmm_context_t *ctx, dict_t *dict, dict2pid_t *d2p,
               glextree_wfilter_t filter, void *udata)
{
    glextree_t *tree;
    glexroot_t *lcroot;
    int32 w, p;

    tree = ckd_calloc(1, sizeof(*tree));
    tree->refcount = 1;
    tree->ctx = ctx;
    tree->dict = dict_retain(dict);
    tree->d2p = dict2pid_retain(d2p);
    tree->root_alloc = listelem_alloc_init(sizeof(glexroot_t));
    tree->node_alloc = listelem_alloc_init(sizeof(glexnode_t));
    tree->link_alloc = listelem_alloc_init(sizeof(glexlink_t));

    tree->internal = hash_table_new(bin_mdef_n_sseq(d2p->mdef), HASH_CASE_YES);
    tree->internal_leaf = hash_table_new(dict_size(tree->dict) / 10, HASH_CASE_YES);
    /* Externally allocate ssid keys for the above hashtable. */
    tree->ssidbuf = ckd_calloc(sizeof(s3ssid_t), bin_mdef_n_sseq(d2p->mdef));
    for (p = 0; p < bin_mdef_n_sseq(d2p->mdef); ++p)
        tree->ssidbuf[p] = p;

    E_INFO("Building lexicon tree:\n");
    /* First, build a root node for every possible left-context phone.
     * These can't be easily added at runtime, and it doesn't cost
     * very much to cover them all just in case (FIXME: actually it
     * might but we will worry about that later...) */
    for (p = 0; p < bin_mdef_n_ciphone(d2p->mdef); ++p) {
        glexroot_add_child(tree, &tree->root, p);
    }
    E_INFO("\t%d left context roots\n", bin_mdef_n_ciphone(d2p->mdef));
    /* Now build a tree of first and second phones under the newly allocated roots. */
    for (w = 0; w < dict_size(dict); ++w) {
        if (filter && !(*filter)(tree, w, udata))
            continue;
        glextree_add_word(tree, w);
    }
    /* Dump some tree statistics. */
    for (lcroot = tree->root.down.kids; lcroot; lcroot = lcroot->sibs) {
        glexroot_t *ciroot;
        int n_ci = 0, n_rc = 0;
        for (ciroot = lcroot->down.kids; ciroot; ciroot = ciroot->sibs) {
            glexroot_t *rcroot;
            ++n_ci;
            for (rcroot = ciroot->down.kids; rcroot; rcroot = rcroot->sibs) {
                ++n_rc;
            }
        }
        E_INFO("         %s has %d ci roots, %d rc roots\n",
               bin_mdef_ciphone_str(d2p->mdef, lcroot->phone), n_ci, n_rc);
    }
    E_INFO("Tree has %d lex roots (%d KiB)\n",
           tree->nlexroot, tree->nlexroot * sizeof(glexroot_t) / 1024);
    E_INFO("         %d nodes (%d KiB), %d HMMs (%d KiB)\n",
           tree->nlexnode, tree->nlexnode * sizeof(glexnode_t) / 1024,
           tree->nlexhmm, tree->nlexhmm * sizeof(hmm_t) / 1024);

    return tree;
}

int
glextree_has_word(glextree_t *tree, int32 wid)
{
    glexroot_t *ciroot;
    glexroot_t *rcroot;

    /* Take advantage of the fact that we expand all left contexts. */
    if ((ciroot = glextree_find_roots(tree, 0,
                                      dict_first_phone(tree->dict, wid))) == NULL)
        return FALSE;
    if (dict_pronlen(tree->dict, wid) == 1) {
        glexlink_t *link;
        /* Take advantage of the fact that we expand all right
         * contexts for single-phone words. */
        rcroot = ciroot->down.kids;
        for (link = rcroot->down.nodes; link; link = link->next)
            if (link->dest->wid == wid)
                return TRUE;
        return FALSE;
    }
    else {
        glexnode_t *node;
        glexlink_t *link;
        int pos;

        /* Find the lextree root. */
        if ((rcroot = glexroot_find_child
             (ciroot,dict_second_phone(tree->dict, wid))) == NULL)
            return FALSE;
        /* There will be only one node that isn't a single-phone word,
         * so take it. */
        for (link = rcroot->down.nodes; link; link = link->next)
            if (link->dest->wid == -1)
                break;
        if (link == NULL)
            return FALSE;
        /* Now do second through penultimate phones. */
        node = link->dest;
        for (pos = 1; pos < dict_pronlen(tree->dict, wid) - 1; ++pos) {
            for (link = node->kids; link; link = link->next) {
                if (hmm_nonmpx_ssid(link->dest->hmms)
                    == dict2pid_internal(tree->d2p, wid, pos)
                    && link->dest->wid == -1)
                    break;
            }
            if (link == NULL)
                return FALSE;
            node = link->dest;
        }
        /* Leaf node will have wid attached to it. */
        for (link = node->kids; link; link = link->next)
            if (link->dest->wid == wid)
                return TRUE;
        return FALSE;
    }
}

glextree_t *
glextree_retain(glextree_t *tree)
{
    if (tree == NULL)
        return NULL;
    ++tree->refcount;
    return tree;
}

int
glextree_free(glextree_t *tree)
{
    if (tree == NULL)
        return 0;
    if (--tree->refcount > 0)
        return tree->refcount;
    listelem_alloc_free(tree->root_alloc);
    listelem_alloc_free(tree->node_alloc);
    listelem_alloc_free(tree->link_alloc);
    ckd_free(tree->ssidbuf);
    hash_table_free(tree->internal);
    hash_table_free(tree->internal_leaf);
    glist_free(tree->internal_leaf_wids);
    ckd_free(tree->d2p);
    ckd_free(tree->dict);
    ckd_free(tree);
    return 0;
}
