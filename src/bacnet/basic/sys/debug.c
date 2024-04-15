/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2008 Steve Karg

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to:
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330
 Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/

#include <stdint.h> /* for standard integer types uint8_t etc. */
#include <stdbool.h> /* for the standard bool type. */
#include <stdio.h> /* Standard I/O */
#include <stdlib.h> /* Standard Library */
#include <stdarg.h>
#include "bacnet/basic/sys/debug.h"

/** @file debug.c  Debug print function */

#if DEBUG_ENABLED
void debug_printf(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    vfprintf(stdout, format, ap);
    va_end(ap);
    fflush(stdout);

    return;
}
#else
void debug_printf(const char *format, ...)
{
    (void)format;
}
#endif

#if PRINT_ENABLED
int debug_aprintf(const char *format, ...)
{
    int length = 0;
    va_list ap;

    va_start(ap, format);
    length = vfprintf(stdout, format, ap);
    va_end(ap);
    fflush(stdout);

    return length;
}

int debug_fprintf(FILE *stream, const char *format, ...)
{
    int length = 0;
    va_list ap;

    va_start(ap, format);
    length = vfprintf(stream, format, ap);
    va_end(ap);
    fflush(stream);

    return length;
}

void debug_perror(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    fflush(stderr);
}
#else
int debug_aprintf(const char *format, ...)
{
    (void)format;
    return 0;
}

int debug_fprintf(FILE *stream, const char *format, ...)
{
    (void)stream;
    (void)format;
    return 0;
}

void debug_perror(const char *format, ...)
{
    (void)format;
}
#endif
