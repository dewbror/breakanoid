#ifndef STRBOOL_H_
#define STRBOOL_H_

#include <stdbool.h>

static inline const char* strbool(bool b) {
    return b ? "true" : "false";
}

#endif // STRBOOL_H_
