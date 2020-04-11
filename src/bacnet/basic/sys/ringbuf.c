/**
 * @file
 * @author Steve Karg
 * @date 2004
 * @brief  Generic ring buffer library for deeply embedded system.
 *
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
 * Generic ring buffer library for deeply embedded system.
 * It uses a data store whose size is a power of 2 (8, 16, 32, 64, ...)
 * and doesn't waste any data bytes.  It has very low overhead, and
 * utilizes modulo for indexing the data in the data store.
 * It uses separate variables for consumer and producer so it can
 * be used in multithreaded environment.
 *
 * See the unit tests for usage examples.
 *
 */
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "bacnet/basic/sys/ringbuf.h"

/**
 * Returns the number of elements in the ring buffer
 *
 * @param  b - pointer to RING_BUFFER structure
 * @return Number of elements in the ring buffer
 */
unsigned Ringbuf_Count(RING_BUFFER const *b)
{
    unsigned head, tail; /* used to avoid volatile decision */

    if (b) {
        head = b->head;
        tail = b->tail;
        return head - tail;
    }

    return 0;
}

/**
 * Returns the empty/full status of the ring buffer
 *
 * @param  b - pointer to RING_BUFFER structure
 * @return true if the ring buffer is full, false if it is not.
 */
bool Ringbuf_Full(RING_BUFFER const *b)
{
    return (b ? (Ringbuf_Count(b) == b->element_count) : true);
}

/**
 * Returns the empty/full status of the ring buffer
 *
 * @param  b - pointer to RING_BUFFER structure
 * @return true if the ring buffer is empty, false if it is not.
 */
bool Ringbuf_Empty(RING_BUFFER const *b)
{
    return (b ? (Ringbuf_Count(b) == 0) : true);
}

/**
 * Updates the depth tracking in the ring buffer
 *
 * @param  b - pointer to RING_BUFFER structure
 */
static void Ringbuf_Depth_Update(RING_BUFFER *b)
{
    unsigned count;

    if (b) {
        count = Ringbuf_Count(b);
        if (count > b->depth) {
            b->depth = count;
        }
    }
}

/**
 * Updates the depth tracking in the ring buffer
 *
 * @param  b - pointer to RING_BUFFER structure
 * @return largest number of items that have been in the ring buffer
 */
unsigned Ringbuf_Depth(RING_BUFFER const *b)
{
    unsigned depth = 0;

    if (b) {
        depth = b->depth;
    }

    return depth;
}

/**
 * Resets the depth tracking in the ring buffer
 *
 * @param  b - pointer to RING_BUFFER structure
 * @return largest number of items that have been in the ring buffer
 */
unsigned Ringbuf_Depth_Reset(RING_BUFFER *b)
{
    unsigned depth = 0;

    if (b) {
        depth = b->depth;
        b->depth = 0;
    }

    return depth;
}

/**
 * Gets the capacity of the ring buffer (number of possible elements)
 *
 * @param  b - pointer to RING_BUFFER structure
 * @return largest number of items that have been in the ring buffer
 */
unsigned Ringbuf_Size(RING_BUFFER const *b)
{
    unsigned count = 0;

    if (b) {
        count = b->element_count;
    }

    return count;
}

/**
 * Looks at the data from the head of the list without removing it
 *
 * @param  b - pointer to RING_BUFFER structure
 * @return pointer to the data, or NULL if nothing in the list
 */
volatile void *Ringbuf_Peek(RING_BUFFER const *b)
{
    volatile uint8_t *data_element = NULL; /* return value */

    if (!Ringbuf_Empty(b)) {
        data_element = b->buffer;
        data_element += ((b->tail % b->element_count) * b->element_size);
    }

    return data_element;
}

/**
 * Looks at the data from the next element of the list without removing it
 *
 * @param  b - pointer to RING_BUFFER structure
 * @param  data_element - find the next element from this one
 * @return pointer to the data, or NULL if nothing in the list
 */
volatile void *Ringbuf_Peek_Next(RING_BUFFER const *b, uint8_t *data_element)
{
    unsigned index; /* list index */
    volatile uint8_t *this_element;
    volatile uint8_t *next_element = NULL; /* return value */
    if (!Ringbuf_Empty(b) && data_element != NULL) {
        /* Use (b->head-1) here to avoid walking off end of ring */
        for (index = b->tail; index < b->head - 1; index++) {
            /* Find the specified data_element */
            this_element =
                b->buffer + ((index % b->element_count) * b->element_size);
            if (data_element == this_element) {
                /* Found the current element, get the next one on the list */
                next_element = b->buffer +
                    (((index + 1) % b->element_count) * b->element_size);
                break;
            }
        }
    }

    return next_element;
}

/**
 * Copy the data from the front of the list, and removes it
 *
 * @param  b - pointer to RING_BUFFER structure
 * @param  data_element - element of data that is loaded with data from ring
 * @return true if data was copied, false if list is empty
 */
bool Ringbuf_Pop(RING_BUFFER *b, uint8_t *data_element)
{
    bool status = false; /* return value */
    volatile uint8_t *ring_data = NULL; /* used to help point ring data */
    unsigned i; /* loop counter */

    if (!Ringbuf_Empty(b)) {
        ring_data = b->buffer;
        ring_data += ((b->tail % b->element_count) * b->element_size);
        if (data_element) {
            for (i = 0; i < b->element_size; i++) {
                data_element[i] = ring_data[i];
            }
        }
        b->tail++;
        status = true;
    }

    return status;
}

/**
 * Copy the data from the specified element, and removes it and moves other
 * elements up the list
 *
 * @param  b - pointer to RING_BUFFER structure
 * @param  this_element - element to find
 * @param  data_element - element of data that is loaded with data from ring
 * @return true if data was copied, false if list is empty
 */
bool Ringbuf_Pop_Element(
    RING_BUFFER *b, uint8_t *this_element, uint8_t *data_element)
{
    bool status = false; /* return value */
    volatile uint8_t *ring_data = NULL; /* used to help point ring data */
    volatile uint8_t *prev_data;
    unsigned index; /* list index */
    unsigned this_index = b->head; /* index of element to remove */
    unsigned i; /* loop counter */
    if (!Ringbuf_Empty(b) && this_element != NULL) {
        for (index = b->tail; index < b->head; index++) {
            /* Find the specified data_element */
            ring_data =
                b->buffer + ((index % b->element_count) * b->element_size);
            if (this_element == ring_data) {
                /* Found the specified element, copy the data if required */
                this_index = index;
                if (data_element) {
                    for (i = 0; i < b->element_size; i++) {
                        data_element[i] = ring_data[i];
                    }
                }
                break;
            }
        }
        if (this_index < b->head) {
            /* Found a match, move elements up the list to fill the gap */
            for (index = this_index; index > b->tail; index--) {
                /* Get pointers to current and previous data_elements */
                ring_data =
                    b->buffer + ((index % b->element_count) * b->element_size);
                prev_data = b->buffer +
                    (((index - 1) % b->element_count) * b->element_size);
                for (i = 0; i < b->element_size; i++) {
                    ring_data[i] = prev_data[i];
                }
            }
        }
        b->tail++;
        status = true;
    }

    return status;
}

/**
 * Adds an element of data to the ring buffer
 *
 * @param  b - pointer to RING_BUFFER structure
 * @param  data_element - one element that is copied to the ring buffer
 * @return true on succesful add, false if not added
 */
bool Ringbuf_Put(RING_BUFFER *b, uint8_t *data_element)
{
    bool status = false; /* return value */
    volatile uint8_t *ring_data = NULL; /* used to help point ring data */
    unsigned i; /* loop counter */

    if (b && data_element) {
        /* limit the amount of elements that we accept */
        if (!Ringbuf_Full(b)) {
            ring_data = b->buffer;
            ring_data += ((b->head % b->element_count) * b->element_size);
            for (i = 0; i < b->element_size; i++) {
                ring_data[i] = data_element[i];
            }
            b->head++;
            Ringbuf_Depth_Update(b);
            status = true;
        }
    }

    return status;
}

/**
 * Adds an element of data to the front of the ring buffer
 *
 * Note that this function moves the tail on add instead of head,
 * so this function cannot be used if you are keeping producer and
 * consumer as separate processes (i.e. interrupt handlers)
 *
 * @param  b - pointer to RING_BUFFER structure
 * @param  data_element - one element to copy to the front of the ring
 * @return true on succesful add, false if not added
 */
bool Ringbuf_Put_Front(RING_BUFFER *b, uint8_t *data_element)
{
    bool status = false; /* return value */
    volatile uint8_t *ring_data = NULL; /* used to help point ring data */
    unsigned i = 0; /* loop counter */

    if (b && data_element) {
        /* limit the amount of elements that we accept */
        if (!Ringbuf_Full(b)) {
            b->tail--;
            ring_data = b->buffer;
            ring_data += ((b->tail % b->element_count) * b->element_size);
            /* copy the data to the ring data element */
            for (i = 0; i < b->element_size; i++) {
                ring_data[i] = data_element[i];
            }
            Ringbuf_Depth_Update(b);
            status = true;
        }
    }

    return status;
}

/**
 * Gets a pointer to the next free data element of the buffer
 *              without adding it to the ring.
 *
 * @param  b - pointer to RING_BUFFER structure
 * @return pointer to the next data element, or NULL if the list is full
 */
volatile void *Ringbuf_Data_Peek(RING_BUFFER *b)
{
    volatile uint8_t *ring_data = NULL; /* used to help point ring data */

    if (b) {
        /* limit the amount of elements that we accept */
        if (!Ringbuf_Full(b)) {
            ring_data = b->buffer;
            ring_data += ((b->head % b->element_count) * b->element_size);
        }
    }

    return ring_data;
}

/**
 * Adds the previously peeked element of data to the end of the
 * ring buffer.
 *
 * @param  b - pointer to RING_BUFFER structure
 * @param  data_element - pointer to the peeked data element
 * @return true if the buffer has space and the data element points to the
 *              same memory previously peeked.
 */
bool Ringbuf_Data_Put(RING_BUFFER *b, volatile uint8_t *data_element)
{
    bool status = false;
    volatile uint8_t *ring_data = NULL; /* used to help point ring data */

    if (b) {
        /* limit the amount of elements that we accept */
        if (!Ringbuf_Full(b)) {
            ring_data = b->buffer;
            ring_data += ((b->head % b->element_count) * b->element_size);
            if (ring_data == data_element) {
                /* same chunk of memory - okay to signal the head */
                b->head++;
                Ringbuf_Depth_Update(b);
                status = true;
            }
        }
    }

    return status;
}

/**
 * Test that the parameter is a power of two.
 *
 * @param x unsigned integer value to be tested
 *
 * @return  true if the parameter is a power of 2
 */
static bool isPowerOfTwo(unsigned int x)
{
    /* First x in the below expression is for the case when x is 0 */
    return x && (!(x & (x - 1)));
}

/**
 * Configures the ring buffer data buffer.  Note that the element_count
 * parameter must be a power of two.
 *
 * @param  b - pointer to RING_BUFFER structure
 * @param  buffer - pointer to a data buffer that is used to store the ring data
 * @param  element_size - size of one element in the data block
 * @param  element_count - number elements in the data block
 *
 * @return  true if ring buffer was initialized
 */
bool Ringbuf_Init(RING_BUFFER *b,
    volatile uint8_t *buffer,
    unsigned element_size,
    unsigned element_count)
{
    bool status = false;

    if (b && isPowerOfTwo(element_count)) {
        b->head = 0;
        b->tail = 0;
        b->buffer = buffer;
        b->element_size = element_size;
        b->element_count = element_count;
        /* tuning diagnostics */
        b->depth = 0;
        status = true;
    }

    return status;
}

#ifdef TEST
#include <assert.h>
#include <string.h>
#include <limits.h>
#include "ctest.h"

/**
 * Unit Test for the ring buffer
 *
 * @param  pTest - test tracking pointer
 * @param  test_buffer - pointer to RING_BUFFER structure
 * @param  data_element - one data element
 * @param  element_size - size of one data element
 * @param  element_count - number of data elements in the store
 */
static void testRingAroundBuffer(Test *pTest,
    RING_BUFFER *test_buffer,
    uint8_t *data_element,
    unsigned element_size,
    unsigned element_count)
{
    volatile uint8_t *test_data;
    unsigned index;
    unsigned data_index;
    unsigned count;
    uint8_t value;
    bool status;

    ct_test(pTest, Ringbuf_Empty(test_buffer));
    /* test the ring around the buffer */
    for (index = 0; index < element_count; index++) {
        for (count = 1; count < 4; count++) {
            value = (index * count) % 255;
            for (data_index = 0; data_index < element_size; data_index++) {
                data_element[data_index] = value;
            }
            status = Ringbuf_Put(test_buffer, data_element);
            ct_test(pTest, status == true);
            ct_test(pTest, Ringbuf_Count(test_buffer) == count);
        }
        for (count = 1; count < 4; count++) {
            value = (index * count) % 255;
            test_data = Ringbuf_Peek(test_buffer);
            ct_test(pTest, test_data);
            if (test_data) {
                for (data_index = 0; data_index < element_size; data_index++) {
                    ct_test(pTest, test_data[data_index] == value);
                }
            }
            status = Ringbuf_Pop(test_buffer, NULL);
            ct_test(pTest, status == true);
        }
    }
    ct_test(pTest, Ringbuf_Empty(test_buffer));
}

/**
 * Unit Test for the ring buffer
 *
 * @param  pTest - test tracking pointer
 * @param  data_store - buffer to store elements
 * @param  data_element - one data element
 * @param  element_size - size of one data element
 * @param  element_count - number of data elements in the store
 */
static bool testRingBuf(Test *pTest,
    uint8_t *data_store,
    uint8_t *data_element,
    unsigned element_size,
    unsigned element_count)
{
    RING_BUFFER test_buffer;
    volatile uint8_t *test_data;
    unsigned index;
    unsigned data_index;
    bool status;

    status =
        Ringbuf_Init(&test_buffer, data_store, element_size, element_count);
    if (!status) {
        return false;
    }
    ct_test(pTest, Ringbuf_Empty(&test_buffer));
    ct_test(pTest, Ringbuf_Depth(&test_buffer) == 0);

    for (data_index = 0; data_index < element_size; data_index++) {
        data_element[data_index] = data_index;
    }
    status = Ringbuf_Put(&test_buffer, data_element);
    ct_test(pTest, status == true);
    ct_test(pTest, !Ringbuf_Empty(&test_buffer));
    ct_test(pTest, Ringbuf_Depth(&test_buffer) == 1);

    test_data = Ringbuf_Peek(&test_buffer);
    for (data_index = 0; data_index < element_size; data_index++) {
        ct_test(pTest, test_data[data_index] == data_element[data_index]);
    }
    ct_test(pTest, !Ringbuf_Empty(&test_buffer));
    (void)Ringbuf_Pop(&test_buffer, NULL);
    ct_test(pTest, Ringbuf_Empty(&test_buffer));
    ct_test(pTest, Ringbuf_Depth(&test_buffer) == 1);

    /* fill to max */
    for (index = 0; index < element_count; index++) {
        for (data_index = 0; data_index < element_size; data_index++) {
            data_element[data_index] = index;
        }
        status = Ringbuf_Put(&test_buffer, data_element);
        ct_test(pTest, status == true);
        ct_test(pTest, !Ringbuf_Empty(&test_buffer));
        ct_test(pTest, Ringbuf_Depth(&test_buffer) == (index + 1));
    }
    ct_test(pTest, Ringbuf_Depth(&test_buffer) == element_count);
    /* verify actions on full buffer */
    for (index = 0; index < element_count; index++) {
        for (data_index = 0; data_index < element_size; data_index++) {
            data_element[data_index] = index;
        }
        status = Ringbuf_Put(&test_buffer, data_element);
        ct_test(pTest, status == false);
        ct_test(pTest, !Ringbuf_Empty(&test_buffer));
        ct_test(pTest, Ringbuf_Depth(&test_buffer) == element_count);
    }

    /* check buffer full */
    for (index = 0; index < element_count; index++) {
        test_data = Ringbuf_Peek(&test_buffer);
        ct_test(pTest, test_data);
        if (test_data) {
            for (data_index = 0; data_index < element_size; data_index++) {
                ct_test(pTest, test_data[data_index] == index);
            }
        }
        (void)Ringbuf_Pop(&test_buffer, NULL);
    }
    ct_test(pTest, Ringbuf_Empty(&test_buffer));
    ct_test(pTest, Ringbuf_Depth(&test_buffer) == element_count);
    Ringbuf_Depth_Reset(&test_buffer);
    ct_test(pTest, Ringbuf_Depth(&test_buffer) == 0);

    testRingAroundBuffer(
        pTest, &test_buffer, data_element, element_size, element_count);

    /* adjust the internal index of Ringbuf to test unsigned wrapping */
    test_buffer.head = UINT_MAX - 1;
    test_buffer.tail = UINT_MAX - 1;

    testRingAroundBuffer(
        pTest, &test_buffer, data_element, element_size, element_count);

    return true;
}

/**
 * Unit Test for the ring buffer with 16 data elements
 *
 * @param       pTest - test tracking pointer
 */
void testRingBufSizeSmall(Test *pTest)
{
    bool status;
    uint8_t data_element[5];
    uint8_t data_store[sizeof(data_element) * NEXT_POWER_OF_2(16)];

    status = testRingBuf(pTest, data_store, data_element, sizeof(data_element),
        sizeof(data_store) / sizeof(data_element));
    ct_test(pTest, status);
}

/**
 * Unit Test for the ring buffer with 32 data elements
 *
 * @param       pTest - test tracking pointer
 */
void testRingBufSizeLarge(Test *pTest)
{
    bool status;
    uint8_t data_element[16];
    uint8_t data_store[sizeof(data_element) * NEXT_POWER_OF_2(99)];

    status = testRingBuf(pTest, data_store, data_element, sizeof(data_element),
        sizeof(data_store) / sizeof(data_element));
    ct_test(pTest, status);
}

/**
 * Unit Test for the ring buffer with 32 data elements
 *
 * @param       pTest - test tracking pointer
 */
void testRingBufSizeInvalid(Test *pTest)
{
    bool status;
    uint8_t data_element[16];
    uint8_t data_store[sizeof(data_element) * 99];

    status = testRingBuf(pTest, data_store, data_element, sizeof(data_element),
        sizeof(data_store) / sizeof(data_element));
    ct_test(pTest, status == false);
}

void testRingBufPowerOfTwo(Test *pTest)
{
    ct_test(pTest, NEXT_POWER_OF_2(3) == 4);
    ct_test(pTest, NEXT_POWER_OF_2(100) == 128);
    ct_test(pTest, NEXT_POWER_OF_2(127) == 128);
    ct_test(pTest, NEXT_POWER_OF_2(128) == 128);
    ct_test(pTest, NEXT_POWER_OF_2(129) == 256);
    ct_test(pTest, NEXT_POWER_OF_2(300) == 512);
    ct_test(pTest, NEXT_POWER_OF_2(500) == 512);
}

/**
 * Unit Test for the ring buffer peek/pop next element
 *
 * @param  pTest - test tracking pointer
 * @param  data_store - buffer to store elements
 * @param  data_element - one data element
 * @param  element_size - size of one data element
 * @param  element_count - number of data elements in the store
 */
static bool testRingBufNextElement(Test *pTest,
    uint8_t *data_store,
    uint8_t *data_element,
    unsigned element_size,
    unsigned element_count)
{
    RING_BUFFER test_buffer;
    volatile uint8_t *test_data;
    unsigned index;
    unsigned data_index;
    bool status;
    status =
        Ringbuf_Init(&test_buffer, data_store, element_size, element_count);
    if (!status) {
        return false;
    }
    ct_test(pTest, Ringbuf_Empty(&test_buffer));
    ct_test(pTest, Ringbuf_Depth(&test_buffer) == 0);

    for (data_index = 0; data_index < element_size; data_index++) {
        data_element[data_index] = data_index;
    }
    status = Ringbuf_Put(&test_buffer, data_element);
    ct_test(pTest, status == true);
    ct_test(pTest, !Ringbuf_Empty(&test_buffer));
    ct_test(pTest, Ringbuf_Depth(&test_buffer) == 1);

    test_data = Ringbuf_Peek(&test_buffer);
    for (data_index = 0; data_index < element_size; data_index++) {
        ct_test(pTest, test_data[data_index] == data_element[data_index]);
    }
    ct_test(pTest, !Ringbuf_Empty(&test_buffer));
    (void)Ringbuf_Pop(&test_buffer, NULL);
    ct_test(pTest, Ringbuf_Empty(&test_buffer));
    ct_test(pTest, Ringbuf_Depth(&test_buffer) == 1);

    /* fill to max */
    for (index = 0; index < element_count; index++) {
        for (data_index = 0; data_index < element_size; data_index++) {
            data_element[data_index] = index;
        }
        status = Ringbuf_Put(&test_buffer, data_element);
        ct_test(pTest, status == true);
        ct_test(pTest, !Ringbuf_Empty(&test_buffer));
        ct_test(pTest, Ringbuf_Depth(&test_buffer) == (index + 1));
    }
    ct_test(pTest, Ringbuf_Depth(&test_buffer) == element_count);
    ct_test(pTest, Ringbuf_Count(&test_buffer) == element_count);

    /* Walk through ring buffer */
    test_data = Ringbuf_Peek(&test_buffer);
    ct_test(pTest, test_data);
    for (index = 1; index < element_count; index++) {
        test_data = Ringbuf_Peek_Next(&test_buffer, (uint8_t *)test_data);
        ct_test(pTest, test_data);
        if (test_data) {
            for (data_index = 0; data_index < element_size; data_index++) {
                ct_test(pTest, test_data[data_index] == index);
            }
        }
    }
    ct_test(pTest, Ringbuf_Count(&test_buffer) == element_count);
    /* Try to walk off end of buffer - should return NULL */
    test_data = Ringbuf_Peek_Next(&test_buffer, (uint8_t *)test_data);
    ct_test(pTest, (test_data == NULL));

    /* Walk through ring buffer and pop alternate elements */
    test_data = Ringbuf_Peek(&test_buffer);
    ct_test(pTest, test_data);
    for (index = 1; index < element_count / 2; index++) {
        test_data = Ringbuf_Peek_Next(&test_buffer, (uint8_t *)test_data);
        ct_test(pTest, test_data);
        (void)Ringbuf_Pop_Element(&test_buffer, (uint8_t *)test_data, NULL);
        test_data = Ringbuf_Peek_Next(&test_buffer, (uint8_t *)test_data);
    }
    ct_test(pTest, Ringbuf_Count(&test_buffer) == element_count / 2 + 1);

    /* Walk through ring buffer and check data */
    test_data = Ringbuf_Peek(&test_buffer);
    ct_test(pTest, test_data);
    for (index = 0; index < element_count / 2; index++) {
        if (test_data) {
            for (data_index = 0; data_index < element_size; data_index++) {
                ct_test(pTest, test_data[data_index] == index * 2);
            }
        }
        test_data = Ringbuf_Peek_Next(&test_buffer, (uint8_t *)test_data);
        ct_test(pTest, test_data);
    }
    ct_test(pTest, Ringbuf_Count(&test_buffer) == element_count / 2 + 1);

    return true;
}

/**
 * Unit Test for the ring buffer with 16 data elements
 *
 * @param       pTest - test tracking pointer
 */
void testRingBufNextElementSizeSmall(Test *pTest)
{
    bool status;
    uint8_t data_element[5];
    uint8_t data_store[sizeof(data_element) * NEXT_POWER_OF_2(16)];

    status = testRingBufNextElement(pTest, data_store, data_element,
        sizeof(data_element), sizeof(data_store) / sizeof(data_element));
    ct_test(pTest, status);
}

#ifdef TEST_RING_BUFFER
/**
 * Main program entry for Unit Test
 *
 * @return  returns 0 on success, and non-zero on fail.
 */
int main(void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("Ring Buffer", NULL);

    /* individual tests */
    rc = ct_addTestFunction(pTest, testRingBufPowerOfTwo);
    assert(rc);
    rc = ct_addTestFunction(pTest, testRingBufSizeSmall);
    assert(rc);
    rc = ct_addTestFunction(pTest, testRingBufSizeLarge);
    assert(rc);
    rc = ct_addTestFunction(pTest, testRingBufSizeInvalid);
    assert(rc);
    rc = ct_addTestFunction(pTest, testRingBufNextElementSizeSmall);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void)ct_report(pTest);

    ct_destroy(pTest);

    return 0;
}
#endif
#endif
