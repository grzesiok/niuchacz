#include <check.h>
#include <stdlib.h>
#include "algorithms/sizeparse.h"

START_TEST(test_sizeparse_basic)
{
    int ok = 0;
    size_t v;

    v = parse_size_string("512MB", &ok);
    ck_assert_int_eq(ok, 1);
    ck_assert_uint_eq(v, 512ULL * 1024ULL * 1024ULL);

    v = parse_size_string("1G", &ok);
    ck_assert_int_eq(ok, 1);
    ck_assert_uint_eq(v, 1024ULL * 1024ULL * 1024ULL);

    v = parse_size_string("2k", &ok);
    ck_assert_int_eq(ok, 1);
    ck_assert_uint_eq(v, 2ULL * 1024ULL);

    v = parse_size_string("1024", &ok);
    ck_assert_int_eq(ok, 1);
    ck_assert_uint_eq(v, 1024ULL);

    v = parse_size_string("xyz", &ok);
    ck_assert_int_eq(ok, 0);
    ck_assert_uint_eq(v, 0ULL);
}
END_TEST

Suite *sizeparse_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("SizeParse_Suite");
    tc_core = tcase_create("Core_Tests");

    tcase_add_test(tc_core, test_sizeparse_basic);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = sizeparse_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
