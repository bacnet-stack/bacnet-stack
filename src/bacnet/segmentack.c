/**
 * @file
 * @brief Segment Acknowledgment (SegmentAck) PDU encode and decode functions
 * @author Julien Bennet <antibarbie@users.sourceforge.net>
 * @date 2010
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include "bacnet/segmentack.h"

/**
 * @brief Method to encode the segment ack .
 * @param apdu[in]  Pointer to the buffer for encoding.
 * @param negativeack[in]  Acknowedlegment for the segment.
 * @param server[in]  Set to True if the acknowledgment is from the server, else
 * false.
 * @param invoke_id[in]  Invoke Id
 * @param sequence_number[in]  Sequence number of the segment to be acknowledged
 * @param actual_window_size[in]  Actual window size.
 * @return Length of encoded data or zero on error.
 */
int segmentack_encode_apdu(
    uint8_t *apdu,
    bool negativeack,
    bool server,
    uint8_t invoke_id,
    uint8_t sequence_number,
    uint8_t actual_window_size)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    uint8_t server_code = server ? 0x01 : 0x00;
    uint8_t nak_code = negativeack ? 0x02 : 0x00;

    if (apdu) {
        apdu[0] = PDU_TYPE_SEGMENT_ACK | server_code | nak_code;
        apdu[1] = invoke_id;
        apdu[2] = sequence_number;
        apdu[3] = actual_window_size;
        apdu_len = 4;
    }

    return apdu_len;
}

/**
 * @brief Method to decode the segment ack service request
 * @param apdu[in] The apdu portion of the ACK reply.
 * @param apdu_len[in] The total length of the apdu.
 * @param invoke_id[in]  Invoke Id of the request.
 * @param sequence_number[in]  Sequence number of the segment received.
 * @param actual_window_size[in]  Actual window size.
 * @return Length of decoded data or zero on error.
 */
int segmentack_decode_service_request(
    uint8_t *apdu,
    unsigned apdu_len,
    uint8_t *invoke_id,
    uint8_t *sequence_number,
    uint8_t *actual_window_size)
{
    int len = 0;
    int apdu_header_size = 3;

    if (apdu_len >= apdu_header_size) {
        if (invoke_id) {
            *invoke_id = apdu[0];
        }
        if (sequence_number) {
            *sequence_number = apdu[1];
        }
        if (actual_window_size) {
            *actual_window_size = apdu[2];
        }
    }

    return len;
}
