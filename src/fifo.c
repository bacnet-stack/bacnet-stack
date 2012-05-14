/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2004 by Steve Karg

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

/** @file fifo.c  Generic FIFO library for deeply embedded system */

/* Functional Description: Generic FIFO library for deeply
   embedded system. See the unit tests for usage examples. */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "fifo.h"

/****************************************************************************
* DESCRIPTION: Returns the number of elements in the ring buffer
* RETURN:      Number of elements in the ring buffer
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
unsigned FIFO_Count(
    FIFO_BUFFER const *b)
{
    unsigned head, tail;        /* used to avoid volatile decision */

    if (b) {
        head = b->head;
        tail = b->tail;
        return head - tail;
    } else {
        return 0;
    }
}

/****************************************************************************
* DESCRIPTION: Returns the empty/full status of the ring buffer
* RETURN:      true if the ring buffer is full, false if it is not.
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
bool FIFO_Full(
    FIFO_BUFFER const *b)
{
    return (b ? (FIFO_Count(b) == b->buffer_len) : true);
}

/****************************************************************************
* DESCRIPTION: Tests to see if space is available
* RETURN:      true if the number of bytes is available
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
bool FIFO_Available(
    FIFO_BUFFER const *b,
    unsigned count)
{
    return (b ? (count <= (b->buffer_len - FIFO_Count(b))) : false);
}

/****************************************************************************
* DESCRIPTION: Returns the empty/full status of the ring buffer
* RETURN:      true if the ring buffer is empty, false if it is not.
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
bool FIFO_Empty(
    FIFO_BUFFER const *b)
{
    return (b ? (FIFO_Count(b) == 0) : true);
}

/****************************************************************************
* DESCRIPTION: Looks at the data from the head of the list without removing it
* RETURN:      byte of data, or zero if nothing in the list
* ALGORITHM:   none
* NOTES:       Use Empty function to see if there is data to retrieve
*****************************************************************************/
uint8_t FIFO_Peek(
    FIFO_BUFFER const *b)
{
    if (b) {
        return (b->buffer[b->tail % b->buffer_len]);
    }

    return 0;
}

/****************************************************************************
* DESCRIPTION: Gets the data from the front of the list, and removes it
* RETURN:      the data, or zero if nothing in the list
* ALGORITHM:   none
* NOTES:       Use Empty function to see if there is data to retrieve
*****************************************************************************/
uint8_t FIFO_Get(
    FIFO_BUFFER * b)
{
    uint8_t data_byte = 0;

    if (!FIFO_Empty(b)) {
        data_byte = b->buffer[b->tail % b->buffer_len];
        b->tail++;
    }
    return data_byte;
}

/****************************************************************************
* DESCRIPTION: Adds an element of data to the FIFO
* RETURN:      true on successful add, false if not added
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
bool FIFO_Put(
    FIFO_BUFFER * b,
    uint8_t data_byte)
{
    bool status = false;        /* return value */

    if (b) {
        /* limit the ring to prevent overwriting */
        if (!FIFO_Full(b)) {
            b->buffer[b->head % b->buffer_len] = data_byte;
            b->head++;
            status = true;
        }
    }

    return status;
}

/****************************************************************************
* DESCRIPTION: Adds one or more elements of data to the FIFO
* RETURN:      true if space available and added, false if not added
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
bool FIFO_Add(
    FIFO_BUFFER * b,
    uint8_t * data_bytes,
    unsigned count)
{
    bool status = false;        /* return value */

    /* limit the ring to prevent overwriting */
    if (FIFO_Available(b, count)) {
        while (count) {
            b->buffer[b->head % b->buffer_len] = *data_bytes;
            b->head++;
            data_bytes++;
            count--;
        }
        status = true;
    }

    return status;
}

/****************************************************************************
* DESCRIPTION: Flushes any data in the buffer
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
void FIFO_Flush(
    FIFO_BUFFER * b)
{
    unsigned head;      /* used to avoid volatile decision */

    if (b) {
        head = b->head;
        b->tail = head;
    }
}

/****************************************************************************
* DESCRIPTION: Configures the ring buffer
* RETURN:      none
* ALGORITHM:   none
* NOTES:        buffer_len must be a power of two
*****************************************************************************/
void FIFO_Init(
    FIFO_BUFFER * b,
    volatile uint8_t * buffer,
    unsigned buffer_len)
{
    if (b) {
        b->head = 0;
        b->tail = 0;
        b->buffer = buffer;
        b->buffer_len = buffer_len;
    }

    return;
}

#ifdef TEST
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "ctest.h"

/* test the FIFO */
/* note: must be a power of two! */
#define FIFO_BUFFER_SIZE 64
void testFIFOBuffer(
    Test * pTest)
{
    FIFO_BUFFER test_buffer = { 0 };
    volatile uint8_t data_store[FIFO_BUFFER_SIZE] = { 0 };
    uint8_t add_data[40] = { "RoseSteveLouPatRachelJessicaDaniAmyHerb" };
    uint8_t test_add_data[40] = { 0 };
    uint8_t test_data = 0;
    unsigned index = 0;
    unsigned count = 0;
    bool status = 0;

    FIFO_Init(&test_buffer, data_store, sizeof(data_store));
    ct_test(pTest, FIFO_Empty(&test_buffer));

    /* load the buffer */
    for (test_data = 0; test_data < FIFO_BUFFER_SIZE; test_data++) {
        ct_test(pTest, !FIFO_Full(&test_buffer));
        ct_test(pTest, FIFO_Available(&test_buffer, 1));
        status = FIFO_Put(&test_buffer, test_data);
        ct_test(pTest, status == true);
        ct_test(pTest, !FIFO_Empty(&test_buffer));
    }
    /* not able to put any more */
    ct_test(pTest, FIFO_Full(&test_buffer));
    ct_test(pTest, !FIFO_Available(&test_buffer, 1));
    status = FIFO_Put(&test_buffer, 42);
    ct_test(pTest, status == false);
    /* unload the buffer */
    for (index = 0; index < FIFO_BUFFER_SIZE; index++) {
        ct_test(pTest, !FIFO_Empty(&test_buffer));
        test_data = FIFO_Peek(&test_buffer);
        ct_test(pTest, test_data == index);
        test_data = FIFO_Get(&test_buffer);
        ct_test(pTest, test_data == index);
        ct_test(pTest, FIFO_Available(&test_buffer, 1));
        ct_test(pTest, !FIFO_Full(&test_buffer));
    }
    ct_test(pTest, FIFO_Empty(&test_buffer));
    test_data = FIFO_Get(&test_buffer);
    ct_test(pTest, test_data == 0);
    test_data = FIFO_Peek(&test_buffer);
    ct_test(pTest, test_data == 0);
    ct_test(pTest, FIFO_Empty(&test_buffer));
    /* test the ring around the buffer */
    for (index = 0; index < FIFO_BUFFER_SIZE; index++) {
        ct_test(pTest, FIFO_Empty(&test_buffer));
        ct_test(pTest, FIFO_Available(&test_buffer, 4));
        for (count = 1; count < 4; count++) {
            test_data = count;
            status = FIFO_Put(&test_buffer, test_data);
            ct_test(pTest, status == true);
            ct_test(pTest, !FIFO_Empty(&test_buffer));
        }
        for (count = 1; count < 4; count++) {
            ct_test(pTest, !FIFO_Empty(&test_buffer));
            test_data = FIFO_Peek(&test_buffer);
            ct_test(pTest, test_data == count);
            test_data = FIFO_Get(&test_buffer);
            ct_test(pTest, test_data == count);
        }
    }
    ct_test(pTest, FIFO_Empty(&test_buffer));
    /* test Add */
    ct_test(pTest, FIFO_Available(&test_buffer, sizeof(add_data)));
    status = FIFO_Add(&test_buffer, add_data, sizeof(add_data));
    ct_test(pTest, status == true);
    count = FIFO_Count(&test_buffer);
    ct_test(pTest, count == sizeof(add_data));
    ct_test(pTest, !FIFO_Empty(&test_buffer));
    for (index = 0; index < sizeof(add_data); index++) {
        /* unload the buffer */
        ct_test(pTest, !FIFO_Empty(&test_buffer));
        test_data = FIFO_Peek(&test_buffer);
        ct_test(pTest, test_data == add_data[index]);
        test_data = FIFO_Get(&test_buffer);
        ct_test(pTest, test_data == add_data[index]);
    }
    ct_test(pTest, FIFO_Empty(&test_buffer));
    /* test flush */
    status = FIFO_Add(&test_buffer, test_add_data, sizeof(test_add_data));
    ct_test(pTest, status == true);
    ct_test(pTest, !FIFO_Empty(&test_buffer));
    FIFO_Flush(&test_buffer);
    ct_test(pTest, FIFO_Empty(&test_buffer));

    return;
}

#ifdef TEST_FIFO_BUFFER
int main(
    void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("FIFO Buffer", NULL);

    /* individual tests */
    rc = ct_addTestFunction(pTest, testFIFOBuffer);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);

    ct_destroy(pTest);

    return 0;
}
#endif
#endif
