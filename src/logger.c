/*
  logger.c
*/

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "logger.h"

#define TIMEBUF_MAX          32
#define FILELINE_PRINT_LEVEL 5 // fileline printing disabled

#define COLOR_RED     "\x1b[31m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"

static FILE* log_file                  = NULL;
static const char* const log_file_name = "./breakanoid.log";
static const char* const levels[]      = {"[ERROR] ", "[WARN]  ", "[INFO]  ", "[DEBUG] ", "[TRACE] "};

void open_log_file(void) {
    LOG_TRACE("Attempting to open log file: %s", log_file_name);

    // Open file for appending or create it if it doesnt exists
    log_file = fopen(log_file_name, "w");
    if(log_file == NULL) {
        LOG_WARN("Could not open log file: %s", log_file_name);
        return;
    }

    LOG_INFO("Opened log file: %s", log_file_name);
}

void close_log_file(void) {
    LOG_TRACE("Attempting to close log file: %s", log_file_name);
    if(log_file != NULL) {
        int ret = fclose(log_file);
        if(ret < 0) {
            LOG_ERROR("Failed to close log file: %s", log_file_name);
            return;
        }
        log_file = NULL;
        LOG_INFO("Closed log file: %s", log_file_name);
    } else {
        LOG_WARN("Tried to close log file: %s, but file pointer was NULL", log_file_name);
    }
}

void logger__msg(log_level level, const char* file, int line, const char* fmt, ...) {
    int ret           = 0;
    size_t ret_LU     = 0;
    const char* color = NULL;

    switch(level) {
        case LOG_ERROR:
            color = COLOR_RED;
            break;
        case LOG_WARN:
            color = COLOR_YELLOW;
            break;
        case LOG_INFO:
            color = COLOR_GREEN;
            break;
        case LOG_DEBUG:
            color = COLOR_BLUE;
            break;
        case LOG_TRACE:
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

    // Print date, time, level etc to stderr
#if LOG_LEVEL >= FILELINE_PRINT_LEVEL
    // Handle if file == NULL
    if(file != NULL) {
        ret = fprintf(stderr, "%s %s%s%s", timebuf, color, levels[level], COLOR_RESET);
    } else {
        ret = fprintf(stderr, "%s %s%s%s(%s:%d)", timebuf, color, levels[level], COLOR_RESET, file, line);
    }
#else
    (void)file;
    (void)line;
    ret = fprintf(stderr, "%s %s%s%s", timebuf, color, levels[level], COLOR_RESET);
#endif
    if(ret < 0) {
        // handle fprintf error
        return;
    }

    // Print date, time, level etc to log_file
#if LOG_LEVEL >= FILELINE_PRINT_LEVEL
    if(log_file != NULL) {
        // Handle if file == NULL
        if(file != NULL) {
            ret = fprintf(log_file, "%s %s%s%s", timebuf, color, levels[level], COLOR_RESET);
        } else {
            ret = fprintf(log_file, "%s %s%s%s(%s:%d)", timebuf, color, levels[level], COLOR_RESET, file, line);
        }
    }
#else
    if(log_file != NULL) {
        ret = fprintf(log_file, "%s %s", timebuf, levels[level]);
    }
#endif
    if(ret < 0) {
        // handle fprintf error
        return;
    }

    va_list args;
    va_list args_copy;
    va_start(args, fmt);

    // Do NOT use args after having used it in vfprintf, create a copy and use the copy instead.
    va_copy(args_copy, args);

    // Print the formatted string to stderr
    ret = vfprintf(stderr, fmt, args);
    if(ret < 0) {
        // Handle vfprintf error
        return;
    }

    // Print the formatted string to log_file. USE args_copy
    if(log_file != NULL) {
        ret = vfprintf(log_file, fmt, args_copy);
    }
    if(ret < 0) {
        // Handle vfprintf error
        return;
    }

    va_end(args);

    // Print newline to stderr
    ret = fprintf(stderr, "\n");
    if(ret < 0) {
        // Handle fprintf error
        return;
    }

    // Print newline to log_file
    if(log_file != NULL) {
        ret = fprintf(log_file, "\n");
    }
    if(ret < 0) {
        // Handle fprintf error
        return;
    }
}
