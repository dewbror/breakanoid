/*
  logger.h
*/

#ifndef LOGGER_H_
#define LOGGER_H_
#pragma once

#include "config.h"

typedef enum {
    LOG_ERROR = 0, // ERROR should always result in application crash
    LOG_WARN  = 1, // WARN are errors that dont need to crash the application.
    LOG_INFO  = 2, // INFO is high level information about what the applciaiton is currently doing.
    LOG_DEBUG = 3, // DEBUG is low level information about what the application is doing.
    LOG_TRACE = 4  // TRACE is very low and very verbose information of what the application is doing.
} log_level;

/**
 * Open log file.
 */
void open_log_file(void);

/**
 * Close log file.
 */
void close_log_file(void);

/**
 * NOT MEANT TO BE USED, USE LOG_ERROR, _INFO, etc. instead.
 *
 * \param[in] level The log_level of the message.
 * \param[in] fmt A printf-style message format string.
 * \param[in] ... Additional parameters matching % tokens in the "fmt" string, if any.
 */
void logger__msg(log_level level, const char* file, int line, const char* fmt, ...) FORMAT_ATTR(4, 5);

// If LOG_LEVEL is not defined, use default LOG_LEVEL
#ifndef LOG_LEVEL
#ifdef NDEBUG
// Default LOG_LEVEL for NDEBUG mode is LOG_WARN
#define LOG_LEVEL 2
#else
// Default LOG_LEVEL in debug mode is LOG_DEBUG
#define LOG_LEVEL 3
#endif
#endif

#define LOG_ERROR(...)                                                                                                 \
    do {                                                                                                               \
        if(LOG_LEVEL >= LOG_ERROR)                                                                                     \
            logger__msg(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__);                                                   \
    } while(0)
#define LOG_WARN(...)                                                                                                  \
    do {                                                                                                               \
        if(LOG_LEVEL >= LOG_WARN)                                                                                      \
            logger__msg(LOG_WARN, __FILE__, __LINE__, __VA_ARGS__);                                                    \
    } while(0)
#define LOG_INFO(...)                                                                                                  \
    do {                                                                                                               \
        if(LOG_LEVEL >= LOG_INFO)                                                                                      \
            logger__msg(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__);                                                    \
    } while(0)
#define LOG_DEBUG(...)                                                                                                 \
    do {                                                                                                               \
        if(LOG_LEVEL >= LOG_DEBUG)                                                                                     \
            logger__msg(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__);                                                   \
    } while(0)
#define LOG_TRACE(...)                                                                                                 \
    do {                                                                                                               \
        if(LOG_LEVEL >= LOG_TRACE)                                                                                     \
            logger__msg(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__);                                                   \
    } while(0)
#endif // LOGGER_H_
