#ifndef LOGGER_H_
#define LOGGER_H_

#include "config.h"

#define LOG_LEVEL_ERROR 0
#define LOG_LEVEL_WARN  1
#define LOG_LEVEL_INFO  2
#define LOG_LEVEL_DEBUG 3
#define LOG_LEVEL_TRACE 4

/**
 * Open file file_name for logging. If file_name = NULL will set logging to stderr.
 *
 * \param[in] file_name A string with the name of the log file to open.
 */
void logger_open(const char* file_name);

/**
 * Close log file.
 */
void logger_close(void);

/**
 * NOT MEANT TO BE USED, USE LOG_ERROR, _INFO, etc. instead.
 *
 * \param[in] level The log_level of the message.
 * \param[in] fmt A printf-style message format string.
 * \param[in] ... Additional parameters matching % tokens in the "fmt" string, if any.
 */
void logger__msg(int level, const char* file, int line, const char* fmt, ...) FORMAT_ATTR(4, 5);

// If LOG_LEVEL is not defined, use default LOG_LEVEL
#ifndef LOG_LEVEL
#ifdef NDEBUG
// Default LOG_LEVEL for NDEBUG mode is LOG_WARN
#define LOG_LEVEL LOG_LEVEL_INFO
#else
// Default LOG_LEVEL in debug mode is LOG_DEBUG
#define LOG_LEVEL LOG_LEVEL_TRACE
#endif
#endif

#if LOG_LEVEL >= LOG_LEVEL_ERROR
#define LOG_ERROR(...) logger__msg(LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__);
#else
#define LOG_ERROR(...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_WARN
#define LOG_WARN(...) logger__msg(LOG_LEVEL_WARN, __FILE__, __LINE__, __VA_ARGS__);
#else
#define LOG_WARN(...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_INFO
#define LOG_INFO(...) logger__msg(LOG_LEVEL_INFO, __FILE__, __LINE__, __VA_ARGS__);
#else
#define LOG_INFO(...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
#define LOG_DEBUG(...) logger__msg(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__);
#else
#define LOG_DEBUG(...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_TRACE
#define LOG_TRACE(...) logger__msg(LOG_LEVEL_TRACE, __FILE__, __LINE__, __VA_ARGS__);
#else
#define LOG_TRACE(...) ((void)0)
#endif

#endif // LOGGER_H_
