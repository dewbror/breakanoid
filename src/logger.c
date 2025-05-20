#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "logger.h"

#define TIMEBUF_MAX 32

#define COLOR_RED    "\x1b[31m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_GREEN  "\x1b[32m"
#define COLOR_BLUE   "\x1b[34m"
// #define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN  "\x1b[36m"
#define COLOR_RESET "\x1b[0m"

static bool logging_to_file       = true;
static FILE* fp_log               = NULL;
static const char* log_file_name  = NULL;
static const char* const levels[] = {"[ERROR] ", "[WARN] ", "[INFO] ", "[DEBUG] ", "[TRACE] "};

void logger_open(const char* file_name) {
    // If file_name == NULL, set logging to stderr
    if(file_name == NULL) {
        fp_log          = stderr;
        logging_to_file = false;
        return;
    }

    // If file_name != NULL, open it as logging file
    fp_log = fopen(file_name, "w");
    if(fp_log == NULL) {
        // Handle fopen error
        return;
    }

    log_file_name = file_name;
    LOG_INFO("Log file opened: %s", log_file_name);
}

void logger_close(void) {
    if(!logging_to_file) {
        return;
    }
    LOG_DEBUG("Attempting to close log file: %s", log_file_name);

    // Check if log_file is NULL
    if(fp_log != NULL) {
        // Close log_file
        int ret = fclose(fp_log);
        if(ret < 0) {
            // Handle fclose error
            return;
        }
        // Set free pointer to NULL
        fp_log = NULL;
    }
}

void logger__msg(int level, const char* file, int line, const char* fmt, ...) {
    int ret           = 0;
    size_t ret_LU     = 0;
    const char* color = NULL;

    // Set the color based on the log level of the message
    switch(level) {
        case LOG_LEVEL_ERROR:
            color = COLOR_RED;
            break;
        case LOG_LEVEL_WARN:
            color = COLOR_YELLOW;
            break;
        case LOG_LEVEL_INFO:
            color = COLOR_GREEN;
            break;
        case LOG_LEVEL_DEBUG:
            color = COLOR_BLUE;
            break;
        case LOG_LEVEL_TRACE:
            color = COLOR_CYAN;
            break;
        default:
            color = COLOR_RESET;
            break;
    }

    // Get the current time
    time_t now        = time(NULL);
    struct tm* tm_now = localtime(&now);

    // Format time string
    char timebuf[TIMEBUF_MAX];
    ret_LU = strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", tm_now);
    if(ret_LU == 0) {
        // Handle strftime error
        return;
    }

    if(fp_log == NULL) {
        // If fp_log == NULL print an error directly to stderr.
        // fprintf(stderr, "%s %s[ERROR]%s fp_log == NULL in logger.c\n", timebuf, COLOR_RED, COLOR_RESET);
        return;
    }

    va_list args;

    // Initialize variable argument list
    va_start(args, fmt);

    // UNUSED
    (void)file;
    (void)line;

    if(logging_to_file) {
        ret = fprintf(fp_log, "%s %s", timebuf, levels[level]);
        if(ret < 0) {
            // Handle fprintf error
            return;
        }
    } else {
        ret = fprintf(fp_log, "%s %s%s%s", timebuf, color, levels[level], COLOR_RESET);
        if(ret < 0) {
            // Handle fprintf error
            return;
        }
    }
    ret = vfprintf(fp_log, fmt, args);
    if(ret < 0) {
        // Handle vfprintf error
        return;
    }
    ret = fprintf(fp_log, "\n");
    if(ret < 0) {
        // Handle fprintf error
        return;
    }

    // Cleanup va lists
    va_end(args);
}
