/**
 * @file
 * @brief API for memory copy function
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2008
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 * @section DESCRIPTION
 *
 * Memory copy functions for deeply embedded system.  The functions
 * are used with a buffer, the buffer offset, the sizeof the buffer,
 * and the number of bytes to copy to the buffer.
 */
#include <stddef.h>
#include <string.h>
#include "bacnet/memcopy.h"

/**
 * Tests to see if the number of bytes is available from an offset
 * given the maximum sizeof a buffer.
 *
 * @param offset - offset into a buffer
 * @param max - maximum sizeof a buffer
 * @param len - number of bytes to add to the buffer starting at offset.
 *
 * @return True if there is enough space to copy len bytes.
 */
bool memcopylen(size_t offset, size_t max, size_t len)
{
    return ((offset + len) <= max);
}

#if defined(MEMCOPY_SIMPLE)
/**
 * Copy len bytes from src to offset of dest if there is enough space
 * in a buffer of max bytes.
 *
 * @param dest - pointer to the destination buffer
 * @param src - pointer to the source buffer
 * @param offset - offset into the destination buffer
 * @param max - maximum sizeof the destination buffer
 * @param len - number of bytes to add to the destination buffer
 * starting at offset.
 *
 * @return returns zero if there is not enough space, or returns
 * the number of bytes copied.
 */
size_t memcopy(void *dest, void *src, size_t offset, size_t len, size_t max)
{
    size_t i;
    size_t copy_len = 0;
    char *s1, *s2;

    s1 = dest;
    s2 = src;
    if (memcopylen(offset, max, len)) {
        for (i = 0; i < len; i++) {
            s1[offset + i] = s2[i];
            copy_len++;
        }
    }

    return copy_len;
}
#else
/**
 * Copy len bytes from src to offset of dest if there is enough space
 * in a buffer of max bytes.
 *
 * @param dest - pointer to the destination buffer
 * @param src - pointer to the source buffer
 * @param offset - offset into the destination buffer
 * @param max - maximum sizeof the destination buffer
 * @param len - number of bytes to add to the destination buffer
 * starting at offset.
 *
 * @return returns zero if there is not enough space, or returns
 * the number of bytes copied.
 */
size_t memcopy(
    void *dest,
    const void *src,
    size_t offset, /* where in dest to put the data */
    size_t len, /* amount of data to copy */
    size_t max)
{ /* total size of destination */
    if (memcopylen(offset, max, len)) {
        memcpy(&((char *)dest)[offset], src, len);
        return (len);
    }

    return 0;
}
#endif
