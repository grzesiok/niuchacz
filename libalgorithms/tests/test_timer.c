#include <check.h>
#include <stdlib.h>
#include <unistd.h>
#include "timer.h"

/* Test 1: Verify exact timespec conversion limits into raw nanoseconds data */
START_TEST(test_timer_conversion_and_null_checks)
{
    struct timespec ts = {.tv_sec = 2, .tv_nsec = 500};
    
    ck_assert_int_eq(timerIsNull(&ts), false);
    ck_assert_uint_eq(timerTimespecToNs(&ts), 2000000500ULL);
    
    struct timespec null_ts = {.tv_sec = 0, .tv_nsec = 0};
    ck_assert_int_eq(timerIsNull(&null_ts), true);
    ck_assert_int_eq(timerIsNull(NULL), true);
}
END_TEST

/* Test 2: Ensure evaluation comparison matches layout bounds sequentially */
START_TEST(test_timer_comparison)
{
    struct timespec ts1 = {.tv_sec = 10, .tv_nsec = 100};
    struct timespec ts2 = {.tv_sec = 10, .tv_nsec = 200};
    struct timespec ts3 = {.tv_sec = 11, .tv_nsec = 0};

    ck_assert_int_eq(timerCmp(&ts1, &ts2), -1);
    ck_assert_int_eq(timerCmp(&ts2, &ts1), 1);
    ck_assert_int_eq(timerCmp(&ts1, &ts1), 0);
    ck_assert_int_eq(timerCmp(&ts3, &ts2), 1);
}
END_TEST

/* Test 3: Validate latency registration sweeps using monotonic time delay loops */
START_TEST(test_timer_stopwatch_execution)
{
    struct timespec stopwatch;
    timerWatchStart(&stopwatch);
    
    /* Enforce a brief artificial execution context stall (50 milliseconds) */
    struct timespec delay = {.tv_sec = 0, .tv_nsec = 50000000};
    nanosleep(&delay, NULL);
    
    uint64_t elapsed_ns = timerWatchStop(&stopwatch);
    
    /* Ensure the elapsed threshold registered at least the requested delay */
    ck_assert_uint_gt(elapsed_ns, 45000000ULL);
}
END_TEST

Suite *timer_suite(void)
{
    Suite *s = suite_create("Timer_Utility_Suite");
    TCase *tc_core = tcase_create("Core_Tests");

    tcase_add_test(tc_core, test_timer_conversion_and_null_checks);
    tcase_add_test(tc_core, test_timer_comparison);
    tcase_add_test(tc_core, test_timer_stopwatch_execution);
    
    suite_add_tcase(s, tc_core);
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = timer_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}