/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/**
 * @file test_genrand_thread.c
 * @brief Test current genrand behavior with threads (expected to show issues)
 *
 * This test demonstrates the thread safety issues with the current
 * global RNG implementation. It's expected to show race conditions.
 */

#include "util/genrand.h"
#include "test_thread_utils.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define NUM_THREADS 4
#define VALUES_PER_THREAD 1000

/* Storage for each thread's generated values */
static long thread_values[NUM_THREADS][VALUES_PER_THREAD];
static int collision_count = 0;
static mutex_t collision_mutex;

#ifdef _WIN32
static unsigned __stdcall thread_gen_func(void *arg)
#else
static void *thread_gen_func(void *arg)
#endif
{
    int thread_id = *(int *)arg;
    int i;

    /* Each thread uses a different seed */
    genrand_seed(thread_id * 1000);

    /* Generate values */
    for (i = 0; i < VALUES_PER_THREAD; i++) {
        thread_values[thread_id][i] = genrand_int31();
    }

#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

/* Check if sequences from different threads overlap (they shouldn't) */
static void check_for_collisions(void)
{
    int t1, t2, i, j;
    int local_collisions = 0;

    for (t1 = 0; t1 < NUM_THREADS; t1++) {
        for (t2 = t1 + 1; t2 < NUM_THREADS; t2++) {
            for (i = 0; i < VALUES_PER_THREAD; i++) {
                for (j = 0; j < VALUES_PER_THREAD; j++) {
                    if (thread_values[t1][i] == thread_values[t2][j]) {
                        local_collisions++;
                    }
                }
            }
        }
    }

    mutex_lock(&collision_mutex);
    collision_count += local_collisions;
    mutex_unlock(&collision_mutex);
}

int main(int argc, char *argv[])
{
    thread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    int i;

    (void)argc;
    (void)argv;

    printf("Testing genrand with %d threads (baseline behavior)...\n", NUM_THREADS);
    printf("NOTE: This test may show thread safety issues with global state.\n\n");

    mutex_init(&collision_mutex);

    /* Create threads */
    for (i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        thread_create(&threads[i], thread_gen_func, &thread_ids[i]);
    }

    /* Wait for completion */
    for (i = 0; i < NUM_THREADS; i++) {
        thread_join(threads[i]);
    }

    /* Check for unexpected collisions */
    check_for_collisions();

    printf("Results:\n");
    printf("- Generated %d values per thread\n", VALUES_PER_THREAD);
    printf("- Total values: %d\n", NUM_THREADS * VALUES_PER_THREAD);
    printf("- Cross-thread collisions: %d\n", collision_count);

    if (collision_count > 0) {
        printf("\nWARNING: Found %d collisions between threads!\n", collision_count);
        printf("This indicates thread interference in the global RNG state.\n");
        printf("This is expected with the current global implementation.\n");
    }

    /* Also test that each thread's sequence is internally consistent */
    printf("\nChecking internal sequence consistency...\n");
    for (i = 0; i < NUM_THREADS; i++) {
        int j;
        int duplicates = 0;

        /* Check for duplicates within a thread's sequence */
        for (j = 1; j < VALUES_PER_THREAD; j++) {
            if (thread_values[i][j] == thread_values[i][j-1]) {
                duplicates++;
            }
        }

        printf("Thread %d: %d consecutive duplicates\n", i, duplicates);
        if (duplicates > 10) {
            printf("  WARNING: High duplicate count suggests corruption!\n");
        }
    }

    mutex_destroy(&collision_mutex);

    printf("\nGenrand thread test completed.\n");
    printf("This baseline test demonstrates current threading behavior.\n");

    return 0;
}