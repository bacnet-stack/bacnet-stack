/**
 * @file
 * @brief A basic GetEventNotification-ACK service handling
 * @details The GetEventInformation service ACK service handler is used
 * by a client BACnet-user to obtain a summary of all "active event states".
 * The term "active event states" refers to all event-initiating objects
 * that have an Event_State property whose value is not equal to NORMAL,
 * or have an Acked_Transitions property, which has at least one of the bits
 * (TO-OFFNORMAL, TO-FAULT, TONORMAL) set to FALSE.
 * @author Daniel Blazevic <daniel.blazevic@gmail.com>
 * @date 2014
 * @copyright SPDX-License-Identifier: MIT
 */
#include <assert.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacerror.h"
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"
#include "bacnet/abort.h"
#include "bacnet/getevent.h"
/* basic services */
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/services.h"

/* 40 = min size of get event data in APDU */
#define MAX_NUMBER_OF_EVENTS ((MAX_APDU / 40) + 1)

/** Example function to handle a GetEvent ACK.
 *
 * @param service_request [in] The contents of the service request.
 * @param service_len [in] The length of the service_request.
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param service_data [in] The BACNET_CONFIRMED_SERVICE_ACK_DATA information
 * decoded from the APDU header of this message.
 */
void get_event_ack_handler(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_ACK_DATA *service_data)
{
    uint16_t apdu_len = 0;
    bool more_events = false;
    /* initialize array big enough to accommodate
       multiple get event data in APDU */
    BACNET_GET_EVENT_INFORMATION_DATA get_event_data[MAX_NUMBER_OF_EVENTS] = {
        0
    };

    (void)src;
    (void)service_data;
    getevent_information_link_array(get_event_data, ARRAY_SIZE(get_event_data));
    apdu_len = getevent_ack_decode_service_request(
        &service_request[0], service_len, &get_event_data[0], &more_events);

    if (apdu_len > 0) {
        /* FIXME: Add code to process get_event_data */
    }
}
