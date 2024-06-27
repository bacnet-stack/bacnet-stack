/**
 * @file
 * @brief BACnet integer encode and decode helper functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2012
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_INTEGER_H
#define BACNET_INTEGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /* unsigned value encoding and decoding */
    BACNET_STACK_EXPORT
    int encode_unsigned16(
        uint8_t * apdu,
        uint16_t value);
    BACNET_STACK_EXPORT
    int decode_unsigned16(
        uint8_t * apdu,
        uint16_t * value);
    BACNET_STACK_EXPORT
    int encode_unsigned24(
        uint8_t * apdu,
        uint32_t value);
    BACNET_STACK_EXPORT
    int decode_unsigned24(
        uint8_t * apdu,
        uint32_t * value);
    BACNET_STACK_EXPORT
    int encode_unsigned32(
        uint8_t * apdu,
        uint32_t value);
    BACNET_STACK_EXPORT
    int decode_unsigned32(
        uint8_t * apdu,
        uint32_t * value);
#ifdef UINT64_MAX
    BACNET_STACK_EXPORT
    int encode_unsigned40(
        uint8_t * buffer,
        uint64_t value);
    BACNET_STACK_EXPORT
    int decode_unsigned40(
        uint8_t * buffer,
        uint64_t * value);
    BACNET_STACK_EXPORT
    int encode_unsigned48(
        uint8_t * buffer,
        uint64_t value);
    BACNET_STACK_EXPORT
    int decode_unsigned48(
        uint8_t * buffer,
        uint64_t * value);
    BACNET_STACK_EXPORT
    int encode_unsigned56(
        uint8_t * buffer,
        uint64_t value);
    BACNET_STACK_EXPORT
    int decode_unsigned56(
        uint8_t * buffer,
        uint64_t * value);
    BACNET_STACK_EXPORT
    int encode_unsigned64(
        uint8_t * buffer,
        uint64_t value);
    BACNET_STACK_EXPORT
    int decode_unsigned64(
        uint8_t * buffer,
        uint64_t * value);
#endif

    BACNET_STACK_EXPORT
    int bacnet_unsigned_length(
        BACNET_UNSIGNED_INTEGER value);
    BACNET_STACK_EXPORT
    int bacnet_signed_length(
        int32_t value);

    /* signed value encoding and decoding */
    BACNET_STACK_EXPORT
    int encode_signed8(
        uint8_t * apdu,
        int8_t value);
    BACNET_STACK_EXPORT
    int decode_signed8(
        uint8_t * apdu,
        int32_t * value);
    BACNET_STACK_EXPORT
    int encode_signed16(
        uint8_t * apdu,
        int16_t value);
    BACNET_STACK_EXPORT
    int decode_signed16(
        uint8_t * apdu,
        int32_t * value);
    BACNET_STACK_EXPORT
    int encode_signed24(
        uint8_t * apdu,
        int32_t value);
    BACNET_STACK_EXPORT
    int decode_signed24(
        uint8_t * apdu,
        int32_t * value);
    BACNET_STACK_EXPORT
    int encode_signed32(
        uint8_t * apdu,
        int32_t value);
    BACNET_STACK_EXPORT
    int decode_signed32(
        uint8_t * apdu,
        int32_t * value);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
