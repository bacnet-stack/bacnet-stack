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

/* Functional Description: Generic ring buffer library for deeply
   embedded system. See the unit tests for usage examples. */

#include <stddef.h>
#include <stdint.h>
#include "ringbuf.h"

/****************************************************************************
* DESCRIPTION: Returns the empty/full status of the ring buffer
* RETURN:      true if the ring buffer is empty, false if it is not.
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
bool Ringbuf_Empty(
    RING_BUFFER const *b)
{
    return (b->count == 0);
}

/****************************************************************************
* DESCRIPTION: Looks at the data from the head of the list without removing it
* RETURN:      pointer to the data, or NULL if nothing in the list
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
char *Ringbuf_Get_Front(
    RING_BUFFER const *b)
{
    return (b->count ? &(b->data[b->head * b->element_size]) : NULL);
}

/****************************************************************************
* DESCRIPTION: Gets the data from the front of the list, and removes it
* RETURN:      pointer to the data, or NULL if nothing in the list
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
char *Ringbuf_Pop_Front(
    RING_BUFFER * b)
{
    char *data = NULL;  /* return value */

    if (b->count) {
        data = &(b->data[b->head * b->element_size]);
        b->head++;
        if (b->head >= b->element_count)
            b->head = 0;
        b->count--;
    }

    return data;
}

/****************************************************************************
* DESCRIPTION: Adds an element of data to the ring buffer
* RETURN:      true on succesful add, false if not added
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
bool Ringbuf_Put(
    RING_BUFFER * b,    /* ring buffer structure */
    char *data_element)
{       /* one element to add to the ring */
    bool status = false;        /* return value */
    unsigned offset = 0;        /* offset into array of data */
    char *ring_data = NULL;     /* used to help point ring data */
    unsigned i; /* loop counter */

    if (b && data_element) {
        /* limit the amount of data that we accept */
        if (b->count < b->element_count) {
            offset = b->head + b->count;
            if (offset >= b->element_count)
                offset -= b->element_count;
            ring_data = b->data + offset * b->element_size;
            for (i = 0; i < b->element_size; i++) {
                ring_data[i] = data_element[i];
            }
            b->count++;
            status = true;
        }
    }

    return status;
}

/****************************************************************************
* DESCRIPTION: Configures the ring buffer
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
void Ringbuf_Init(
    RING_BUFFER * b,    /* ring buffer structure */
    char *data, /* data block or array of data */
    unsigned element_size,      /* size of one element in the data block */
    unsigned element_count)
{       /* number of elements in the data block */
    b->head = 0;
    b->count = 0;
    b->data = data;
    b->element_size = element_size;
    b->element_count = element_count;

    return;
}

#ifdef TEST
#include <assert.h>
#include <string.h>

#include "ctest.h"

/* test the FIFO */
#define RING_BUFFER_DATA_SIZE 5
#define RING_BUFFER_COUNT 16
void testRingBuf(
    Test * pTest)
{
    RING_BUFFER test_buffer;
    char data_store[RING_BUFFER_DATA_SIZE * RING_BUFFER_COUNT];
    char data[RING_BUFFER_DATA_SIZE];
    char *test_data;
    unsigned index;
    unsigned data_index;
    unsigned count;
    unsigned dummy;
    bool status;

    Ringbuf_Init(&test_buffer, data_store, RING_BUFFER_DATA_SIZE,
        RING_BUFFER_COUNT);
    ct_test(pTest, Ringbuf_Empty(&test_buffer));

    for (data_index = 0; data_index < RING_BUFFER_DATA_SIZE; data_index++) {
        data[data_index] = data_index;
    }
    status = Ringbuf_Put(&test_buffer, data);
    ct_test(pTest, status == true);
    ct_test(pTest, !Ringbuf_Empty(&test_buffer));

    test_data = Ringbuf_Get_Front(&test_buffer);
    for (data_index = 0; data_index < RING_BUFFER_DATA_SIZE; data_index++) {
        ct_test(pTest, test_data[data_index] == data[data_index]);
    }
    ct_test(pTest, !Ringbuf_Empty(&test_buffer));

    test_data = Ringbuf_Pop_Front(&test_buffer);
    for (data_index = 0; data_index < RING_BUFFER_DATA_SIZE; data_index++) {
        ct_test(pTest, test_data[data_index] == data[data_index]);
    }
    ct_test(pTest, Ringbuf_Empty(&test_buffer));

    /* fill to max */
    for (index = 0; index < RING_BUFFER_COUNT; index++) {
        for (data_index = 0; data_index < RING_BUFFER_DATA_SIZE; data_index++) {
            data[data_index] = index;
        }
        status = Ringbuf_Put(&test_buffer, data);
        ct_test(pTest, status == true);
        ct_test(pTest, !Ringbuf_Empty(&test_buffer));
    }
    /* verify actions on full buffer */
    for (index = 0; index < RING_BUFFER_COUNT; index++) {
        for (data_index = 0; data_index < RING_BUFFER_DATA_SIZE; data_index++) {
            data[data_index] = index;
        }
        status = Ringbuf_Put(&test_buffer, data);
        ct_test(pTest, status == false);
        ct_test(pTest, !Ringbuf_Empty(&test_buffer));
    }

    /* check buffer full */
    for (index = 0; index < RING_BUFFER_COUNT; index++) {
        test_data = Ringbuf_Get_Front(&test_buffer);
        for (data_index = 0; data_index < RING_BUFFER_DATA_SIZE; data_index++) {
            ct_test(pTest, test_data[data_index] == index);
        }

        test_data = Ringbuf_Pop_Front(&test_buffer);
        for (data_index = 0; data_index < RING_BUFFER_DATA_SIZE; data_index++) {
            ct_test(pTest, test_data[data_index] == index);
        }
    }
    ct_test(pTest, Ringbuf_Empty(&test_buffer));

    /* test the ring around the buffer */
    for (index = 0; index < RING_BUFFER_COUNT; index++) {
        for (count = 1; count < 4; count++) {
            dummy = index * count;
            for (data_index = 0; data_index < RING_BUFFER_DATA_SIZE;
                data_index++) {
                data[data_index] = dummy;
            }
            status = Ringbuf_Put(&test_buffer, data);
            ct_test(pTest, status == true);
        }

        for (count = 1; count < 4; count++) {
            dummy = index * count;
            test_data = Ringbuf_Get_Front(&test_buffer);
            for (data_index = 0; data_index < RING_BUFFER_DATA_SIZE;
                data_index++) {
                ct_test(pTest, test_data[data_index] == dummy);
            }

            test_data = Ringbuf_Pop_Front(&test_buffer);
            for (data_index = 0; data_index < RING_BUFFER_DATA_SIZE;
                data_index++) {
                ct_test(pTest, test_data[data_index] == dummy);
            }
        }
    }
    ct_test(pTest, Ringbuf_Empty(&test_buffer));


    return;
}

#ifdef TEST_RINGBUF
int main(
    void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("ringbuf", NULL);

    /* individual tests */
    rc = ct_addTestFunction(pTest, testRingBuf);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);

    ct_destroy(pTest);

    return 0;
}
#endif
#endif
