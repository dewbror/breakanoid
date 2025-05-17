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

#define LOG_LEVEL 4
#include "logger.h"

#define BUFFER_SIZE 512

// Helper to read the log file into a buffer
static void read_log_file(char* buffer, size_t bufsize) {
    FILE* f = fopen("./break.log", "r");
    assert_non_null(f);
    size_t n  = fread(buffer, 1, bufsize - 1, f);
    buffer[n] = 0;
    fclose(f);
}

static int setup(void** state) {
    // UNUSED
    (void)state;

    logger_open("./break.log");

    return 0;
}

static int teardown(void** state) {
    // UNUSED
    (void)state;

    logger_close();
    remove("./break.log");

    return 0;
}

// Test that the log file has been created (in setup)
static void test_log_file_creation(void** state) {
    // UNUSED
    (void)state;

    FILE* f = fopen("./break.log", "r");
    assert_non_null(f);
    fclose(f);
}

static void test_log_message_written(void** state) {
    // UNUSED
    (void)state;

    LOG_ERROR("Test message: %d", 42);
    logger_close();

    char buf[BUFFER_SIZE];
    read_log_file(buf, sizeof(buf));
    assert_true(strstr(buf, "Test message: 42") != NULL);
    assert_true(strstr(buf, "[ERROR] ") != NULL);
}

static void test_log_level_color_prefix(void** state) {
    // UNUSED
    (void)state;

    LOG_ERROR("Err!");
    LOG_WARN("Warn!");
    LOG_INFO("Info!");
    LOG_DEBUG("Debug!");
    LOG_TRACE("Trace!");
    logger_close();

    char buf[BUFFER_SIZE];
    read_log_file(buf, sizeof(buf));
    assert_true(strstr(buf, "[ERROR] ") != NULL);
    assert_true(strstr(buf, "[WARN] ") != NULL);
    assert_true(strstr(buf, "[INFO] ") != NULL);
    assert_true(strstr(buf, "[DEBUG] ") != NULL);
    assert_true(strstr(buf, "[TRACE] ") != NULL);
}

const struct CMUnitTest logger_tests[] = {
    cmocka_unit_test_setup_teardown(test_log_file_creation, setup, teardown),
    cmocka_unit_test_setup_teardown(test_log_message_written, setup, teardown),
    cmocka_unit_test_setup_teardown(test_log_level_color_prefix, setup, teardown),
};

const size_t logger_tests_count = sizeof(logger_tests) / sizeof(logger_tests[0]);
