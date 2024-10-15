/**
 * @file
 * @brief BACnet APDU structures
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_APDU_H
#define BACNET_APDU_H
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

typedef struct _confirmed_service_data {
    bool segmented_message;
    bool more_follows;
    bool segmented_response_accepted;
    int max_segs;
    int max_resp;
    uint8_t invoke_id;
    uint8_t sequence_number;
    uint8_t proposed_window_number;
    uint8_t priority;
} BACNET_CONFIRMED_SERVICE_DATA;

typedef struct _confirmed_service_ack_data {
    bool segmented_message;
    bool more_follows;
    uint8_t invoke_id;
    uint8_t sequence_number;
    uint8_t proposed_window_number;
} BACNET_CONFIRMED_SERVICE_ACK_DATA;

uint8_t apdu_network_priority(void);
void apdu_network_priority_set(uint8_t pri);

#endif
