/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2022
 * @brief Platform libc and compiler abstraction layer
 * @details This libc and compiler abstraction layer assists with differences
 * between compiler and libc versions, capabilities, and C standards.
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_SYS_PLATFORM_H
#define BACNET_SYS_PLATFORM_H
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#ifndef INT_MAX
#define INT_MAX (~0U >> 1U)
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

/* marking some code as 'deprecated' */
#if defined(BACNET_STACK_DEPRECATED_DISABLE)
#define BACNET_STACK_DEPRECATED(message)
#elif defined(_MSC_VER)
#define BACNET_STACK_DEPRECATED(message) __declspec(deprecated(message))
#elif defined(__GNUC__)
#define BACNET_STACK_DEPRECATED(message) __attribute__((deprecated(message)))
#else
#define BACNET_STACK_DEPRECATED(message)
#endif

#if defined(_MSC_VER)
#ifndef __inline__
#define __inline__ __inline
#endif
#if (_MSC_VER < 1900)
#include <stdio.h>
#include <stdarg.h>
#define snprintf c99_snprintf
#define vsnprintf c99_vsnprintf

__inline int
c99_vsnprintf(char *outBuf, size_t size, const char *format, va_list ap)
{
    int count = -1;

    if (size != 0) {
        count = _vsnprintf_s(outBuf, size, _TRUNCATE, format, ap);
    }
    if (count == -1) {
        count = _vscprintf(format, ap);
    }

    return count;
}

__inline int c99_snprintf(char *outBuf, size_t size, const char *format, ...)
{
    int count;
    va_list ap;

    va_start(ap, format);
    count = c99_vsnprintf(outBuf, size, format, ap);
    va_end(ap);

    return count;
}
#endif
#endif

/* some common min/max as defined in windef.h */
#ifndef NOMINMAX
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#endif /* NOMINMAX */

#if defined(__MINGW32__)
#define BACNET_STACK_FALLTHROUGH() /* fall through */
#elif defined(__GNUC__)
#define BACNET_STACK_FALLTHROUGH() __attribute__((fallthrough))
#else
#define BACNET_STACK_FALLTHROUGH() /* fall through */
#endif

#endif
