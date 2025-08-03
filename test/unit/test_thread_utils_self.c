/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/**
 * @file test_thread_utils_self.c
 * @brief Self-test for thread utilities
 */

#include "test_thread_utils.h"
#include <stdio.h>
#include <assert.h>

/* Shared data for testing */
static volatile int shared_counter = 0;
static mutex_t counter_mutex;
static barrier_t thread_barrier;

#ifdef _WIN32
static unsigned __stdcall test_thread_func(void *arg)
#else
static void *test_thread_func(void *arg)
#endif
{
    int thread_id = *(int *)arg;
    int i;

    printf("Thread %d: Started\n", thread_id);

    /* Test barrier - wait for all threads to start */
    barrier_wait(&thread_barrier);
    printf("Thread %d: Passed barrier\n", thread_id);

    /* Test mutex - increment shared counter */
    for (i = 0; i < 1000; i++) {
        mutex_lock(&counter_mutex);
        shared_counter++;
        mutex_unlock(&counter_mutex);
    }

    /* Test thread sleep */
    thread_sleep_ms(10);

    /* Test assertion macro */
    THREAD_TEST_ASSERT(1 == 1);
    THREAD_TEST_ASSERT(thread_id >= 0);

    printf("Thread %d: Completed\n", thread_id);

#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

int main(int argc, char *argv[])
{
    thread_t threads[4];
    int thread_ids[4];
    int i;

    (void)argc;
    (void)argv;

    printf("Testing thread utilities...\n");

    /* Reset test state */
    thread_test_failed = 0;
    shared_counter = 0;

    /* Initialize synchronization primitives */
    assert(mutex_init(&counter_mutex) == 0);
    assert(barrier_init(&thread_barrier, 4) == 0);

    /* Start memory tracking */
    thread_test_start_memory_tracking();

    /* Create threads */
    for (i = 0; i < 4; i++) {
        thread_ids[i] = i;
        assert(thread_create(&threads[i], test_thread_func, &thread_ids[i]) == 0);
    }

    /* Join threads */
    for (i = 0; i < 4; i++) {
        assert(thread_join(threads[i]) == 0);
    }

    /* Verify results */
    assert(shared_counter == 4000); /* 4 threads * 1000 increments */
    assert(thread_test_failed == 0);

    /* Check for memory leaks */
    assert(thread_test_check_memory_leaks() == 0);

    /* Cleanup */
    assert(mutex_destroy(&counter_mutex) == 0);
    assert(barrier_destroy(&thread_barrier) == 0);

    printf("Thread utilities self-test PASSED\n");
    printf("Shared counter: %d (expected 4000)\n", shared_counter);

    return 0;
}