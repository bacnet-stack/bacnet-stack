/**
 * @file
 * @brief API for a static buffer library for deeply embedded system.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2005
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_SYS_SBUF_H
#define BACNET_SYS_SBUF_H
#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

struct static_buffer_t {
    char *data; /* block of memory or array of data */
    unsigned size;      /* actual size, in bytes, of the block of data */
    unsigned count;     /* number of bytes in use */
};
typedef struct static_buffer_t STATIC_BUFFER;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    void sbuf_init(
        STATIC_BUFFER * b,      /* static buffer structure */
        char *data,     /* data block */
        unsigned size); /* actual size, in bytes, of the data block or array of data */

    /* returns true if size==0, false if size > 0 */
    BACNET_STACK_EXPORT
    bool sbuf_empty(
        STATIC_BUFFER const *b);
    /* returns the data block, or NULL if not initialized */
    BACNET_STACK_EXPORT
    char *sbuf_data(
        STATIC_BUFFER const *b);
    /* returns the max size of the data block */
    BACNET_STACK_EXPORT
    unsigned sbuf_size(
        STATIC_BUFFER * b);
    /* returns the number of bytes used in the data block */
    BACNET_STACK_EXPORT
    unsigned sbuf_count(
        STATIC_BUFFER * b);
    /* returns true if successful, false if not enough room to append data */
    BACNET_STACK_EXPORT
    bool sbuf_put(
        STATIC_BUFFER * b,      /* static buffer structure */
        unsigned offset,        /* where to start */
        char *data,     /* data to add */
        unsigned data_size);    /* how many to add */
    /* returns true if successful, false if not enough room to append data */
    BACNET_STACK_EXPORT
    bool sbuf_append(
        STATIC_BUFFER * b,      /* static buffer structure */
        char *data,     /* data to append */
        unsigned data_size);    /* how many to append */
    /* returns true if successful, false if count is bigger than size */
    BACNET_STACK_EXPORT
    bool sbuf_truncate(
        STATIC_BUFFER * b,      /* static buffer structure */
        unsigned count);        /* new number of bytes used in buffer */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
