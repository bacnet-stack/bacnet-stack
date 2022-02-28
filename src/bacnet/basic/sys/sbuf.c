/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2005 by Steve Karg

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
 Boston, MA  02111-1307
 USA.

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

/** @file sbuf.c  Static buffer library for deeply embedded system. */

/* Functional Description: Static buffer library for deeply
   embedded system. See the unit tests for usage examples. */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "sbuf.h"

void sbuf_init(STATIC_BUFFER *b, /* static buffer structure */
    char *data, /* data block */
    unsigned size)
{ /* actual size, in bytes, of the data block or array of data */
    if (b) {
        b->data = data;
        b->size = size;
        b->count = 0;
    }

    return;
}

/* returns true if count==0, false if count > 0 */
bool sbuf_empty(STATIC_BUFFER const *b)
{
    return (b ? (b->count == 0) : false);
}

char *sbuf_data(STATIC_BUFFER const *b)
{
    return (b ? b->data : NULL);
}

unsigned sbuf_size(STATIC_BUFFER *b)
{
    return (b ? b->size : 0);
}

unsigned sbuf_count(STATIC_BUFFER *b)
{
    return (b ? b->count : 0);
}

/* returns true if successful, false if not enough room to append data */
bool sbuf_put(STATIC_BUFFER *b, /* static buffer structure */
    unsigned offset, /* where to start */
    char *data, /* data to place in buffer */
    unsigned data_size)
{ /* how many bytes to add */
    bool status = false; /* return value */

    if (b && b->data) {
        if (((offset + data_size) < b->size)) {
            b->count = offset + data_size;
            while (data_size) {
                b->data[offset] = *data;
                offset++;
                data++;
                data_size--;
            }
            status = true;
        }
    }

    return status;
}

/* returns true if successful, false if not enough room to append data */
bool sbuf_append(STATIC_BUFFER *b, /* static buffer structure */
    char *data, /* data to place in buffer */
    unsigned data_size)
{ /* how many bytes to add */
    unsigned count = 0;

    if (b) {
        count = b->count;
    }

    return sbuf_put(b, count, data, data_size);
}

/* returns true if successful, false if not enough room to append data */
bool sbuf_truncate(STATIC_BUFFER *b, /* static buffer structure */
    unsigned count)
{ /* total number of bytes in to remove */
    bool status = false; /* return value */

    if (b) {
        if (count < b->size) {
            b->count = count;
            status = true;
        }
    }

    return status;
}
