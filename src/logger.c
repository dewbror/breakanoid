/*
  logger.c
*/

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "logger.h"

#define TIMEBUF_MAX 32
#define FILELINE_PRINT_LEVEL 5 // fileline printing disabled

#define COLOR_RED     "\x1b[31m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"

static const char* const levels[] = {"[ERROR] ", "[WARN]  ", "[INFO]  ", "[DEBUG] ", "[TRACE] "};

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
    if(ret_LU <= 0) {
        // Handle strftime error
        return;
    }
    // Print the log level
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

    va_list args;
    va_start(args, fmt);

    // Print the formatted string
    ret = vfprintf(stderr, fmt, args);
    if(ret < 0) {
        // Handle vfprintf error
        return;
    }

    va_end(args);

    // Print newline
    ret = fprintf(stderr, "\n");
    if(ret < 0) {
        // Handle fprintf error
        return;
    }
}
