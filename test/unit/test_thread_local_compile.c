/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/**
 * @file test_thread_local_compile.c
 * @brief Test that thread_local.h compiles correctly on this platform
 */

/* Define to enable thread-local storage */
#define PS_USE_THREAD_LOCAL_RNG

#include "util/thread_local.h"
#include <stdio.h>
#include <assert.h>

/* Test that PS_THREAD_LOCAL is defined */
#ifndef PS_THREAD_LOCAL
#error "PS_THREAD_LOCAL should be defined when PS_USE_THREAD_LOCAL_RNG is set"
#endif

/* Test basic compilation with different types */
PS_THREAD_LOCAL int tls_int = 0;
PS_THREAD_LOCAL unsigned long tls_array[10];
PS_THREAD_LOCAL char *tls_ptr = NULL;

/* Test static thread-local (most common case) */
static PS_THREAD_LOCAL int static_tls = 42;

/* Test in function scope */
void test_function_scope(void)
{
    static PS_THREAD_LOCAL int func_tls = 0;
    func_tls++;
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    /* Basic sanity check - can we access thread-local variables? */
    tls_int = 123;
    assert(tls_int == 123);

    tls_array[0] = 456;
    assert(tls_array[0] == 456);

    assert(static_tls == 42);
    static_tls = 84;
    assert(static_tls == 84);

    test_function_scope();

    printf("Thread-local storage compilation test PASSED\n");
    return 0;
}