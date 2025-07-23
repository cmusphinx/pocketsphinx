/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/**
 * @file test_thread_utils.h
 * @brief Thread utilities for multi-threaded testing
 */

#ifndef _TEST_THREAD_UTILS_H_
#define _TEST_THREAD_UTILS_H_

#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
typedef HANDLE thread_t;
typedef CRITICAL_SECTION mutex_t;
typedef struct {
    HANDLE event;
    LONG count;
    LONG target;
} barrier_t;
#else
#include <pthread.h>
typedef pthread_t thread_t;
typedef pthread_mutex_t mutex_t;

/* macOS doesn't have pthread_barrier_t, so provide our own */
#ifdef __APPLE__
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int count;
    int target;
} barrier_t;
#else
typedef pthread_barrier_t barrier_t;
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Thread function signature */
#ifdef _WIN32
typedef unsigned (__stdcall *thread_func_t)(void *);
#else
typedef void *(*thread_func_t)(void *);
#endif

/* Thread management */
int thread_create(thread_t *thread, thread_func_t func, void *arg);
int thread_join(thread_t thread);

/* Synchronization */
int mutex_init(mutex_t *mutex);
int mutex_destroy(mutex_t *mutex);
int mutex_lock(mutex_t *mutex);
int mutex_unlock(mutex_t *mutex);

int barrier_init(barrier_t *barrier, int count);
int barrier_destroy(barrier_t *barrier);
int barrier_wait(barrier_t *barrier);

/* Test assertions that work across threads */
#define THREAD_TEST_ASSERT(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "Assertion failed: %s at %s:%d\n", \
                #cond, __FILE__, __LINE__); \
        thread_test_failed = 1; \
    } \
} while (0)

/* Global flag for test failures in threads */
extern volatile int thread_test_failed;

/* Memory leak detection helpers */
void thread_test_start_memory_tracking(void);
int thread_test_check_memory_leaks(void);

/* Platform-independent sleep */
void thread_sleep_ms(int milliseconds);

#ifdef __cplusplus
}
#endif

#endif /* _TEST_THREAD_UTILS_H_ */