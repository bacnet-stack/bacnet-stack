
/**
 * @file
 * @brief BACnet integer encode and decode helper functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacint.h"

int encode_unsigned16(uint8_t *apdu, uint16_t value)
{
    if (apdu) {
        apdu[0] = (uint8_t)((value >> 8) & 0xFF);
        apdu[1] = (uint8_t)((value >> 0) & 0xFF);
    }

    return 2;
}

int decode_unsigned16(const uint8_t *apdu, uint16_t *value)
{
    if (apdu && value) {
        *value  = (uint16_t)apdu[0] << 8;
        *value |= (uint16_t)apdu[1];
    }

    return 2;
}

int encode_unsigned24(uint8_t *apdu, uint32_t value)
{
    if (apdu) {
        apdu[0] = (uint8_t)((value >> 16) & 0xFF);
        apdu[1] = (uint8_t)((value >> 8)  & 0xFF);
        apdu[2] = (uint8_t)((value >> 0)  & 0xFF);
    }

    return 3;
}

int decode_unsigned24(const uint8_t *apdu, uint32_t *value)
{
    if (apdu && value) {
        *value  = (uint32_t)apdu[0] << 16;
        *value |= (uint32_t)apdu[1] << 8;
        *value |= (uint32_t)apdu[2];
    }

    return 3;
}

int encode_unsigned32(uint8_t *apdu, uint32_t value)
{
    if (apdu) {
        apdu[0] = (uint8_t)((value >> 24) & 0xFF);
        apdu[1] = (uint8_t)((value >> 16) & 0xFF);
        apdu[2] = (uint8_t)((value >> 8)  & 0xFF);
        apdu[3] = (uint8_t)((value >> 0)  & 0xFF);
    }

    return 4;
}

int decode_unsigned32(const uint8_t *apdu, uint32_t *value)
{
    if (apdu && value) {
        *value  = (uint32_t)apdu[0] << 24;
        *value |= (uint32_t)apdu[1] << 16;
        *value |= (uint32_t)apdu[2] << 8;
        *value |= (uint32_t)apdu[3];
    }

    return 4;
}

#ifdef UINT64_MAX
/**
 * @brief       Encode a 40-bit unsigned value into buffer
 * @param       buffer - pointer to bytes for storing encoding
 * @param       value - 40-bit value to encode
 * @return      Returns the number of bytes encoded
 */
int encode_unsigned40(uint8_t *buffer, uint64_t value)
{
    if (buffer) {
        buffer[0] = (uint8_t)((value >> 32) & 0xFF);
        buffer[1] = (uint8_t)((value >> 24) & 0xFF);
        buffer[2] = (uint8_t)((value >> 16) & 0xFF);
        buffer[3] = (uint8_t)((value >> 8)  & 0xFF);
        buffer[4] = (uint8_t)((value >> 0)  & 0xFF);
    }

    return 5;
}

/**
 * @brief       Decode a 40-bit unsigned value from a buffer
 * @param       buffer - pointer to bytes for used for decoding
 * @param       value - pointer to 64-bit value to store the decoded value
 * @return      Returns the number of bytes decoded
 */
int decode_unsigned40(const uint8_t *buffer, uint64_t *value)
{
    if (buffer && value) {
        *value  = (uint64_t)buffer[0] << 32;
        *value |= (uint64_t)buffer[1] << 24;
        *value |= (uint64_t)buffer[2] << 16;
        *value |= (uint64_t)buffer[3] << 8;
        *value |= (uint64_t)buffer[4];
    }

    return 5;
}

/**
 * @brief       Encode a 48-bit unsigned value into buffer
 * @param       buffer - pointer to bytes for storing encoding
 * @param       value - 48-bit value to encode
 * @return      Returns the number of bytes encoded
 */
int encode_unsigned48(uint8_t *buffer, uint64_t value)
{
    if (buffer) {
        buffer[0] = (uint8_t)((value >> 40) & 0xFF);
        buffer[1] = (uint8_t)((value >> 32) & 0xFF);
        buffer[2] = (uint8_t)((value >> 24) & 0xFF);
        buffer[3] = (uint8_t)((value >> 16) & 0xFF);
        buffer[4] = (uint8_t)((value >> 8)  & 0xFF);
        buffer[5] = (uint8_t)((value >> 0)  & 0xFF);
    }

    return 6;
}

/**
 * @brief       Decode a 48-bit unsigned value from a buffer
 * @param       buffer - pointer to bytes for used for decoding
 * @param       value - pointer to 64-bit value to store the decoded value
 * @return      Returns the number of bytes decoded
 */
int decode_unsigned48(const uint8_t *buffer, uint64_t *value)
{
    if (buffer && value) {
        *value  = (uint64_t)buffer[0] << 40;
        *value |= (uint64_t)buffer[1] << 32;
        *value |= (uint64_t)buffer[2] << 24;
        *value |= (uint64_t)buffer[3] << 16;
        *value |= (uint64_t)buffer[4] << 8;
        *value |= (uint64_t)buffer[5];
    }

    return 6;
}

/**
 * @brief       Encode a 56-bit unsigned value into buffer
 * @param       buffer - pointer to bytes for storing encoding
 * @param       value - 56-bit value to encode
 * @return      Returns the number of bytes encoded
 */
int encode_unsigned56(uint8_t *buffer, uint64_t value)
{
    if (buffer) {
        buffer[0] = (uint8_t)((value >> 48) & 0xFF);
        buffer[1] = (uint8_t)((value >> 40) & 0xFF);
        buffer[2] = (uint8_t)((value >> 32) & 0xFF);
        buffer[3] = (uint8_t)((value >> 24) & 0xFF);
        buffer[4] = (uint8_t)((value >> 16) & 0xFF);
        buffer[5] = (uint8_t)((value >> 8)  & 0xFF);
        buffer[6] = (uint8_t)((value >> 0)  & 0xFF);
    }

    return 7;
}

/**
 * @brief       Decode a 56-bit unsigned value from a buffer
 * @param       buffer - pointer to bytes for used for decoding
 * @param       value - pointer to 64-bit value to store the decoded value
 * @return      Returns the number of bytes decoded
 */
int decode_unsigned56(const uint8_t *buffer, uint64_t *value)
{
    if (buffer && value) {
        *value  = (uint64_t)buffer[0] << 48;
        *value |= (uint64_t)buffer[1] << 40;
        *value |= (uint64_t)buffer[2] << 32;
        *value |= (uint64_t)buffer[3] << 24;
        *value |= (uint64_t)buffer[4] << 16;
        *value |= (uint64_t)buffer[5] << 8;
        *value |= (uint64_t)buffer[6];
    }

    return 7;
}

/**
 * @brief       Encode a 64-bit unsigned value into buffer
 * @param       buffer - pointer to bytes for storing encoding
 * @param       value - 16-bit value to encode
 * @return      Returns the number of bytes encoded
 */
int encode_unsigned64(uint8_t *buffer, uint64_t value)
{
    if (buffer) {
        buffer[0] = (uint8_t)((value >> 56) & 0xFF);
        buffer[1] = (uint8_t)((value >> 48) & 0xFF);
        buffer[2] = (uint8_t)((value >> 40) & 0xFF);
        buffer[3] = (uint8_t)((value >> 32) & 0xFF);
        buffer[4] = (uint8_t)((value >> 24) & 0xFF);
        buffer[5] = (uint8_t)((value >> 16) & 0xFF);
        buffer[6] = (uint8_t)((value >> 8)  & 0xFF);
        buffer[7] = (uint8_t)((value >> 0)  & 0xFF);
    }

    return 8;
}

/**
 * @brief       Decode a 64-bit unsigned value from a buffer
 * @param       buffer - pointer to bytes for used for decoding
 * @param       value - pointer to 64-bit value to store the decoded value
 * @return      Returns the number of bytes decoded
 */
int decode_unsigned64(const uint8_t *buffer, uint64_t *value)
{
    if (buffer && value) {
        *value  = (uint64_t)buffer[0] << 56;
        *value |= (uint64_t)buffer[1] << 48;
        *value |= (uint64_t)buffer[2] << 40;
        *value |= (uint64_t)buffer[3] << 32;
        *value |= (uint64_t)buffer[4] << 24;
        *value |= (uint64_t)buffer[5] << 16;
        *value |= (uint64_t)buffer[6] << 8;
        *value |= (uint64_t)buffer[7];
    }

    return 8;
}
#endif
/**
 * @brief       Determine the number of bytes in the value
 *              length of unsigned is variable, as per 20.2.4
 * @param       value - pointer to 32-bit or 64-bit value
 * @return      Returns the number of bytes
 */
int bacnet_unsigned_length(BACNET_UNSIGNED_INTEGER value)
{
    int len = 0; /* return value */

    if (value <= 0xFF) {
        len = 1;
    } else if (value <= 0xFFFF) {
        len = 2;
    } else if (value <= 0xFFFFFF) {
        len = 3;
    } else {
#ifdef UINT64_MAX
        /* Avoid ULL to be compatible with C89. */
        value = value >> 32;

        if (value == 0) {
            len = 4;
        } else if (value <= 0xFF) {
            len = 5;
        } else if (value <= 0xFFFF) {
            len = 6;
        } else if (value <= 0xFFFFFF) {
            len = 7;
        } else {
            len = 8;
        }
#else
        len = 4;
#endif
    }

    return len;
}

#if BACNET_USE_SIGNED
int encode_signed8(uint8_t *apdu, int8_t value)
{
    if (apdu) {
        apdu[0] = (uint8_t)value;
    }

    return 1;
}

int decode_signed8(const uint8_t *apdu, int32_t *value)
{
    if (apdu && value) {
        /* negative - bit 7 is set */
        if (apdu[0] & 0x80) {
            *value = 0xFFFFFF00;
        } else {
            *value = 0;
        }
        *value |= (int32_t)apdu[0] & 0xFF;
    }

    return 1;
}

int encode_signed16(uint8_t *apdu, int16_t value)
{
    if (apdu) {
        apdu[0] = (uint8_t)((value >> 8) & 0xFF);
        apdu[1] = (uint8_t)((value >> 0) & 0xFF);
    }

    return 2;
}

int decode_signed16(const uint8_t *apdu, int32_t *value)
{
    if (apdu && value) {
        /* negative - bit 7 is set */
        if (apdu[0] & 0x80) {
            *value = 0xFFFF0000;
        } else {
            *value = 0;
        }
        *value |= (int32_t)apdu[0] << 8;
        *value |= (int32_t)apdu[1];
    }

    return 2;
}

int encode_signed24(uint8_t *apdu, int32_t value)
{
    if (apdu) {
        apdu[0] = (uint8_t)((value >> 16) & 0xFF);
        apdu[1] = (uint8_t)((value >> 8)  & 0xFF);
        apdu[2] = (uint8_t)((value >> 0)  & 0xFF);
    }

    return 3;
}

int decode_signed24(const uint8_t *apdu, int32_t *value)
{
    if (apdu && value) {
        /* negative - bit 7 is set */
        if (apdu[0] & 0x80) {
            *value = 0xFF000000;
        } else {
            *value = 0;
        }
        *value |= (int32_t)apdu[0] << 16;
        *value |= (int32_t)apdu[1] << 8;
        *value |= (int32_t)apdu[2];
    }

    return 3;
}

int encode_signed32(uint8_t *apdu, int32_t value)
{
    if (apdu) {
        apdu[0] = (uint8_t)((value >> 24) & 0xFF);
        apdu[1] = (uint8_t)((value >> 16) & 0xFF);
        apdu[2] = (uint8_t)((value >> 8)  & 0xFF);
        apdu[3] = (uint8_t)((value >> 0)  & 0xFF);
    }

    return 4;
}

int decode_signed32(const uint8_t *apdu, int32_t *value)
{
    if (apdu && value) {
        *value  = (int32_t)apdu[0] << 24;
        *value |= (int32_t)apdu[1] << 16;
        *value |= (int32_t)apdu[2] << 8;
        *value |= (int32_t)apdu[3];
    }

    return 4;
}

/**
 * @brief Determines the number of bytes used by a BACnet Signed Integer
 *  from clause 20.2.5 Encoding of an Signed Integer Value
 * @param value - signed value
 * @return number of bytes used by a BACnet Signed Integer
 */
int bacnet_signed_length(int32_t value)
{
    int len;

    if ((value >= -128) && (value < 128)) {
        len = 1;
    } else if ((value >= -32768) && (value < 32768)) {
        len = 2;
    } else if ((value >= -8388608) && (value < 8388608)) {
        len = 3;
    } else {
        len = 4;
    }

    return len;
}
#endif
