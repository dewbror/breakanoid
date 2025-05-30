#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "error/error.h"
#include "logger.h"

error_t error_init(const error_src_t src, const int code, const char* fmt, ...)
{
    va_list args;

    // Get the size of the array we need to allocate
    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    if(len < 0) {
        // Handle vsnprintf error
        LOG_ERROR("%s: len < 0", __func__);
        return (error_t){NULL, src, code};
    }

    char* msg = (char*)malloc((size_t)len + 1);
    if(msg == NULL) {
        // Handle malloc error
        LOG_ERROR("%s: Failed to allocate memory of size %lu", __func__, (size_t)len + 1);
        return (error_t){NULL, src, code};
    }

    va_start(args, fmt);
    int ret = vsnprintf(msg, (size_t)len + 1, fmt, args);
    va_end(args);

    if(ret < 0) {
        // Handle vsnprintf error
        LOG_ERROR("%s: ret < 0", __func__);
        free(msg);
        msg = NULL;
        return (error_t){NULL, src, code};
    }

    // NULL terminate msg
    msg[len] = '\0';

    return (error_t){msg, src, code};
}

void error_destroy(error_t* p_err)
{
    if(p_err == NULL) {
        LOG_ERROR("%s: p_err is NULL", __func__);
        return;
    }

    if(p_err->msg == NULL) {
        LOG_ERROR("%s: msg is NULL", __func__);
        return;
    }

    free(p_err->msg);
    p_err->msg = NULL;
}
