#ifndef TYPES_H_
#define TYPES_H_
#pragma once

/**
 * types.h
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// C2x/C23 or later
#if (defined(__STDC__) &&         \
     defined(__STDC_VERSION__) && \
     (__STDC_VERSION__ >= 202000L))
#include <stddef.h> // nullptr_t
// pre C23, pre C++11 or non-standard
#else
  #define nullptr (void*)0
  typedef void* nullptr_t;
#endif

#if (defined(__STDC__) &&          \
     defined(__STDC_VERSION__) &&  \
     (__STDC_VERSION__ >= 199901L) )
#include <stdbool.h>
#else
// Pre-C99, define true and false manually
typedef enum { false = 0, true = 1 } bool;
#endif
#endif // TYPES_H_
