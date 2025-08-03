/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/**
 * @file test_thread_local_basic.c
 * @brief Test that thread-local variables have independent values per thread
 */

/* Define to enable thread-local storage */
#define PS_USE_THREAD_LOCAL_RNG

#include "util/thread_local.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

/* Thread-local variable to test */
static PS_THREAD_LOCAL int thread_value = 0;
static PS_THREAD_LOCAL int thread_id = -1;

/* Shared data for verification */
#define NUM_THREADS 2
static int results[NUM_THREADS];

#ifdef _WIN32
static unsigned __stdcall thread_func(void *arg)
#else
static void *thread_func(void *arg)
#endif
{
    int id = *(int *)arg;

    /* Set thread-local values */
    thread_id = id;
    thread_value = (id + 1) * 100;

    /* Sleep briefly to ensure both threads run concurrently */
#ifdef _WIN32
    Sleep(10);
#else
    usleep(10000);
#endif

    /* Verify our values haven't been changed by other thread */
    assert(thread_id == id);
    assert(thread_value == (id + 1) * 100);

    /* Store result for main thread to verify */
    results[id] = thread_value;

    printf("Thread %d: thread_value = %d\n", id, thread_value);

#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

int main(int argc, char *argv[])
{
    int thread_ids[NUM_THREADS];
    int i;

#ifdef _WIN32
    HANDLE threads[NUM_THREADS];
#else
    pthread_t threads[NUM_THREADS];
#endif

    (void)argc;
    (void)argv;

    printf("Testing thread-local storage with %d threads...\n", NUM_THREADS);

    /* Initialize thread IDs */
    for (i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
    }

    /* Create threads */
    for (i = 0; i < NUM_THREADS; i++) {
#ifdef _WIN32
        threads[i] = (HANDLE)_beginthreadex(NULL, 0, thread_func,
                                           &thread_ids[i], 0, NULL);
        assert(threads[i] != 0);
#else
        int ret = pthread_create(&threads[i], NULL, thread_func, &thread_ids[i]);
        assert(ret == 0);
#endif
    }

    /* Wait for threads to complete */
    for (i = 0; i < NUM_THREADS; i++) {
#ifdef _WIN32
        WaitForSingleObject(threads[i], INFINITE);
        CloseHandle(threads[i]);
#else
        pthread_join(threads[i], NULL);
#endif
    }

    /* Verify results */
    for (i = 0; i < NUM_THREADS; i++) {
        int expected = (i + 1) * 100;
        assert(results[i] == expected);
        printf("Thread %d result verified: %d\n", i, results[i]);
    }

    /* Verify main thread has its own independent values */
    assert(thread_id == -1);
    assert(thread_value == 0);

    printf("Thread-local storage basic test PASSED\n");
    return 0;
}