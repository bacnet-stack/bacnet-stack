/**
 * @file
 * @brief Segment Acknowledgment (SegmentAck) PDU encode and decode functions
 * @author Julien Bennet <antibarbie@users.sourceforge.net>
 * @date 2010
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_SEGMENT_ACK_H
#define BACNET_SEGMENT_ACK_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int segmentack_encode_apdu(
    uint8_t *apdu,
    bool negativeack,
    bool server,
    uint8_t invoke_id,
    uint8_t sequence_number,
    uint8_t actual_window_size);

BACNET_STACK_EXPORT
int segmentack_decode_service_request(
    uint8_t *apdu,
    unsigned apdu_len,
    uint8_t *invoke_id,
    uint8_t *sequence_number,
    uint8_t *actual_window_size);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* BACNET_SEGMENT_ACK_H */
