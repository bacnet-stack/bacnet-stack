/**
 * @file
 * @author Steve Karg
 * @date 2022
 * @brief Platform libc and compiler abstraction layer 
  *
 * @section DESCRIPTION
 *
 * This libc and compiler abstraction layer assists with differences
 * between compiler and libc versions, capabilities, and standards.
 *
 * @section LICENSE
 *
 * Copyright (C) 2022 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
*/
#ifndef BACNET_SYS_PLATFORM_H
#define BACNET_SYS_PLATFORM_H

#include <math.h>

#ifndef islessgreater
#define islessgreater(x, y) ((x) < (y) || (x) > (y))
#endif

/* marking some code as 'deprecated' */
# if defined(_MSC_VER)
#   define BACNET_STACK_DEPRECATED(message) __declspec(deprecated(message))
#elif defined(__GNUC__)
#   define BACNET_STACK_DEPRECATED(message) __attribute__((deprecated(message)))
# else
#   define BACNET_STACK_DEPRECATED(message)
# endif

# if defined(WIN32) || defined(WIN64)
#  define strcasecmp _stricmp
#elif defined(__ZEPHYR__)
#  include <strings.h>
# endif

#if defined(__MINGW32__)
#define BACNET_STACK_FALLTHROUGH() /* fall through */
#elif defined(__GNUC__)
#define BACNET_STACK_FALLTHROUGH() __attribute__ ((fallthrough))
#else 
#define BACNET_STACK_FALLTHROUGH() /* fall through */
#endif

#endif
