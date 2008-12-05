/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2008 by Steve Karg

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

/* Functional Description: Generic FIFO library for deeply
   embedded system. See the unit tests for usage examples.
   This library only uses a byte sized chunk.
   This library is designed for use in Interrupt Service Routines
   and so is declared as "static inline" */

#ifndef FIFO_H
#define FIFO_H

#include <stdint.h>
#include <stdbool.h>

struct fifo_buffer_t {
    volatile unsigned head;      /* first byte of data */
    volatile unsigned tail;     /* last byte of data */
    volatile uint8_t *buffer; /* block of memory or array of data */
    unsigned buffer_len;     /* length of the data */
};
typedef struct fifo_buffer_t FIFO_BUFFER;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

bool FIFO_Empty(
        FIFO_BUFFER const *b);

uint8_t FIFO_Peek(
        FIFO_BUFFER const *b);

uint8_t FIFO_Get(
        FIFO_BUFFER * b);

bool FIFO_Put(
        FIFO_BUFFER * b,
        uint8_t data_byte);

bool FIFO_Add(
        FIFO_BUFFER * b,
        uint8_t *data_bytes,
        unsigned count);

void FIFO_Flush(
        FIFO_BUFFER * b);

/* note: buffer_len must be a power of two */
void FIFO_Init(
        FIFO_BUFFER * b,
        volatile uint8_t *buffer,
        unsigned buffer_len);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
