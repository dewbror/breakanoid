
/*
  Author: William Brorsson <dewb@duck.com>
  Date Created: May 16, 2025

  tests_main.c
*/

// SDL_MAIN_IMPLEMENTATION
#include "SDL3/SDL_main.h"

#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <cmocka.h>

// test_logger.c
extern const struct CMUnitTest logger_tests[];
extern const size_t logger_tests_count;

int main(void) {
    int fail = 0;

    // Run the logger test group
    fail += _cmocka_run_group_tests("Logger tests", logger_tests, logger_tests_count, NULL, NULL);

    return fail;
}
