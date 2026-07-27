#include <check.h>
#include <stdlib.h>
#include <string.h>
#include "dlist.h"

/* Test 1: Verify layout lifecycle initialization pipelines */
START_TEST(test_dlist_lifecycle)
{
    dlist_t *list = dlist_alloc();
    ck_assert_ptr_nonnull(list);
    ck_assert_int_eq(dlist_is_empty(list), true);
    
    dlist_free(list);
}
END_TEST

/* Test 2: Standard element addition and simple bound checks */
START_TEST(test_dlist_insert_operations)
{
    dlist_t *list = dlist_alloc();
    ck_assert_ptr_nonnull(list);

    const char *payload = "DListTestData";
    size_t payload_len = strlen(payload) + 1;

    void *added_node = dlist_add(list, 100, payload, payload_len);
    ck_assert_ptr_nonnull(added_node);
    ck_assert_int_eq(dlist_is_empty(list), false);

    /* Clean exit steps */
    dlist_release(added_node);
    dlist_free(list);
}
END_TEST

/* Test suite assembly for the Doubly Linked List subsystem */
Suite *dlist_suite(void)
{
    Suite *s = suite_create("Doubly_Linked_List_Suite");
    TCase *tc_core = tcase_create("Core_Tests");

    tcase_add_test(tc_core, test_dlist_lifecycle);
    tcase_add_test(tc_core, test_dlist_insert_operations);
    
    suite_add_tcase(s, tc_core);
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = dlist_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}