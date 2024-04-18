/**************************************************************************
*
* Copyright (C) 2012 Steve Karg <skarg@users.sourceforge.net>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*********************************************************************/
#ifndef DEBUG_H
#define DEBUG_H

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
    void debug_printf(
        const char *format,
        ...);
    BACNET_STACK_EXPORT
    int debug_aprintf(
        const char *format,
        ...);
    BACNET_STACK_EXPORT
    int debug_fprintf(
        FILE *stream,
        const char *format,
        ...);
    BACNET_STACK_EXPORT
    void debug_perror(
        const char *format,
        ...);
    BACNET_STACK_EXPORT
    void debug_printf_hex(
        uint32_t offset,
        const uint8_t *buffer,
        size_t buffer_length,
        const char *format, ...);

    BACNET_STACK_EXPORT
    void debug_printf_disabled(
        const char *format,
        ...);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
