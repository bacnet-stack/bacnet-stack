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

/* Functional Description: Memory copy function */

#ifndef MEMCOPY_H
#define MEMCOPY_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* copy len bytes from src to offset of dest if there is enough space. */
/* returns 0 if there is not enough space, or the number of bytes copied. */
size_t memcopy(
    void * dest,
    void * src,
    size_t offset, /* where in dest to put the data */
    size_t len, /* amount of data to copy */
    size_t max); /* total size of destination */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
