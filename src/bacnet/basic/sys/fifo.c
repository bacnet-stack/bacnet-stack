/**
 * @file
 * @brief Generic interrupt safe FIFO library for deeply embedded system.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 * @section DESCRIPTION
 *
 * Generic interrupt safe FIFO library for deeply embedded system
 * This library only uses a byte sized chunk for a data element.
 * It uses a data store whose size is a power of 2 (8, 16, 32, 64, ...)
 * and doesn't waste any data bytes.  It has very low overhead, and
 * utilizes modulo for indexing the data in the data store.
 *
 * To use this library, first declare a data store, sized for a power of 2:
 * {@code
 * static volatile FIFO_DATA_STORE(data_store, 64);
 * }
 *
 * Then declare the FIFO tracking structure:
 * {@code
 * static FIFO_BUFFER queue;
 * }
 *
 * Initialize the queue with the data store:
 * {@code
 * FIFO_Init(&queue, data_store, sizeof(data_store));
 * }
 *
 * Then begin to use the FIFO queue by giving it data, retrieving data,
 * and checking the FIFO queue to see if it is empty or full:
 * {@code
 * uint8_t in_data = 0;
 * uint8_t out_data = 0;
 * uint8_t add_data[5] = {0};
 * uint8_t pull_data[5] = {0};
 * unsigned count = 0;
 * bool status = false;
 *
 * status = FIFO_Put(&queue, in_data);
 * if (!FIFO_Empty(&queue)) {
 *     out_data = FIFO_Get(&queue);
 * }
 * if (FIFO_Available(&queue, sizeof(add_data))) {
 *     status = FIFO_Add(&queue, add_data, sizeof(add_data));
 * }
 * count = FIFO_Count(&queue);
 * if (count == sizeof(add_data)) {
 *     count = FIFO_Pull(&queue, &pull_data[0], sizeof(pull_data));
 * }
 *
 * }
 *
 * Normally the FIFO is used by a producer, such as in interrupt service
 * routine, which places data into the queue using FIFO_Put(), and a consumer,
 * such as a main loop handler, which pulls data from the queue by first
 * checking the queue for data using FIFO_Empty(), and then pulling data from
 * the queue using FIFO_Get().
 *
 */
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "bacnet/basic/sys/fifo.h"

/**
 * Returns the number of bytes in the FIFO
 *
 * @param b - pointer to FIFO_BUFFER structure
 *
 * @return Number of bytes in the FIFO
 */
unsigned FIFO_Count(FIFO_BUFFER const *b)
{
    unsigned head, tail; /* used to avoid volatile decision */

    if (b) {
        head = b->head;
        tail = b->tail;
        return head - tail;
    } else {
        return 0;
    }
}

/**
 * Returns the full status of the FIFO
 *
 * @param b - pointer to FIFO_BUFFER structure
 *
 * @return true if the FIFO is full, false if it is not.
 */
bool FIFO_Full(FIFO_BUFFER const *b)
{
    return (b ? (FIFO_Count(b) == b->buffer_len) : true);
}

/**
 * Tests to see if space is available in the FIFO
 *
 * @param b - pointer to FIFO_BUFFER structure
 * @param count [in] - number of bytes tested for availability
 *
 * @return true if the number of bytes sought is available
 */
bool FIFO_Available(FIFO_BUFFER const *b, unsigned count)
{
    return (b ? (count <= (b->buffer_len - FIFO_Count(b))) : false);
}

/**
 * Returns the empty status of the FIFO
 *
 * @param b - pointer to FIFO_BUFFER structure
 * @return true if the FIFO is empty, false if it is not.
 */
bool FIFO_Empty(FIFO_BUFFER const *b)
{
    return (b ? (FIFO_Count(b) == 0) : true);
}

/**
 * Peeks at the data from the front of the FIFO without removing it.
 * Use FIFO_Empty() or FIFO_Available() function to see if there is
 * data to retrieve since this function doesn't return a flag indicating
 * success or failure.
 *
 * @param b - pointer to FIFO_BUFFER structure
 *
 * @return byte of data, or zero if nothing in the list
 */
uint8_t FIFO_Peek(FIFO_BUFFER const *b)
{
    unsigned index;

    if (b) {
        index = b->tail % b->buffer_len;
        return (b->buffer[index]);
    }

    return 0;
}

/**
 * Peeks ahead from the front of the FIFO without removing any data.
 * Limit the number of bytes peeked to the number of bytes available.
 *
 * @param b - pointer to FIFO_BUFFER structure
 * @param buffer [out] - buffer to hold the peeked bytes
 * @param length [in] - number of bytes to peek from the FIFO
 * @return number of bytes peeked
 */
unsigned FIFO_Peek_Ahead(FIFO_BUFFER const *b, uint8_t* buffer, unsigned length)
{
    unsigned count = 0;
    unsigned index;
    unsigned tail;
    unsigned i;

    if (b) {
        count = FIFO_Count(b);
        if (count > length) {
            /* adjust to limit the number of bytes peeked */
            count = length;
        }
        tail = b->tail;
        for(i = 0; i < count; i++) {
            index = tail % b->buffer_len;
            buffer[i] = b->buffer[index];
            tail++;
        }
    }

    return count;
}

/**
 * Gets a byte from the front of the FIFO, and removes it.
 * Use FIFO_Empty() or FIFO_Available() function to see if there is
 * data to retrieve since this function doesn't return a flag indicating
 * success or failure.
 *
 * @param b - pointer to FIFO_BUFFER structure
 *
 * @return the data
 */
uint8_t FIFO_Get(FIFO_BUFFER *b)
{
    uint8_t data_byte = 0;
    unsigned index;

    if (!FIFO_Empty(b)) {
        index = b->tail % b->buffer_len;
        data_byte = b->buffer[index];
        b->tail++;
    }
    return data_byte;
}

/**
 * Pulls one or more bytes from the front of the FIFO, and removes them
 * from the FIFO.  If less bytes are available, only the available bytes
 * are retrieved.
 *
 * @param b - pointer to FIFO_BUFFER structure
 * @param buffer [out] - buffer to hold the pulled bytes
 * @param length [in] - number of bytes to pull from the FIFO
 *
 * @return      the number of bytes actually pulled from the FIFO
 */
unsigned FIFO_Pull(FIFO_BUFFER *b, uint8_t *buffer, unsigned length)
{
    unsigned count;
    uint8_t data_byte;
    unsigned index;

    count = FIFO_Count(b);
    if (count > length) {
        /* adjust to limit the number of bytes pulled */
        count = length;
    }
    if (length > count) {
        /* adjust the return value */
        length = count;
    }
    while (count) {
        index = b->tail % b->buffer_len;
        data_byte = b->buffer[index];
        b->tail++;
        if (buffer) {
            *buffer = data_byte;
            buffer++;
        }
        count--;
    }

    return length;
}

/**
 * Adds a byte of data to the FIFO
 *
 * @param  b - pointer to FIFO_BUFFER structure
 * @param  data_byte [in] - data to put into the FIFO
 *
 * @return true on successful add, false if not added
 */
bool FIFO_Put(FIFO_BUFFER *b, uint8_t data_byte)
{
    bool status = false; /* return value */
    unsigned index;

    if (b) {
        /* limit the buffer to prevent overwriting */
        if (!FIFO_Full(b)) {
            index = b->head % b->buffer_len;
            b->buffer[index] = data_byte;
            b->head++;
            status = true;
        }
    }

    return status;
}

/**
 * Adds one or more bytes of data to the FIFO
 *
 * @param  b - pointer to FIFO_BUFFER structure
 * @param  buffer [out] - data bytes to add to the FIFO
 * @param  count [in] - number of bytes to add to the FIFO
 *
 * @return true if space available and added, false if not added
 */
bool FIFO_Add(FIFO_BUFFER *b, uint8_t *buffer, unsigned count)
{
    bool status = false; /* return value */
    unsigned index;

    /* limit the buffer to prevent overwriting */
    if (FIFO_Available(b, count) && buffer) {
        while (count) {
            index = b->head % b->buffer_len;
            b->buffer[index] = *buffer;
            b->head++;
            buffer++;
            count--;
        }
        status = true;
    }

    return status;
}

/**
 * Flushes any data in the FIFO buffer
 *
 * @param  b - pointer to FIFO_BUFFER structure
 *
 * @return none
 */
void FIFO_Flush(FIFO_BUFFER *b)
{
    unsigned head; /* used to avoid volatile decision */

    if (b) {
        head = b->head;
        b->tail = head;
    }
}

/**
 * Initializes the FIFO buffer with a data store
 *
 * @param  b - pointer to FIFO_BUFFER structure
 * @param  buffer [in] - data bytes used to store bytes used by the FIFO
 * @param  buffer_len [in] - size of the buffer in bytes - must be power of 2.
 *
 * @return      none
 */
void FIFO_Init(FIFO_BUFFER *b, volatile uint8_t *buffer, unsigned buffer_len)
{
    if (b && buffer && buffer_len) {
        b->head = 0;
        b->tail = 0;
        b->buffer = buffer;
        b->buffer_len = buffer_len;
    }

    return;
}
