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

/* Functional Description: Static buffer library for deeply
   embedded system. See the unit tests for usage examples. */

#ifndef SBUF_H
#define SBUF_H

#include <stdint.h>
#include <stdbool.h>

struct static_buffer_t
{
  char *data; /* block of memory or array of data */
  unsigned size; /* actual size, in bytes, of the block of data */
  unsigned count; /* number of bytes in use */
};
typedef struct static_buffer_t STATIC_BUFFER;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void sbuf_init(
  STATIC_BUFFER *b, /* static buffer structure */
  char *data, /* actual size, in bytes, of the data block or array of data */
  unsigned size); /* number of bytes used */
/* returns true if size==0, false if size > 0 */
bool sbuf_empty(STATIC_BUFFER const *b);
char *sbuf_data(STATIC_BUFFER const *b);
unsigned sbuf_size(STATIC_BUFFER *b);
unsigned sbuf_count(STATIC_BUFFER *b);
/* returns true if successful, false if not enough room to append data */
bool sbuf_put(
  STATIC_BUFFER *b, /* static buffer structure */
  unsigned offset, /* where to start */
  char *data, /* number of bytes used */
  unsigned data_size); /* how many to add */
/* returns true if successful, false if not enough room to append data */
bool sbuf_append(
  STATIC_BUFFER *b, /* static buffer structure */
  char *data, /* number of bytes used */
  unsigned data_size); /* how many to add */
/* returns true if successful, false if not enough room to append data */
bool sbuf_truncate(
  STATIC_BUFFER *b, /* static buffer structure */
  unsigned count); /* total number of bytes in use */
  
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
