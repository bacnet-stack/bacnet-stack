/**
 * @file
 * @brief Static buffer library for deeply embedded system.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2005
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "bacnet/basic/sys/sbuf.h"

void sbuf_init(
    STATIC_BUFFER *b, /* static buffer structure */
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

unsigned sbuf_size(STATIC_BUFFER const *b)
{
    return (b ? b->size : 0);
}

unsigned sbuf_count(STATIC_BUFFER const *b)
{
    return (b ? b->count : 0);
}

/* returns true if successful, false if not enough room to append data */
bool sbuf_put(
    STATIC_BUFFER *b, /* static buffer structure */
    unsigned offset, /* where to start */
    const char *data, /* data to place in buffer */
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
bool sbuf_append(
    STATIC_BUFFER *b, /* static buffer structure */
    const char *data, /* data to place in buffer */
    unsigned data_size)
{ /* how many bytes to add */
    unsigned count = 0;

    if (b) {
        count = b->count;
    }

    return sbuf_put(b, count, data, data_size);
}

/* returns true if successful, false if not enough room to append data */
bool sbuf_truncate(
    STATIC_BUFFER *b, /* static buffer structure */
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
