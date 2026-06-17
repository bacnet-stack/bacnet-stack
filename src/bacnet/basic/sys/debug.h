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

/*
 * Each message Priority also has a decimal Severity level indicator.
 * These are described in the following table along with their numerical
 * values.  Severity values MUST be in the range of 0 to 7 inclusive,
 * or a special value of -1 to indicate that logging is disabled.
 */
#define DEBUG_LOG_EMERGENCY 0 /* system is unusable */
#define DEBUG_LOG_ALERT 1 /* action must be taken immediately */
#define DEBUG_LOG_CRITICAL 2 /* critical conditions */
#define DEBUG_LOG_ERROR 3 /* error conditions */
#define DEBUG_LOG_WARNING 4 /* warning conditions */
#define DEBUG_LOG_NOTICE 5 /* normal but significant condition */
#define DEBUG_LOG_INFO 6 /* informational */
#define DEBUG_LOG_DEBUG 7 /* debug-level messages */
#define DEBUG_LOG_PRIMASK 0x07 /* mask to extract priority part (internal) */
#define DEBUG_LOG_PRIORITY(p) ((p) & DEBUG_LOG_PRIMASK)
#define DEBUG_LOG_DISABLED -1 /* special 'set' value to indicate no logging */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int debug_log_fprintf(int severity, FILE *stream, const char *format, ...);
BACNET_STACK_EXPORT
int debug_log_fprintf_disabled(
    int severity, FILE *stream, const char *format, ...);
BACNET_STACK_EXPORT
void debug_log_severity_set(int severity);
BACNET_STACK_EXPORT
void debug_log_severity_ascii_set(const char *argv);
BACNET_STACK_EXPORT
int debug_log_severity_get(void);

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
