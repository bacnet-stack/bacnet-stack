/**
 * @file
 * @brief Debug print function
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2008
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h> /* for standard integer types uint8_t etc. */
#include <stdbool.h> /* for the standard bool type. */
#include <stdarg.h>
#if PRINT_ENABLED || DEBUG_ENABLED
#include <stdio.h> /* Standard I/O */
#include <stdlib.h> /* Standard Library */
#include <errno.h>
#endif
#if DEBUG_ENABLED
#include <string.h>
#include <ctype.h>
#endif
#include "bacnet/basic/sys/debug.h"
#if DEBUG_PRINTF_WITH_TIMESTAMP
#include "bacnet/datetime.h"
#endif

#if DEBUG_PRINTF_WITH_TIMESTAMP
/**
 * @brief Print timestamp with a printf string
 * @param format - printf format string
 * @param ... - variable arguments
 * @note This function is only available if
 *  DEBUG_PRINTF_WITH_TIMESTAMP is non-zero
 *  and DEBUG_ENABLED is non-zero
 */
void debug_printf(const char *format, ...)
{
#if DEBUG_ENABLED
    va_list ap;
    char stamp_str[64];
    char buf[1024];
    BACNET_DATE date;
    BACNET_TIME time;
    datetime_local(&date, &time, NULL, NULL);
    snprintf(
        stamp_str, sizeof(stamp_str), "[%02d:%02d:%02d.%03d]: ", time.hour,
        time.min, time.sec, time.hundredths * 10);
    va_start(ap, format);
    vsprintf(buf, format, ap);
    va_end(ap);
    printf("%s%s", stamp_str, buf);
    fflush(stdout);
#else
    (void)format;
#endif
}
#else
/**
 * @brief Print with a printf string
 * @param format - printf format string
 * @param ... - variable arguments
 * @note This function is only available if
 * DEBUG_ENABLED is non-zero
 */
void debug_printf(const char *format, ...)
{
#if DEBUG_ENABLED
    va_list ap;

    va_start(ap, format);
    vfprintf(stdout, format, ap);
    va_end(ap);
    fflush(stdout);
#else
    (void)format;
#endif
}
#endif

/**
 * @brief Print with a printf string that does nothing
 * @param format - printf format string
 * @param ... - variable arguments
 * @note useful when used with defines such as DEBUG_PRINTF
 */
void debug_printf_disabled(const char *format, ...)
{
    (void)format;
}

/**
 * @brief print format with HEX dump of a buffer
 * @param offset - starting address to print to the left side
 * @param buffer - buffer from which to print hex from
 * @param buffer_length - number of bytes from the buffer to print
 * @param format - printf format string
 * @param ... - variable arguments
 * @note This function is only available if DEBUG_ENABLED is non-zero
 */
void debug_printf_hex(
    uint32_t offset,
    const uint8_t *buffer,
    size_t buffer_length,
    const char *format,
    ...)
{
#if DEBUG_ENABLED
    size_t i = 0;
    bool new_line = true;
    char line[16 + 1] = { 0 };
    size_t remainder = 0;
    va_list ap;

    va_start(ap, format);
    vfprintf(stdout, format, ap);
    va_end(ap);
    /* print the buffer after the formatted text */
    if (buffer && buffer_length) {
        for (i = 0; i < buffer_length; i++) {
            if (new_line) {
                new_line = false;
                printf("%08x  ", (unsigned int)(offset + i));
                memset(line, '.', sizeof(line) - 1);
            }
            printf("%02x ", buffer[i]);
            if (isprint(buffer[i])) {
                line[i % 16] = buffer[i];
            }
            if ((i != 0) && (!((i + 1) % 16))) {
                printf(" %s\n", line);
                new_line = true;
            }
        }
        remainder = buffer_length % 16;
        if (remainder) {
            for (i = 0; i < (16 - remainder); i++) {
                printf("   ");
            }
            printf(" %s\n", line);
        }
    }
    fflush(stdout);
#else
    (void)offset;
    (void)buffer;
    (void)buffer_length;
    (void)format;
#endif
}

/**
 * @brief Print with a printf string
 * @param format - printf format string
 * @param ... - variable arguments
 * @note This function is only available if
 * PRINT_ENABLED is non-zero
 * @return number of characters printed
 */
int debug_printf_stdout(const char *format, ...)
{
    int length = 0;
#if PRINT_ENABLED
    va_list ap;

    va_start(ap, format);
    length = vfprintf(stdout, format, ap);
    va_end(ap);
    fflush(stdout);
#else
    (void)format;
#endif
    return length;
}

/**
 * @brief Print with a printf string
 * @param stream - file stream to print to
 * @param format - printf format string
 * @param ... - variable arguments
 * @note This function is only available if
 * PRINT_ENABLED is non-zero
 * @return number of characters printed
 */
int debug_fprintf(FILE *stream, const char *format, ...)
{
    int length = 0;
#if PRINT_ENABLED
    va_list ap;

    va_start(ap, format);
    length = vfprintf(stream, format, ap);
    va_end(ap);
    fflush(stream);
#else
    (void)stream;
    (void)format;
#endif
    return length;
}

/**
 * @brief Print with a fprintf string that does nothing
 * @param format - printf format string
 * @param ... - variable arguments
 * @note useful when used with defines such as DEBUG_PRINTF
 */
int debug_fprintf_disabled(FILE *stream, const char *format, ...)
{
    (void)stream;
    (void)format;
    return 0;
}

/**
 * @brief debug printf to stderr when PRINT_ENABLED is non-zero
 * @param format - printf format string
 * @param ... - variable arguments
 * @note This function is only available if PRINT_ENABLED is non-zero
 */
void debug_printf_stderr(const char *format, ...)
{
#if PRINT_ENABLED
    va_list ap;

    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    fflush(stderr);
#else
    (void)format;
#endif
}

#if PRINT_ENABLED
/**
 * @brief Prints a textual description of the error code currently
 *  stored in the system variable errno to stderr.
 * @param s - pointer to a null-terminated string with explanatory message
 * @note This function is only available if PRINT_ENABLED is non-zero
 */
void debug_perror(const char *message)
{
    perror(message);
    fflush(stderr);
}
#endif
