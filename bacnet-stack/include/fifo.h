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
#ifndef FIFO_H
#define FIFO_H

/* Functional Description: Generic FIFO library for deeply
   embedded system. See the unit tests for usage examples.
   This library only uses a byte sized chunk.
   This library is designed for use in Interrupt Service Routines
   and so is declared as "static inline" */

#include <stdint.h>
#include <stdbool.h>

struct fifo_buffer_t {
    volatile unsigned head;     /* first byte of data */
    volatile unsigned tail;     /* last byte of data */
    volatile uint8_t *buffer;   /* block of memory or array of data */
    unsigned buffer_len;        /* length of the data */
};
typedef struct fifo_buffer_t FIFO_BUFFER;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    unsigned FIFO_Count(
        FIFO_BUFFER const *b);

    bool FIFO_Full(
        FIFO_BUFFER const *b);

    bool FIFO_Available (
        FIFO_BUFFER const *b,
        unsigned count);

    bool FIFO_Empty(
        FIFO_BUFFER const *b);

    uint8_t FIFO_Peek(
        FIFO_BUFFER const *b);

    uint8_t FIFO_Get(
        FIFO_BUFFER * b);

    unsigned FIFO_Pull(
        FIFO_BUFFER * b,
        uint8_t * data_bytes,
        unsigned length);

    bool FIFO_Put(
        FIFO_BUFFER * b,
        uint8_t data_byte);

    bool FIFO_Add(
        FIFO_BUFFER * b,
        uint8_t * data_bytes,
        unsigned count);

    void FIFO_Flush(
        FIFO_BUFFER * b);

/* note: buffer_len must be a power of two */
    void FIFO_Init(
        FIFO_BUFFER * b,
        volatile uint8_t * buffer,
        unsigned buffer_len);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
