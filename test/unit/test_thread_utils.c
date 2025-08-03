/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/**
 * @file test_thread_utils.c
 * @brief Thread utilities implementation for multi-threaded testing
 */

#include "test_thread_utils.h"
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <errno.h>
#endif

/* Global flag for test failures */
volatile int thread_test_failed = 0;

/* Thread management */
int thread_create(thread_t *thread, thread_func_t func, void *arg)
{
#ifdef _WIN32
    *thread = (HANDLE)_beginthreadex(NULL, 0, func, arg, 0, NULL);
    return (*thread == 0) ? -1 : 0;
#else
    return pthread_create(thread, NULL, func, arg);
#endif
}

int thread_join(thread_t thread)
{
#ifdef _WIN32
    DWORD result = WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    return (result == WAIT_OBJECT_0) ? 0 : -1;
#else
    return pthread_join(thread, NULL);
#endif
}

/* Mutex operations */
int mutex_init(mutex_t *mutex)
{
#ifdef _WIN32
    InitializeCriticalSection(mutex);
    return 0;
#else
    return pthread_mutex_init(mutex, NULL);
#endif
}

int mutex_destroy(mutex_t *mutex)
{
#ifdef _WIN32
    DeleteCriticalSection(mutex);
    return 0;
#else
    return pthread_mutex_destroy(mutex);
#endif
}

int mutex_lock(mutex_t *mutex)
{
#ifdef _WIN32
    EnterCriticalSection(mutex);
    return 0;
#else
    return pthread_mutex_lock(mutex);
#endif
}

int mutex_unlock(mutex_t *mutex)
{
#ifdef _WIN32
    LeaveCriticalSection(mutex);
    return 0;
#else
    return pthread_mutex_unlock(mutex);
#endif
}

/* Barrier operations */
int barrier_init(barrier_t *barrier, int count)
{
#ifdef _WIN32
    barrier->event = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!barrier->event) return -1;
    barrier->count = 0;
    barrier->target = count;
    return 0;
#elif defined(__APPLE__)
    pthread_mutex_init(&barrier->mutex, NULL);
    pthread_cond_init(&barrier->cond, NULL);
    barrier->count = 0;
    barrier->target = count;
    return 0;
#else
    return pthread_barrier_init(barrier, NULL, count);
#endif
}

int barrier_destroy(barrier_t *barrier)
{
#ifdef _WIN32
    CloseHandle(barrier->event);
    return 0;
#elif defined(__APPLE__)
    pthread_mutex_destroy(&barrier->mutex);
    pthread_cond_destroy(&barrier->cond);
    return 0;
#else
    return pthread_barrier_destroy(barrier);
#endif
}

int barrier_wait(barrier_t *barrier)
{
#ifdef _WIN32
    LONG count = InterlockedIncrement(&barrier->count);
    if (count >= barrier->target) {
        SetEvent(barrier->event);
        return 1; /* Last thread */
    }
    WaitForSingleObject(barrier->event, INFINITE);
    return 0;
#elif defined(__APPLE__)
    int last = 0;
    pthread_mutex_lock(&barrier->mutex);
    barrier->count++;
    if (barrier->count >= barrier->target) {
        barrier->count = 0;
        pthread_cond_broadcast(&barrier->cond);
        last = 1;
    } else {
        pthread_cond_wait(&barrier->cond, &barrier->mutex);
    }
    pthread_mutex_unlock(&barrier->mutex);
    return last;
#else
    int ret = pthread_barrier_wait(barrier);
    if (ret == PTHREAD_BARRIER_SERIAL_THREAD)
        return 1; /* Last thread */
    return (ret == 0) ? 0 : -1;
#endif
}

/* Platform-independent sleep */
void thread_sleep_ms(int milliseconds)
{
#ifdef _WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}

/* Memory tracking stubs - could be enhanced with real tracking */
static size_t memory_baseline = 0;

void thread_test_start_memory_tracking(void)
{
    /* In a real implementation, we'd capture current memory usage */
    memory_baseline = 0;
}

int thread_test_check_memory_leaks(void)
{
    /* In a real implementation, we'd compare current usage to baseline */
    /* For now, just return "no leaks" */
    return 0;
}