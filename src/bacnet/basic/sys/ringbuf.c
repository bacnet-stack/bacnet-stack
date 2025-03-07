/**
 * @file
 * @brief  Generic ring buffer library for deeply embedded system.
 * @details Generic ring buffer library that uses a data store whose size
 * is a power of 2 (8, 16, 32, 64, ...) and doesn't waste any data bytes.
 * It has very low overhead, and utilizes modulo for indexing the data
 * in the data store. The ring buffer uses separate variables for
 * consumer and producer so it can be used in multithreaded environment.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
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
volatile void *
Ringbuf_Peek_Next(RING_BUFFER const *b, const uint8_t *data_element)
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
    RING_BUFFER *b, const uint8_t *this_element, uint8_t *data_element)
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
 * @return true on successful add, false if not added
 */
bool Ringbuf_Put(RING_BUFFER *b, const uint8_t *data_element)
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
 * @return true on successful add, false if not added
 */
bool Ringbuf_Put_Front(RING_BUFFER *b, const uint8_t *data_element)
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
bool Ringbuf_Data_Put(RING_BUFFER *b, const volatile uint8_t *data_element)
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
 * Gets the data size of each element in the ring buffer
 *
 * @param  b - pointer to RING_BUFFER structure
 * @return size of each element in the ring buffer
 */
unsigned Ringbuf_Data_Size(RING_BUFFER const *b)
{
    unsigned size = 0;

    if (b) {
        size = b->element_size;
    }

    return size;
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
 * @return  true if ring buffer was initialized
 * @deprecated  Use Ringbuf_Initialize() instead
 */
bool Ringbuf_Init(
    RING_BUFFER *b,
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

/**
 * Configures the ring buffer data buffer.  Note that the element_count
 * parameter must be a power of two.
 *
 * @param  b - pointer to RING_BUFFER structure
 * @param  buffer - pointer to a data buffer that is used to store the ring data
 * @param  buffer_size - size of the data buffer
 * @param  element_size - size of one element in the data block
 * @param  element_count - number elements in the data block
 *
 * @return  true if ring buffer was initialized
 */
bool Ringbuf_Initialize(
    RING_BUFFER *b,
    volatile uint8_t *buffer,
    unsigned buffer_size,
    unsigned element_size,
    unsigned element_count)
{
    if ((element_size * element_count) <= buffer_size) {
        return Ringbuf_Init(b, buffer, element_size, element_count);
    }

    return false;
}
