#include <check.h>
#include <stdlib.h>
#include <pthread.h>
#include "spinlock.h"

#define NUM_THREADS 4
#define INCREMENTS_PER_THREAD 100000

/* Shared resources for the concurrent test execution */
static spinlock_t shared_lock = SPINLOCK_UNLOCKED;
static volatile long global_counter = 0;

/* Thread worker routine that increments a counter under spinlock protection */
static void* spinlock_worker(void* arg)
{
    for (int i = 0; i < INCREMENTS_PER_THREAD; i++) {
        spinlockLock(&shared_lock);
        global_counter++;
        spinlockUnlock(&shared_lock);
    }
    return NULL;
}

/* Test 1: Verify standard single-threaded sequential lock acquisition and release */
START_TEST(test_spinlock_basic_operations)
{
    spinlock_t lock = SPINLOCK_UNLOCKED;

    /* TryLock should succeed on an open lock */
    ck_assert_int_eq(spinlockLockTry(&lock), true);

    /* Subsequent TryLock must fail while the lock is held */
    ck_assert_int_eq(spinlockLockTry(&lock), false);

    /* Unlock the spinlock and verify it can be captured again */
    spinlockUnlock(&lock);
    ck_assert_int_eq(spinlockLockTry(&lock), true);
    
    spinlockUnlock(&lock);
}
END_TEST

/* Test 2: Stress test the spinlock implementation with multiple competing pthreads */
START_TEST(test_spinlock_concurrency)
{
    pthread_t threads[NUM_THREADS];
    shared_lock = SPINLOCK_UNLOCKED;
    global_counter = 0;

    /* Spawn threads to concurrently race for the counter */
    for (int i = 0; i < NUM_THREADS; i++) {
        ck_assert_int_eq(pthread_create(&threads[i], NULL, spinlock_worker, NULL), 0);
    }

    /* Wait for all workers to complete execution */
    for (int i = 0; i < NUM_THREADS; i++) {
        ck_assert_int_eq(pthread_join(threads[i], NULL), 0);
    }

    /* Without mutual exclusion, data races would result in a lower counter value */
    long expected_value = (long)NUM_THREADS * INCREMENTS_PER_THREAD;
    ck_assert_int_eq(global_counter, expected_value);
}
END_TEST

/* Test suite assembly for the Spinlock algorithm components */
Suite *spinlock_suite(void)
{
    Suite *s = suite_create("Spinlock_Algorithm_Suite");
    TCase *tc_core = tcase_create("Core_Tests");

    tcase_add_test(tc_core, test_spinlock_basic_operations);
    tcase_add_test(tc_core, test_spinlock_concurrency);
    
    suite_add_tcase(s, tc_core);
    return s;
}

/* REQUIRED ENTRY POINT: Fixed missing or malformed main wrapper execution path */
int main(void)
{
    int number_failed;
    Suite *s = spinlock_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}