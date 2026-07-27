#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "hll.h"
#include "algorithms/hash/hash.h" /* Declares hashvalue64 and hash64 types */

/* Forward declaration of your library's native 64-bit Murmur function to satisfy compiler checks */
uint64_t MurmurHash2_64(const void *key, int len, unsigned int seed);

/* 
 * FIXED: Adjusted signature types to strictly mirror your internal 'hash64' definition:
 * (unsigned char* data, size_t size, hashvalue64 seed)
 */
static hashvalue64 dummy_hash_wrapper(unsigned char *key, size_t len, hashvalue64 seed) {
    /* Cast parameters safely to match your internal Murmur implementation signature */
    return (hashvalue64)MurmurHash2_64((const void*)key, (int)len, (unsigned int)seed);
}

/* Test 1: Verify allocation flows and accurate zero counts for fresh maps */
START_TEST(test_hll_lifecycle)
{
    hll_t *hll = hll_create(dummy_hash_wrapper, 10); /* 2^10 = 1024 registers */
    ck_assert_ptr_nonnull(hll);
    ck_assert_uint_eq(hll_cardinality(hll), 0ULL);
    
    hll_destroy(hll);
}
END_TEST

/* Test 2: Standard convergence evaluation asserting estimation accuracy thresholds */
START_TEST(test_hll_approximation_accuracy)
{
    hll_t *hll = hll_create(dummy_hash_wrapper, 12); /* 4096 registers */
    ck_assert_ptr_nonnull(hll);

    char item_buffer[64];
    /* Insert 5000 unique sequential string identifiers */
    for (int i = 0; i < 5000; i++) {
        snprintf(item_buffer, sizeof(item_buffer), "item_unique_id_%d", i);
        hll_insert(hll, (unsigned char*)item_buffer, strlen(item_buffer));
    }

    unsigned long long cardinality = hll_cardinality(hll);
    
    /* Standard HLL error bound for p=12 is ~1.04/sqrt(4096) = 1.625% */
    /* Assert that the estimation stays comfortably within a safe 5% tolerance window */
    ck_assert_uint_gt(cardinality, 4750ULL);
    ck_assert_uint_lt(cardinality, 5250ULL);

    hll_destroy(hll);
}
END_TEST

Suite *hll_suite(void)
{
    Suite *s = suite_create("HyperLogLog_Stream_Suite");
    TCase *tc_core = tcase_create("Core_Tests");

    tcase_add_test(tc_core, test_hll_lifecycle);
    tcase_add_test(tc_core, test_hll_approximation_accuracy);
    
    suite_add_tcase(s, tc_core);
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = hll_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}