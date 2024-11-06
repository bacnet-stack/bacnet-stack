/**
 * @file
 * @brief BACnet MSTP COBS encoding for extended frames
 * @author Kerry Lynn <kerlyn@ieee.org>
 * @date 2014
 * @copyright SPDX-License-Identifier: MIT
 * @defgroup DLMSTP BACnet MS/TP DataLink Network Layer
 * @ingroup DataLink
 */
#include <stddef.h>
#include <stdint.h>
#include "bacnet/datalink/mstpdef.h"
#include "bacnet/datalink/cobs.h"

/**
 * @brief Encode the CRC32K as little-endian byte order
 * @param buffer - encoded buffer
 * @param buffer_size - encoded buffer size
 * @param dataValue new data value equivalent to one octet.
 * @param crc - the CRC32K value
 * @return number of bytes encoded
 */
size_t cobs_crc32k_encode(uint8_t *buffer, size_t buffer_size, uint32_t crc)
{
    if (buffer_size >= 4) {
        buffer[0] = (uint8_t)(crc & 0x000000ff);
        buffer[1] = (uint8_t)((crc & 0x0000ff00) >> 8);
        buffer[2] = (uint8_t)((crc & 0x00ff0000) >> 16);
        buffer[3] = (uint8_t)((crc & 0xff000000) >> 24);
        return 4;
    }

    return 0;
}

/**
 * @brief Accumulate "dataValue" into the CRC in "crc32kValue".
 * @param dataValue new data value equivalent to one octet.
 * @param crc32kValue accumulated value equivalent to four octets.
 * @return value is updated CRC.
 * @note This function is copied directly from the BACnet standard.
 */
uint32_t cobs_crc32k(uint8_t dataValue, uint32_t crc32kValue)
{
    uint8_t data, b;
    uint32_t crc;

    data = dataValue;
    crc = crc32kValue;
    for (b = 0; b < 8; b++) {
        if ((data & 1) ^ (crc & 1)) {
            crc >>= 1;
            /* CRC-32K polynomial, 1 + x**1 + ... + x**30 (+ x**32) */
            crc ^= 0xEB31D82E;
        } else {
            crc >>= 1;
        }
        data >>= 1;
    }

    return crc; /* Return updated crc value */
}

/**
 * @brief Encodes 'length' octets of data located at 'from' and
 * writes one or more COBS code blocks at 'buffer', removing
 * any 0x55 octets that may present be in the encoded data.
 * @param buffer - encoded buffer
 * @param buffer_size - encoded buffer size
 * @param from - buffer to encode
 * @param length - number of bytes in the buffer to encode
 * @return the length of the encoded data, or 0 if error
 * @note This function is copied mostly from the BACnet standard.
 */
size_t cobs_encode(
    uint8_t *buffer,
    size_t buffer_size,
    const uint8_t *from,
    size_t length,
    uint8_t mask)
{
    size_t code_index = 0;
    size_t read_index = 0;
    size_t write_index = 1;
    uint8_t code = 1;
    uint8_t data = 0;
    uint8_t last_code = 0;

    if ((buffer_size < 1) || (length < 1)) {
        /* error - buffer too small */
        return 0;
    }
    while (read_index < length) {
        data = from[read_index++];
        /*
         * In the case of encountering a non-zero octet in the data,
         * simply copy input to output and increment the code octet.
         */
        if (data != 0) {
            if (write_index == buffer_size) {
                /* error - buffer too small */
                return 0;
            }
            buffer[write_index++] = data ^ mask;
            code++;
            if (code != 255) {
                continue;
            }
        }
        /*
         * In the case of encountering a zero in the data or having
         * copied the maximum number (254) of non-zero octets, store
         * the code octet and reset the encoder state variables.
         */
        last_code = code;
        if (code_index == buffer_size) {
            /* error - buffer too small */
            return 0;
        }
        buffer[code_index] = code ^ mask;
        code_index = write_index++;
        code = 1;
    }
    /*
     * If the last chunk contains exactly 254 non-zero octets, then
     * this exception is handled above (and returned length must be
     * adjusted). Otherwise, encode the last chunk normally, as if
     * a "phantom zero" is appended to the data.
     */
    if ((last_code == 255) && (code == 1)) {
        write_index--;
    } else {
        if (code_index == buffer_size) {
            /* error - buffer too small */
            return 0;
        }
        buffer[code_index] = code ^ mask;
    }

    return write_index;
}
/**
 * @brief Encodes 'length' octets of client data located at 'from' and writes
 * the COBS-encoded Encoded Data and Encoded CRC-32K fields at 'buffer'.
 * @param buffer - encoded buffer
 * @param buffer_size - encoded buffer size
 * @param from - buffer to encode
 * @param length - number of bytes in the buffer to encode
 * @return the combined length of these encoded fields, or 0 if error
 * @note This function is copied mostly from the BACnet standard.
 */
size_t cobs_frame_encode(
    uint8_t *buffer, size_t buffer_size, const uint8_t *from, size_t length)
{
    size_t cobs_data_len, cobs_crc_len;
    uint32_t crc32K;
    uint8_t crc_buffer[4];
    int i;

    /*
     * Prepare the Encoded Data field for transmission.
     */
    cobs_data_len =
        cobs_encode(buffer, buffer_size, from, length, MSTP_PREAMBLE_X55);
    if (cobs_data_len == 0) {
        return 0;
    }
    /*
     * Calculate CRC-32K over the Encoded Data field.
     * NOTE: May be done as each octet is transmitted to reduce latency.
     */
    crc32K = CRC32K_INITIAL_VALUE;
    for (i = 0; i < cobs_data_len; i++) {
        crc32K = cobs_crc32k(buffer[i], crc32K); /* See Clause G.3.1 */
    }
    /*
     * Prepare the Encoded CRC-32K field for transmission.
     */
    crc32K = ~crc32K;
    (void)cobs_crc32k_encode(crc_buffer, sizeof(crc_buffer), crc32K);
    cobs_crc_len = cobs_encode(
        (uint8_t *)(buffer + cobs_data_len), buffer_size - cobs_data_len,
        crc_buffer, sizeof(crc_buffer), MSTP_PREAMBLE_X55);
    if (cobs_crc_len == 0) {
        return 0;
    }
    /*
     * Return the combined length of the Encoded Data and Encoded CRC-32K
     * fields. NOTE: Subtract two before use as the MS/TP frame Length field.
     */
    return cobs_data_len + cobs_crc_len;
}

/**
 * @brief Decodes 'length' octets of data located at 'from' and
 * writes the original client data at 'buffer', restoring any
 * 'mask' octets that may present in the encoded data.
 * @param buffer - decoded buffer
 * @param buffer_size - decoded buffer size
 * @param from - buffer to decode
 * @param length - number of bytes in the buffer to decode
 * @return the length of the decoded buffer, or 0 if error
 * @note This function is copied directly from the BACnet standard.
 */
size_t cobs_decode(
    uint8_t *buffer,
    size_t buffer_size,
    const uint8_t *from,
    size_t length,
    uint8_t mask)
{
    size_t read_index = 0;
    size_t write_index = 0;
    uint8_t code, last_code;

    while (read_index < length) {
        code = from[read_index] ^ mask;
        last_code = code;
        /*
         * Sanity check the encoding to prevent the while() loop below
         * from overrunning the output buffer.
         */
        if ((code == 0) || ((read_index + code) > length)) {
            return 0;
        }
        read_index++;
        while (--code > 0) {
            if (write_index == buffer_size) {
                /* error - destination buffer too small */
                return 0;
            }
            if (read_index == length) {
                /* error - source buffer too small */
                return 0;
            }
            buffer[write_index++] = from[read_index++] ^ mask;
        }
        /*
         * Restore the implicit zero at the end of each decoded block
         * except when it contains exactly 254 non-zero octets or the
         * end of data has been reached.
         */
        if ((last_code != 255) && (read_index < length)) {
            if (write_index == buffer_size) {
                /* error - destination buffer too small */
                return 0;
            }
            buffer[write_index++] = 0;
        }
    }

    return write_index;
}

/**
 * Decodes Encoded Data and Encoded CRC-32K fields at 'from' and
 * writes the decoded client data at 'buffer'. Assumes 'length' contains
 * the actual combined length of these fields in octets (that is, the
 * MS/TP header Length field plus two).
 * @param buffer - decoded buffer
 * @param buffer_size - decoded buffer size
 * @param from - frame to decode
 * @param length - number of bytes in the frame to decode
 * @return length of decoded frame in octets or zero if error.
 * @note Safe to call with 'output' <= 'input' (decodes in place).
 * @note This function is copied directly from the BACnet standard.
 */
size_t cobs_frame_decode(
    uint8_t *buffer, size_t buffer_size, const uint8_t *from, size_t length)
{
    size_t data_len, crc_len;
    uint32_t crc32K;
    uint8_t crc_buffer[4];
    int i;

    if (length < COBS_ENCODED_CRC_SIZE) {
        /* error during decode */
        return 0;
    }
    /*
     * Calculate the CRC32K over the Encoded Data octets before decoding.
     * NOTE: Adjust 'length' by removing size of Encoded CRC-32K field.
     */
    data_len = length - COBS_ENCODED_CRC_SIZE;
    crc32K = CRC32K_INITIAL_VALUE;
    for (i = 0; i < data_len; i++) {
        /* See Clause G.3.1 */
        crc32K = cobs_crc32k(from[i], crc32K);
    }
    data_len =
        cobs_decode(buffer, buffer_size, from, data_len, MSTP_PREAMBLE_X55);
    if (data_len == 0) {
        /* error during decode */
        return 0;
    }
    /*
     * Decode the Encoded CRC-32K field
     */
    crc_len = cobs_decode(
        crc_buffer, sizeof(crc_buffer), from + length - COBS_ENCODED_CRC_SIZE,
        COBS_ENCODED_CRC_SIZE, MSTP_PREAMBLE_X55);
    /*
     * Sanity check length of decoded CRC32K.
     */
    if (crc_len != sizeof(uint32_t)) {
        return 0;
    }
    /*
     * Continue to verify CRC32K of incoming frame.
     */
    for (i = 0; i < crc_len; i++) {
        crc32K = cobs_crc32k(crc_buffer[i], crc32K);
    }
    if (crc32K == CRC32K_RESIDUE) {
        return data_len;
    }

    return 0;
}
