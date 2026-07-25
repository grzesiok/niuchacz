#include <check.h>
#include <stdlib.h>
#include <string.h>
#include "bst.h"

/* Test 1: Verify the basic allocation and destruction of the BST instance */
START_TEST(test_bst_lifecycle)
{
    bst_t *tree = bst_create(NULL);
    ck_assert_ptr_nonnull(tree);
    ck_assert_ptr_null(tree->_root);
    
    bst_destroy(tree);
}
END_TEST

/* Test 2: Standard insertion and retrieval of nodes from the binary tree */
START_TEST(test_bst_insert_and_search)
{
    bst_t *tree = bst_create(NULL);
    ck_assert_ptr_nonnull(tree);

    const char *data1 = "NodeData50";
    const char *data2 = "NodeData25";
    const char *data3 = "NodeData75";
    
    char buffer[32];
    size_t read_bytes;

    /* Insert root and child nodes with specific integer keys */
    ck_assert_int_eq(bst_insert(tree, 50, data1, strlen(data1) + 1, NULL), (int)(strlen(data1) + 1));
    ck_assert_int_eq(bst_insert(tree, 25, data2, strlen(data2) + 1, NULL), (int)(strlen(data2) + 1));
    ck_assert_int_eq(bst_insert(tree, 75, data3, strlen(data3) + 1, NULL), (int)(strlen(data3) + 1));

    /* Search for an existing node (Root node) */
    memset(buffer, 0, sizeof(buffer));
    read_bytes = bst_search(tree, 50, buffer, sizeof(buffer));
    ck_assert_int_eq(read_bytes, strlen(data1) + 1);
    ck_assert_str_eq(buffer, data1);

    /* Search for an existing node (Left child node) */
    memset(buffer, 0, sizeof(buffer));
    read_bytes = bst_search(tree, 25, buffer, sizeof(buffer));
    ck_assert_int_eq(read_bytes, strlen(data2) + 1);
    ck_assert_str_eq(buffer, data2);

    /* Search for a non-existing key (Must return -1 or casted equivalent) */
    read_bytes = bst_search(tree, 999, buffer, sizeof(buffer));
    ck_assert_int_eq((ssize_t)read_bytes, -1);

    bst_destroy(tree);
}
END_TEST

/* Test 3: Standard removal of nodes and structure healing assertions */
START_TEST(test_bst_deletion)
{
    bst_t *tree = bst_create(NULL);
    ck_assert_ptr_nonnull(tree);

    const char *dummy = "data";
    char buffer[16];

    /* Setup a small leaf-node tree structure */
    bst_insert(tree, 50, dummy, strlen(dummy) + 1, NULL);
    bst_insert(tree, 30, dummy, strlen(dummy) + 1, NULL);
    bst_insert(tree, 70, dummy, strlen(dummy) + 1, NULL);

    /* Verify node 30 is reachable before deletion */
    ck_assert_int_gt((ssize_t)bst_search(tree, 30, buffer, sizeof(buffer)), 0);

    /* Delete the target leaf node */
    bst_delete(tree, 30);

    /* Verify node 30 is missing while node 50 and 70 remain active */
    ck_assert_int_eq((ssize_t)bst_search(tree, 30, buffer, sizeof(buffer)), -1);
    ck_assert_int_gt((ssize_t)bst_search(tree, 50, buffer, sizeof(buffer)), 0);
    ck_assert_int_gt((ssize_t)bst_search(tree, 70, buffer, sizeof(buffer)), 0);

    bst_destroy(tree);
}
END_TEST

/* Test suite assembly using Check framework */
Suite *bst_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("BST_Algorithm_Suite");
    tc_core = tcase_create("Core_Tests");

    tcase_add_test(tc_core, test_bst_lifecycle);
    tcase_add_test(tc_core, test_bst_insert_and_search);
    tcase_add_test(tc_core, test_bst_deletion);
    
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = bst_suite();
    SRunner *sr = srunner_create(s);

    /* Run tests and report output to standard out stream */
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}