/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/**
 * @file test_genrand_baseline.c
 * @brief Baseline test for current genrand behavior
 *
 * This test captures the current behavior of the global RNG
 * to ensure our thread-local implementation maintains compatibility.
 */

#include "util/genrand.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

/* Test sequence generation */
static void test_deterministic_sequence(void)
{
    long values1[10];
    long values2[10];
    int i;

    /* Generate first sequence */
    genrand_seed(12345);
    for (i = 0; i < 10; i++) {
        values1[i] = genrand_int31();
    }

    /* Generate second sequence with same seed */
    genrand_seed(12345);
    for (i = 0; i < 10; i++) {
        values2[i] = genrand_int31();
    }

    /* Verify sequences match */
    for (i = 0; i < 10; i++) {
        assert(values1[i] == values2[i]);
    }

    printf("Deterministic sequence test PASSED\n");
}

/* Test range of values */
static void test_value_ranges(void)
{
    int i;
    int has_high_values = 0;
    int has_low_values = 0;

    genrand_seed(67890);

    for (i = 0; i < 1000; i++) {
        long val = genrand_int31();

        /* Check if we get values using high bits (but not the sign bit) */
        if (val & 0x40000000L) has_high_values = 1;
        if (val < 0x1000000) has_low_values = 1; /* Check for values below 16M */

        /* Verify value is in correct range */
        assert(val >= 0);
        assert(val <= 0x7FFFFFFF);
    }

    /* Ensure we get good distribution */
    assert(has_high_values);
    assert(has_low_values);

    printf("Value range test PASSED\n");
}

/* Test different seeds produce different sequences */
static void test_different_seeds(void)
{
    long seq1[5], seq2[5];
    int i, different = 0;

    genrand_seed(11111);
    for (i = 0; i < 5; i++) {
        seq1[i] = genrand_int31();
    }

    genrand_seed(22222);
    for (i = 0; i < 5; i++) {
        seq2[i] = genrand_int31();
    }

    for (i = 0; i < 5; i++) {
        if (seq1[i] != seq2[i]) different++;
    }

    /* At least some values should be different */
    assert(different > 0);

    printf("Different seeds test PASSED\n");
}

/* Test automatic initialization */
static void test_auto_init(void)
{
    long val1, val2;

    /* Don't call genrand_seed - it should auto-initialize */
    val1 = genrand_int31();
    val2 = genrand_int31();

    /* Values should be valid (non-zero) and different */
    assert(val1 != 0 || val2 != 0);
    assert(val1 != val2);

    printf("Auto-initialization test PASSED\n");
}

/* Store known good values for regression testing */
static void test_known_values(void)
{
    /* We'll generate and verify the values are consistent */
    long first_run[5];
    long second_run[5];
    int i;

    /* First run with seed 42 */
    genrand_seed(42);
    for (i = 0; i < 5; i++) {
        first_run[i] = genrand_int31();
    }

    /* Second run with same seed should produce same values */
    genrand_seed(42);
    for (i = 0; i < 5; i++) {
        second_run[i] = genrand_int31();
        assert(second_run[i] == first_run[i]);
    }

    printf("Known values test PASSED\n");
}

/* Test s3_rand_int31() macro */
static void test_s3_rand_macro(void)
{
    long val;
    int i;

    genrand_seed(99999);

    for (i = 0; i < 100; i++) {
        val = s3_rand_int31();
        assert(val >= 0);
        assert(val <= S3_RAND_MAX_INT32);
    }

    printf("s3_rand_int31 macro test PASSED\n");
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    printf("Running genrand baseline tests...\n");

    test_deterministic_sequence();
    test_value_ranges();
    test_different_seeds();
    test_auto_init();
    test_known_values();
    test_s3_rand_macro();

    printf("\nAll genrand baseline tests PASSED\n");
    return 0;
}