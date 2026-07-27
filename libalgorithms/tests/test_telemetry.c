#include <check.h>
#include <stdlib.h>
#include <string.h>
#include "telemetry.h"

struct telemetry_entry_t {
    telemetry_behavior_t flags;
    uint64_t value;
};

START_TEST(test_telemetry_lifecycle)
{
    ck_assert_int_eq(telemetry_mgr_start(), 0);
    
    telemetry_list_t *list = telemetry_list_create("test_pool");
    ck_assert_ptr_nonnull(list);
    
    telemetry_mgr_stop();
}
END_TEST

START_TEST(test_telemetry_operations)
{
    ck_assert_int_eq(telemetry_mgr_start(), 0);
    telemetry_list_t *list = telemetry_list_create("kernel_metrics");
    ck_assert_ptr_nonnull(list);
    
    telemetry_entry_t metric_sum;
    telemetry_entry_t metric_last;
    
    ck_assert_int_eq(telemetry_alloc(list, "stat.sum", TELEMETRY_TYPE_SUM, &metric_sum), 0);
    ck_assert_int_eq(telemetry_alloc(list, "stat.last", TELEMETRY_TYPE_LAST, &metric_last), 0);
    
    /* Assert SUM accumulation logic mechanics */
    telemetry_update(&metric_sum, 10);
    telemetry_update(&metric_sum, 15);
    ck_assert_uint_eq(telemetry_get_value(&metric_sum), 25ULL);
    
    /* Assert LAST override logic mechanics */
    telemetry_update(&metric_last, 100);
    telemetry_update(&metric_last, 200);
    ck_assert_uint_eq(telemetry_get_value(&metric_last), 200ULL);
    
    telemetry_mgr_stop();
}
END_TEST

Suite *telemetry_suite(void)
{
    Suite *s = suite_create("Telemetry_Core_Suite");
    TCase *tc_core = tcase_create("Core_Tests");

    tcase_add_test(tc_core, test_telemetry_lifecycle);
    tcase_add_test(tc_core, test_telemetry_operations);
    
    suite_add_tcase(s, tc_core);
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = telemetry_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}