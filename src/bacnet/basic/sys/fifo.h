/**
 * @file
 * @brief Generic interrupt safe FIFO library for deeply embedded system.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
 * @copyright SPDX-License-Identifier: MIT
*/
#ifndef BACNET_SYS_FIFO_H
#define BACNET_SYS_FIFO_H
#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

/**
* FIFO buffer power of two alignment macro
*
* @{
*/
#ifndef NEXT_POWER_OF_2
#define B2(x)    (   (x) | (   (x) >> 1) )
#define B4(x)    ( B2(x) | ( B2(x) >> 2) )
#define B8(x)    ( B4(x) | ( B4(x) >> 4) )
#define B16(x)   ( B8(x) | ( B8(x) >> 8) )
#define B32(x)   (B16(x) | (B16(x) >>16) )
#define NEXT_POWER_OF_2(x) (B32((x)-1) + 1)
#endif
/** @} */

/**
* FIFO data store structure
*
* @{
*/
#define FIFO_DATA_STORE(b,c) uint8_t b[NEXT_POWER_OF_2(c)]
/** @} */

/**
* FIFO data structure
*
* @{
*/
struct fifo_buffer_t {
    /** first byte of data */
    volatile unsigned head;
    /** last byte of data */
    volatile unsigned tail;
    /** block of memory or array of data */
    volatile uint8_t *buffer;
    /** length of the data */
    unsigned buffer_len;
};
typedef struct fifo_buffer_t FIFO_BUFFER;
/** @} */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    unsigned FIFO_Count(
        FIFO_BUFFER const *b);

    BACNET_STACK_EXPORT
    bool FIFO_Full(
        FIFO_BUFFER const *b);

    BACNET_STACK_EXPORT
    bool FIFO_Available(
        FIFO_BUFFER const *b,
        unsigned count);

    BACNET_STACK_EXPORT
    bool FIFO_Empty(
        FIFO_BUFFER const *b);

    BACNET_STACK_EXPORT
    uint8_t FIFO_Peek(
        FIFO_BUFFER const *b);

    BACNET_STACK_EXPORT
    unsigned FIFO_Peek_Ahead(
        FIFO_BUFFER const *b,
        uint8_t* data_bytes,
        unsigned length);

    BACNET_STACK_EXPORT
    uint8_t FIFO_Get(
        FIFO_BUFFER * b);

    BACNET_STACK_EXPORT
    unsigned FIFO_Pull(
        FIFO_BUFFER * b,
        uint8_t * data_bytes,
        unsigned length);

    BACNET_STACK_EXPORT
    bool FIFO_Put(
        FIFO_BUFFER * b,
        uint8_t data_byte);

    BACNET_STACK_EXPORT
    bool FIFO_Add(
        FIFO_BUFFER * b,
        uint8_t * data_bytes,
        unsigned count);

    BACNET_STACK_EXPORT
    void FIFO_Flush(
        FIFO_BUFFER * b);

/* note: buffer_len must be a power of two */
    BACNET_STACK_EXPORT
    void FIFO_Init(
        FIFO_BUFFER * b,
        volatile uint8_t * buffer,
        unsigned buffer_len);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
