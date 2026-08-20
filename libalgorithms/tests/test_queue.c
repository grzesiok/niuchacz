#include <check.h>
#include <stdlib.h>
#include <time.h>
#include "queue.h"

/* Test 1: Verify the basic allocation and destruction cycle of the queue */
START_TEST(test_queue_lifecycle)
{
    size_t q_size = 1024;
    queue_t *q = queue_create(q_size);
    ck_assert_ptr_nonnull(q);
    
    queue_destroy(q);
}
END_TEST

/* Test 2: Standard single-item push/pop operations with full SSE4.2 CRC32 evaluation */
START_TEST(test_queue_simple_write_read)
{
    queue_t *q = queue_create(512);
    ck_assert_ptr_nonnull(q);

    ck_assert_int_eq(queue_producer_new(q), true);
    ck_assert_int_eq(queue_consumer_new(q), true);

    const char *msg = "Hello SSE4.2 Queue!";
    size_t msg_len = strlen(msg) + 1;
    char *buffer = NULL;

    /* Write operation without timeout */
    int write_res = queue_write(q, msg, msg_len, NULL);
    ck_assert_int_eq(write_res, (int)msg_len);

    /* Read operation without timeout */
    int read_res = queue_read(q, (void **)&buffer, NULL);
    ck_assert_int_eq(read_res, (int)msg_len);
    ck_assert_str_eq(buffer, msg);
    free(buffer);

    queue_consumer_free(q);
    queue_producer_free(q);
    queue_destroy(q);
}
END_TEST

/* Test 3: Ensure writing an item that exceeds the queue capacity is rejected */
START_TEST(test_queue_oversized_payload)
{
    queue_t *q = queue_create(100);
    ck_assert_ptr_nonnull(q);

    ck_assert_int_eq(queue_producer_new(q), true);

    char large_buffer[200] = {0};
    /* Write must immediately fail due to strict size boundary violations */
    int write_res = queue_write(q, large_buffer, sizeof(large_buffer), NULL);
    ck_assert_int_eq(write_res, QUEUE_RET_ERROR);

    queue_producer_free(q);
    queue_destroy(q);
}
END_TEST

/* Test suite assembly using Check framework */
Suite *queue_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Queue_SSE42_Suite");
    tc_core = tcase_create("Core_Tests");

    tcase_add_test(tc_core, test_queue_lifecycle);
    tcase_add_test(tc_core, test_queue_simple_write_read);
    tcase_add_test(tc_core, test_queue_oversized_payload);
    
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = queue_suite();
    SRunner *sr = srunner_create(s);

    /* Run tests and report output to standard out stream */
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}