/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2025
 * @brief Platform libc and compiler abstraction layer
 * @details This libc and compiler abstraction layer assists with differences
 * between compiler and libc versions, capabilities, and C standards.
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef UBASIC_PLATFORM_H
#define UBASIC_PLATFORM_H
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#ifndef INT_MAX
#define INT_MAX (~0U >> 1U)
#endif

#ifndef NOMINMAX
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#endif

#ifndef islessgreater
#define islessgreater(x, y) ((x) < (y) || (x) > (y))
#endif

#ifndef isgreaterequal
#define isgreaterequal(x, y) ((x) > (y) || !islessgreater((x), (y)))
#endif

#ifndef islessequal
#define islessequal(x, y) ((x) < (y) || !islessgreater((x), (y)))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) ((size_t)(sizeof(array) / sizeof((array)[0])))
#endif

#endif
