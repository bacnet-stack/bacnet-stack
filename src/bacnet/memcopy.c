/**
 * @file
 * @author Steve Karg
 * @date 2008
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to:
 * The Free Software Foundation, Inc.
 * 59 Temple Place - Suite 330
 * Boston, MA  02111-1307
 * USA.
 *
 * As a special exception, if other files instantiate templates or
 * use macros or inline functions from this file, or you compile
 * this file and link it with other works to produce a work based
 * on this file, this file does not by itself cause the resulting
 * work to be covered by the GNU General Public License. However
 * the source code for this file must still be made available in
 * accordance with section (3) of the GNU General Public License.
 *
 * This exception does not invalidate any other reasons why a work
 * based on this file might be covered by the GNU General Public
 * License.
 *
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
size_t memcopy(void *dest,
    void *src,
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
