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

static int nlexroot;

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

    ++nlexroot;
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

glexnode_t *
glextree_add_node(glextree_t *tree, s3ssid_t ssid, int pid)
{
    glexnode_t *node = listelem_malloc(tree->node_alloc);
    hmm_init(tree->ctx, &node->hmm, FALSE, ssid, pid);
    node->sibs = node->kids = NULL;
    node->rc = -1;
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

        if (ssid == BAD_S3SSID)
            continue;
        /* Find or create the root node for this triphone. */
        if ((rcroot = glexroot_find_child(ciroot, rc)) == NULL)
            rcroot = glexroot_add_child(tree, ciroot, rc);
        /* Find or create a root-leaf node. */
        for (node = rcroot->down.node; node; node = node->sibs)
            if (node->kids == NULL)
                break;
        if (node == NULL) {
            node = glextree_add_node(tree, ssid, ci);
            node->sibs = rcroot->down.node;
            node->rc = rc;
            rcroot->down.node = node;
        }
    }
}

static glexnode_t *
internal_node(glextree_t *tree, glexnode_t **cur, s3ssid_t ssid, int ci)
{
    glexnode_t *node;

    /* Look for a node with the same ssid which is not a leaf node. */
    for (node = *cur; node; node = node->sibs)
        if (hmm_nonmpx_ssid(&node->hmm) == ssid && node->rc == -1)
            break;
    /* Create it if not... */
    if (node == NULL) {
        node = glextree_add_node(tree, ssid, ci);
        node->sibs = *cur;
        node->rc = -1;
        *cur = node;
    }

    return node;
}

/**
 * Find a first non-root (i.e. second phone) node corresponding to ssid.
 */
static glexnode_t *
find_first_internal_node(glextree_t *tree, s3ssid_t ssid)
{
    glexroot_t *lcroot;

    /* FIXME: Obviously, this is very slow. */
    for (lcroot = tree->root.down.kids; lcroot; lcroot = lcroot->sibs) {
        glexroot_t *ciroot;
        for (ciroot = lcroot->down.kids; ciroot; ciroot = ciroot->sibs) {
            glexroot_t *rcroot;
            for (rcroot = ciroot->down.kids; rcroot; rcroot = rcroot->sibs) {
                glexnode_t *node;
                for (node = rcroot->down.node->kids; node; node = node->sibs) {
                    if (hmm_nonmpx_ssid(&node->hmm) == ssid)
                        return node;
                }
            }
        }
    }
    return NULL;
}

static void
glextree_add_multi_phone_word(glextree_t *tree, glexroot_t *ciroot, int lc, int32 w)
{
    glexroot_t *rcroot;
    glexnode_t **cur, *node;
    xwdssid_t *rssid;
    s3ssid_t ssid;
    int pos, rc;

    /* Find leaf corresponding to root node. */
    rc = dict_second_phone(tree->dict, w);
    if ((rcroot = glexroot_find_child (ciroot, rc)) == NULL)
        rcroot = glexroot_add_child(tree, ciroot, rc);

    /* Find or create a root node for the initial triphone. */
    ssid = dict2pid_ldiph_lc(tree->d2p, ciroot->phone, rc, lc);
    assert(ssid != BAD_S3SSID); /* Should never happen. */
    /* This is essentially the same as the loop below except for how
     * we got ssid. */
    cur = &rcroot->down.node;
    node = internal_node(tree, cur, ssid, ciroot->phone);

    /* Internal phones have no left context dependency, so we will
     * "pinch" all the trees together at this point - we look for an
     * existing internal node corresponding to the next phone's SSID.
     * This works a bit differently for >2 phone words as it does for
     * 2-phone words as in the latter case we need to hook into a list
     * of leafnodes.
     */
    cur = &node->kids; /* Default - create new node. */
    if (dict_pronlen(tree->dict, w) > 2) {
        glexnode_t *node2;
        ssid = dict2pid_internal(tree->d2p, w, 1);
        if ((node2 = find_first_internal_node(tree, ssid)) != NULL)
            cur = &node2->kids; /* Found an existing one. */
        else {
            /* Create a new node under cur and move down a level. */
            node2 = internal_node(tree, cur, ssid, dict_pron(tree->dict, w, pos));
            cur = &node->kids;
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
    }

    /* Now node is the penultimate phone and cur points to its
     * children.  We will now create leaf nodes for all contexts.
     * This looks quite a lot like glextree_add_single_phone_word()
     * except that we have a compressed right context list. */
    rssid = dict2pid_rssid(tree->d2p,
                           dict_last_phone(tree->dict, w),
                           dict_second_last_phone(tree->dict, w));
    for (rc = 0; rc < rssid->n_ssid; ++rc) {
        ssid = rssid->ssid[rc];
        for (node = *cur; node; node = node->sibs)
            if (hmm_nonmpx_ssid(&node->hmm) == ssid && node->rc != -1)
                break;
        if (node == NULL) {
            node = glextree_add_node(tree, ssid,
                                     dict_last_phone(tree->dict, w));
            node->sibs = *cur;
            node->rc = rc;
            *cur = node;
        }
    }
}

void
glextree_add_word(glextree_t *tree, int32 w)
{
    glexroot_t *lcroot;

    E_INFO("Adding %s - %d lex roots\n", dict_wordstr(tree->dict, w), nlexroot);
    /* Find or create root nodes for this word for all left contexts. */
    for (lcroot = tree->root.down.kids; lcroot; lcroot = lcroot->sibs) {
        glexroot_t *ciroot;

        /* Is this a possible lc-ci combination?  FIXME: Actually
         * there's no way to find out! */
        if ((ciroot = glexroot_find_child
             (lcroot, dict_first_phone(tree->dict, w))) == NULL) {
            ciroot = glexroot_add_child(tree, lcroot,
                                        dict_first_phone(tree->dict, w));
            E_INFO("Created ciroot (%s,%s)\n",
                   bin_mdef_ciphone_str(tree->d2p->mdef, lcroot->phone),
                   bin_mdef_ciphone_str(tree->d2p->mdef, ciroot->phone));
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
}

glextree_t *
glextree_build(hmm_context_t *ctx, dict_t *dict, dict2pid_t *d2p,
               glextree_wfilter_t filter, void *udata)
{
    glextree_t *tree;
    int32 w, p;

    tree = ckd_calloc(1, sizeof(*tree));
    tree->refcount = 1;
    tree->ctx = ctx;
    tree->dict = dict_retain(dict);
    tree->d2p = dict2pid_retain(d2p);
    tree->root_alloc = listelem_alloc_init(sizeof(glexroot_t));
    tree->node_alloc = listelem_alloc_init(sizeof(glexnode_t));

    E_INFO("Building lexicon tree:\n");
    /* First, build a root node for every possible left-context phone.
     * These can't be easily added at runtime, and it doesn't cost
     * very much to cover them all just in case (FIXME: actually it
     * might but we will worry about that later...) */
    for (p = 0; p < bin_mdef_n_ciphone(d2p->mdef); ++p) {
        E_INFO("Adding left context root for %s\n", bin_mdef_ciphone_str(d2p->mdef, p));
        glexroot_add_child(tree, &tree->root, p);
    }
    /* Now build a tree of first and second phones under the newly allocated roots. */
    for (w = 0; w < dict_size(dict); ++w) {
        if (filter && !(*filter)(tree, w, udata))
            continue;
        glextree_add_word(tree, w);
    }

    return tree;
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
    ckd_free(tree->d2p);
    ckd_free(tree->dict);
    ckd_free(tree);
    return 0;
}
