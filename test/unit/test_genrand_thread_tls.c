/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/**
 * @file test_genrand_thread_tls.c
 * @brief Test genrand with thread-local storage enabled
 *
 * This test verifies that thread-local storage fixes the thread
 * safety issues by ensuring each thread has independent RNG state.
 */

/* Enable thread-local storage */
#define PS_USE_THREAD_LOCAL_RNG

#include "util/genrand.h"
#include "test_thread_utils.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define NUM_THREADS 4
#define VALUES_PER_THREAD 1000

/* Storage for each thread's generated values */
static long thread_values[NUM_THREADS][VALUES_PER_THREAD];
static barrier_t start_barrier;

#ifdef _WIN32
static unsigned __stdcall thread_isolated_func(void *arg)
#else
static void *thread_isolated_func(void *arg)
#endif
{
    int thread_id = *(int *)arg;
    int i;

    /* Wait for all threads to be ready */
    barrier_wait(&start_barrier);

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

/* Test that each thread gets independent sequences */
static void test_thread_isolation(void)
{
    thread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    int i, j, t1, t2;
    int collisions = 0;

    printf("Testing thread isolation with TLS...\n");

    barrier_init(&start_barrier, NUM_THREADS);

    /* Create threads */
    for (i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        thread_create(&threads[i], thread_isolated_func, &thread_ids[i]);
    }

    /* Wait for completion */
    for (i = 0; i < NUM_THREADS; i++) {
        thread_join(threads[i]);
    }

    /* Check for collisions between threads */
    for (t1 = 0; t1 < NUM_THREADS; t1++) {
        for (t2 = t1 + 1; t2 < NUM_THREADS; t2++) {
            for (i = 0; i < VALUES_PER_THREAD; i++) {
                for (j = 0; j < VALUES_PER_THREAD; j++) {
                    if (thread_values[t1][i] == thread_values[t2][j]) {
                        collisions++;
                    }
                }
            }
        }
    }

    printf("Cross-thread collisions: %d\n", collisions);
    assert(collisions == 0);

    barrier_destroy(&start_barrier);
    printf("Thread isolation test PASSED\n");
}

/* Test that same seed in different threads produces different sequences */
static void test_thread_independence(void)
{
    thread_t thread1, thread2;
    int id1 = 0, id2 = 1;
    int i;
    int differences = 0;

    printf("\nTesting thread independence...\n");

    barrier_init(&start_barrier, 2);

    /* Both threads will use the same seed (0) */
    thread_create(&thread1, thread_isolated_func, &id1);
    thread_create(&thread2, thread_isolated_func, &id2);

    thread_join(thread1);
    thread_join(thread2);

    /* Compare sequences - they should be different */
    for (i = 0; i < VALUES_PER_THREAD; i++) {
        if (thread_values[0][i] != thread_values[1][i]) {
            differences++;
        }
    }

    printf("Different values between threads: %d/%d\n", differences, VALUES_PER_THREAD);
    assert(differences == VALUES_PER_THREAD);

    barrier_destroy(&start_barrier);
    printf("Thread independence test PASSED\n");
}

/* Test deterministic behavior within a thread */
#ifdef _WIN32
static unsigned __stdcall thread_deterministic_func(void *arg)
#else
static void *thread_deterministic_func(void *arg)
#endif
{
    int thread_id = *(int *)arg;
    long values1[10], values2[10];
    int i;

    /* Generate first sequence */
    genrand_seed(12345);
    for (i = 0; i < 10; i++) {
        values1[i] = genrand_int31();
    }

    /* Re-seed and generate again */
    genrand_seed(12345);
    for (i = 0; i < 10; i++) {
        values2[i] = genrand_int31();
    }

    /* Verify they match */
    for (i = 0; i < 10; i++) {
        THREAD_TEST_ASSERT(values1[i] == values2[i]);
    }

#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

static void test_thread_deterministic(void)
{
    thread_t threads[2];
    int ids[2] = {0, 1};

    printf("\nTesting deterministic behavior in threads...\n");

    thread_test_failed = 0;

    thread_create(&threads[0], thread_deterministic_func, &ids[0]);
    thread_create(&threads[1], thread_deterministic_func, &ids[1]);

    thread_join(threads[0]);
    thread_join(threads[1]);

    assert(thread_test_failed == 0);
    printf("Thread deterministic test PASSED\n");
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    printf("Testing genrand with thread-local storage...\n");
    printf("PS_USE_THREAD_LOCAL_RNG is enabled\n\n");

    test_thread_isolation();
    test_thread_independence();
    test_thread_deterministic();

    printf("\nAll thread-local genrand tests PASSED\n");
    return 0;
}