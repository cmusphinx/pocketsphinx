/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2015 Carnegie Mellon University.  All rights
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pocketsphinx/err.h>

#include "util/priority_queue.h"
#include "util/ckd_alloc.h"

struct priority_queue_s {
    void **pointers;
    size_t alloc_size;
    size_t size;
    void *max_element;
    int (*compare)(const void *a, const void *b);
};

priority_queue_t* priority_queue_create(size_t len, int (*compare)(const void *a, const void *b))
{
    priority_queue_t* queue;

    queue = (priority_queue_t *)ckd_calloc(1, sizeof(*queue));
    queue->alloc_size = len;
    queue->pointers = (void **)ckd_calloc(len, sizeof(*queue->pointers));
    queue->size = 0;
    queue->max_element = NULL;
    queue->compare = compare;

    return queue;
}

void* priority_queue_poll(priority_queue_t *queue)
{
    
    size_t i;
    void *res;

    if (queue->size == 0) {
        E_WARN("Trying to poll from empty queue\n");
        return NULL;
    }
    if (queue->max_element == NULL) {
        E_ERROR("Trying to poll from queue and max element is undefined\n");
        return NULL;
    }
    res = queue->max_element;
    for (i = 0; i < queue->alloc_size; i++) {
        if (queue->pointers[i] == queue->max_element) {
            queue->pointers[i] = NULL;
            break;
        }
    }
    queue->max_element = NULL;
    for (i = 0; i < queue->alloc_size; i++) {
        if (queue->pointers[i] == 0)
            continue;
        if (queue->max_element == NULL) {
            queue->max_element = queue->pointers[i];
        } else {
            if (queue->compare(queue->pointers[i], queue->max_element) < 0)
                queue->max_element = queue->pointers[i];
        }
    }
    queue->size--;
    return res;
}

void priority_queue_add(priority_queue_t *queue, void *element)
{
    size_t i;
    if (queue->size == queue->alloc_size) {
        E_ERROR("Trying to add element into full queue\n");
        return;
    }
    for (i = 0; i < queue->alloc_size; i++) {
        if (queue->pointers[i] == NULL) {
            queue->pointers[i] = element;
            break;
        }
    }

    if (queue->max_element == NULL || queue->compare(element, queue->max_element) < 0) {
        queue->max_element = element;
    }
    queue->size++;
}

size_t priority_queue_size(priority_queue_t *queue)
{
    return queue->size;
}

void priority_queue_free(priority_queue_t *queue, void (*free_ptr)(void *a))
{
    size_t i;

    for (i = 0; i < queue->alloc_size; i++) {
        if (queue->pointers[i] != NULL) {
            if (free_ptr == NULL) {
                ckd_free(queue->pointers[i]);
            } else {
                free_ptr(queue->pointers[i]);
            }
        }
    }
    ckd_free(queue->pointers);
    ckd_free(queue);
}
