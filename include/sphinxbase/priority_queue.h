#ifndef __PRIORITY_QUEUE_H__
#define __PRIORITY_QUEUE_H__

#include <stdlib.h>
#include <sphinxbase/sphinxbase_export.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0
/* Fool Emacs. */
}
#endif

/**
 * Priority queue for max element tracking.
 * The one expects heap here, but for current application
 * (sorting of ngram entries one per order, i.e. maximum 10)
 * i'll put just and array here, so each operation takes linear time.
 * I swear to rework it some day!
 * TODOTODOTODOTODOTODOTODOTODOTODOTODOTODOTODOTODOTODOTODO!!!!!
 */

typedef struct priority_queue_s priority_queue_t;

SPHINXBASE_EXPORT
priority_queue_t* priority_queue_create(size_t len, int (*compare)(const void *a, const void *b));

SPHINXBASE_EXPORT
void* priority_queue_poll(priority_queue_t *queue);

SPHINXBASE_EXPORT
void priority_queue_add(priority_queue_t *queue, void *element);

SPHINXBASE_EXPORT
size_t priority_queue_size(priority_queue_t *queue);

SPHINXBASE_EXPORT
void priority_queue_free(priority_queue_t *queue, void (*free_ptr)(void *a));

#ifdef __cplusplus
}
#endif

#endif /* __PRIORITY_QUEUE_H__ */