/*
  test_logger.c
*/

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <setjmp.h>
#include <cmocka.h>

#include "util/deletion_queue.h"

static int setup(void** state) {
    // UNUSED
    (void)state;

    return 0;
}

static int teardown(void** state) {
    // UNUSED
    (void)state;

    return 0;
}

static void test_deletion_queue(void** state) {
    // UNUSED
    (void)state;

    // LOG_ERROR("Err!");
    // LOG_WARN("Warn!");
    // LOG_INFO("Info!");
    // LOG_DEBUG("Debug!");
    // LOG_TRACE("Trace!");
    // logger_close();

    // char buf[BUFFER_SIZE];
    // read_log_file(buf, sizeof(buf));
    // assert_true(strstr(buf, "[ERROR] ") != NULL);
    // assert_true(strstr(buf, "[WARN] ") != NULL);
    // assert_true(strstr(buf, "[INFO] ") != NULL);
    // assert_true(strstr(buf, "[DEBUG] ") != NULL);
    // assert_true(strstr(buf, "[TRACE] ") != NULL);
}

// const struct CMUnitTest deletion_queue_tests[] = {
//     // cmocka_unit_test_setup_teardown(test_log_file_creation, setup, teardown),
// };
//
// const size_t deletion_queue_tests_count = sizeof(deletion_queue_tests) / sizeof(deletion_queue_tests[0]);
