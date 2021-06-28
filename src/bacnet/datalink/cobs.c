/**************************************************************************
 *
 * Copyright (C) 2014 Kerry Lynn <kerlyn@ieee.org>
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
#include <stddef.h>
#include <stdint.h>
#include "bacnet/datalink/mstpdef.h"

/*
 * Encodes 'length' octets of data located at 'from' and
 * writes one or more COBS code blocks at 'to', removing
 * any 0x55 octets that may present be in the encoded data.
 * Returns the length of the encoded data.
 * Note: This function is copied directly from the BACnet standard.
 */
size_t cobs_encode(
    uint8_t *to, const uint8_t *from, size_t length, uint8_t mask)
{
    size_t code_index = 0;
    size_t read_index = 0;
    size_t write_index = 1;
    uint8_t code = 1;
    uint8_t data, last_code;

    while (read_index < length) {
        data = from[read_index++];
        /*
         * In the case of encountering a non-zero octet in the data,
         * simply copy input to output and increment the code octet.
         */
        if (data != 0) {
            to[write_index++] = data ^ mask;
            code++;
            if (code != 255)
                continue;
        }
        /*
         * In the case of encountering a zero in the data or having
         * copied the maximum number (254) of non-zero octets, store
         * the code octet and reset the encoder state variables.
         */
        last_code = code;
        to[code_index] = code ^ mask;
        code_index = write_index++;
        code = 1;
    }
    /*
     * If the last chunk contains exactly 254 non-zero octets, then
     * this exception is handled above (and returned length must be
     * adjusted). Otherwise, encode the last chunk normally, as if
     * a "phantom zero" is appended to the data.
     */
    if ((last_code == 255) && (code == 1))
        write_index--;
    else
        to[code_index] = code ^ mask;
    return write_index;
}
/*
 * Encodes 'length' octets of client data located at 'from' and writes
 * the COBS-encoded Encoded Data and Encoded CRC-32K fields at 'to'.
 * Returns the combined length of these encoded fields.
 * Note: This function is copied directly from the BACnet standard.
 */
size_t cobs_frame_encode(uint8_t *to, const uint8_t *from, size_t length)
{
    size_t cobs_data_len, cobs_crc_len;
    uint32_t crc32K;
    int i;

    /*
     * Prepare the Encoded Data field for transmission.
     */
    cobs_data_len = cobs_encode(to, from, length, MSTP_PREAMBLE_X55);
    /*
     * Calculate CRC-32K over the Encoded Data field.
     * NOTE: May be done as each octet is transmitted to reduce latency.
     */
    crc32K = CRC32K_INITIAL_VALUE;
    for (i = 0; i < cobs_data_len; i++) {
        crc32K = CalcCRC32K(to[i], crc32K); /* See Clause G.3.1 */
    }
    /*
     * Prepare the Encoded CRC-32K field for transmission.
     * NOTE: Assumes a little-endian CPU (otherwise order the
     * octets least-significant first before encoding).
     */
    crc32K = ~crc32K;
    cobs_crc_len = cobs_encode((uint8_t *)(to + cobs_data_len),
        (const uint8_t *)&crc32K, sizeof(uint32_t), MSTP_PREAMBLE_X55);
    /*
     * Return the combined length of the Encoded Data and Encoded CRC-32K
     * fields. NOTE: Subtract two before use as the MS/TP frame Length field.
     */
    return cobs_data_len + cobs_crc_len;
}

/*
 * Decodes 'length' octets of data located at 'from' and
 * writes the original client data at 'to', restoring any
 * 'mask' octets that may present in the encoded data.
 * Returns the length of the encoded data or zero if error.
 * Note: This function is copied directly from the BACnet standard.
 */
size_t cobs_decode(
    uint8_t *to, const uint8_t *from, size_t length, uint8_t mask)
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
        if (code == 0 || read_index + code > length) {
            return 0;
        }
        read_index++;
        while (--code > 0) {
            to[write_index++] = from[read_index++] ^ mask;
        }
        /*
         * Restore the implicit zero at the end of each decoded block
         * except when it contains exactly 254 non-zero octets or the
         * end of data has been reached.
         */
        if ((last_code != 255) && (read_index < length)) {
            to[write_index++] = 0;
        }
    }
    return write_index;
}
#define COBS_ADJ_FOR_ENC_CRC (5) /* Set to 3 if passing MS/TP Length field */
#define COBS_SIZEOF_ENC_CRC (5)
/**
 * Decodes Encoded Data and Encoded CRC-32K fields at 'from' and
 * writes the decoded client data at 'to'. Assumes 'length' contains
 * the actual combined length of these fields in octets (that is, the
 * MS/TP header Length field plus two).
 * @param to - decoded
 * @param from - buffer to decode
 * @param length - number of bytes in the buffer to decode
 * @return length of decoded Data in octets or zero if error.
 * @note Safe to call with 'output' <= 'input' (decodes in place).
 * @note This function is copied directly from the BACnet standard.
 */
size_t cobs_frame_decode(uint8_t *to, const uint8_t *from, size_t length)
{
    size_t data_len, crc_len;
    uint32_t crc32K;
    int i;
    /*
     * Calculate the CRC32K over the Encoded Data octets before decoding.
     * NOTE: Adjust 'length' by removing size of Encoded CRC-32K field.
     */
    data_len = length - COBS_ADJ_FOR_ENC_CRC;
    crc32K = CRC32K_INITIAL_VALUE;
    for (i = 0; i < data_len; i++) {
        crc32K = CalcCRC32K(from[i], crc32K); /* See Clause G.3.1 */
    }
    data_len = cobs_decode(to, from, data_len, MSTP_PREAMBLE_X55);
    /*
     * Decode the Encoded CRC-32K field and append to data.
     */
    crc_len = cobs_decode((uint8_t *)(to + data_len),
        (uint8_t *)(from + length - COBS_ADJ_FOR_ENC_CRC), COBS_SIZEOF_ENC_CRC,
        MSTP_PREAMBLE_X55);
    /*
     * Sanity check length of decoded CRC32K.
     */
    if (crc_len != sizeof(uint32_t)) {
        return 0;
    }
    /*
     * Verify CRC32K of incoming frame.
     */
    for (i = 0; i < crc_len; i++) {
        crc32K = CalcCRC32K((to + data_len)[i], crc32K);
    }
    if (crc32K == CRC32K_RESIDUE) {
        return data_len;
    }

    return 0;
}
