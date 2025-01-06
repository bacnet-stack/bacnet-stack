/**
 * @file
 * @brief API for debug print function
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2008
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_SYS_DEBUG_H
#define BACNET_SYS_DEBUG_H
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

#ifndef DEBUG_ENABLED
#define DEBUG_ENABLED 0
#endif

#ifndef DEBUG_PRINTF_WITH_TIMESTAMP
#define DEBUG_PRINTF_WITH_TIMESTAMP 0
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void debug_printf(const char *format, ...);
BACNET_STACK_EXPORT
void debug_printf_disabled(const char *format, ...);

BACNET_STACK_EXPORT
int debug_printf_stdout(const char *format, ...);

BACNET_STACK_EXPORT
int debug_fprintf(FILE *stream, const char *format, ...);
BACNET_STACK_EXPORT
int debug_fprintf_disabled(FILE *stream, const char *format, ...);

BACNET_STACK_EXPORT
void debug_printf_stderr(const char *format, ...);

BACNET_STACK_EXPORT
void debug_printf_hex(
    uint32_t offset,
    const uint8_t *buffer,
    size_t buffer_length,
    const char *format,
    ...);

#if PRINT_ENABLED
BACNET_STACK_EXPORT
void debug_print(const char *message);
#else
#define debug_print(message) ((void)0)
#endif

#if PRINT_ENABLED
BACNET_STACK_EXPORT
void debug_perror(const char *message);
#else
#define debug_perror(message) ((void)0)
#endif

#if PRINT_ENABLED
#define DEBUG_FPRINTF debug_fprintf
#else
#define DEBUG_FPRINTF debug_fprintf_disabled
#endif

#if DEBUG_ENABLED
#define DEBUG_PRINTF debug_printf
#else
#define DEBUG_PRINTF debug_printf_disabled
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
