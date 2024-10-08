/**************************************************************************
 *
 * Copyright (C) 2007 Steve Karg
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 *
 *********************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/dcc.h"
#include "bacnet/basic/services.h"
/* me */
#include "bacnet/apdu.h"

uint16_t apdu_timeout(void)
{
    return 3000;
}

uint8_t apdu_retries(void)
{
    return 3;
}

bool apdu_service_supported(BACNET_SERVICES_SUPPORTED service_supported)
{
    bool status = false;

    switch (service_supported) {
        case SERVICE_SUPPORTED_READ_PROPERTY:
        case SERVICE_SUPPORTED_WHO_IS:
        case SERVICE_CONFIRMED_REINITIALIZE_DEVICE:
        case SERVICE_SUPPORTED_WRITE_PROPERTY:
        case SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL:
            status = true;
            break;
        default:
            break;
    }

    return status;
}

uint16_t apdu_decode_confirmed_service_request(uint8_t *apdu, /* APDU data */
    uint16_t apdu_len,
    BACNET_CONFIRMED_SERVICE_DATA *service_data,
    uint8_t *service_choice,
    uint8_t **service_request,
    uint16_t *service_request_len)
{
    uint16_t len = 0; /* counts where we are in PDU */

    service_data->segmented_message = (apdu[0] & BIT(3)) ? true : false;
    service_data->more_follows = (apdu[0] & BIT(2)) ? true : false;
    service_data->segmented_response_accepted =
        (apdu[0] & BIT(1)) ? true : false;
    service_data->max_segs = decode_max_segs(apdu[1]);
    service_data->max_resp = decode_max_apdu(apdu[1]);
    service_data->invoke_id = apdu[2];
    len = 3;
    if (service_data->segmented_message) {
        service_data->sequence_number = apdu[len++];
        service_data->proposed_window_number = apdu[len++];
    }
    *service_choice = apdu[len++];
    *service_request = &apdu[len];
    *service_request_len = apdu_len - len;

    return len;
}

/* When network communications are completely disabled,
   only DeviceCommunicationControl and ReinitializeDevice APDUs
   shall be processed and no messages shall be initiated.
   When the initiation of communications is disabled,
   all APDUs shall be processed and responses returned as
   required... */
static bool apdu_confirmed_dcc_disabled(uint8_t service_choice)
{
    bool status = false;

    if (dcc_communication_disabled()) {
        switch (service_choice) {
            case SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL:
            case SERVICE_CONFIRMED_REINITIALIZE_DEVICE:
                break;
            default:
                status = true;
                break;
        }
    }

    return status;
}

/* When network communications are completely disabled,
   only DeviceCommunicationControl and ReinitializeDevice APDUs
   shall be processed and no messages shall be initiated. */
/* If the request is valid and the 'Enable/Disable' parameter is
   DISABLE_INITIATION, the responding BACnet-user shall
   discontinue the initiation of messages except for I-Am
   requests issued in accordance with the Who-Is service procedure.*/
static bool apdu_unconfirmed_dcc_disabled(uint8_t service_choice)
{
    bool status = false;

    if (dcc_communication_disabled()) {
        /* there are no Unconfirmed messages that
           can be processed in this state */
        status = true;
    } else if (dcc_communication_initiation_disabled()) {
        /* WhoIs will be processed and I-Am initiated as response. */
        switch (service_choice) {
            case SERVICE_UNCONFIRMED_WHO_IS:
            case SERVICE_UNCONFIRMED_WHO_HAS:
                break;
            default:
                status = true;
                break;
        }
    }

    return status;
}

void apdu_handler(BACNET_ADDRESS *src,
    uint8_t *apdu, /* APDU data */
    uint16_t apdu_len)
{
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    uint8_t service_choice = 0;
    uint8_t *service_request = NULL;
    uint16_t service_request_len = 0;
    uint16_t len = 0; /* counts where we are in PDU */

    if (apdu) {
        /* PDU Type */
        switch (apdu[0] & 0xF0) {
            case PDU_TYPE_CONFIRMED_SERVICE_REQUEST:
                len = apdu_decode_confirmed_service_request(
                    &apdu[0], /* APDU data */
                    apdu_len, &service_data, &service_choice, &service_request,
                    &service_request_len);
                if (len == 0) {
                    /* service data unable to be decoded - simply drop */
                    break;
                }
                if (apdu_confirmed_dcc_disabled(service_choice)) {
                    /* When network communications are completely disabled,
                       only DeviceCommunicationControl and ReinitializeDevice
                       APDUs shall be processed and no messages shall be
                       initiated. */
                    break;
                }
                if (service_choice == SERVICE_CONFIRMED_READ_PROPERTY) {
                    handler_read_property(service_request, service_request_len,
                        src, &service_data);
                } else if (service_choice == SERVICE_CONFIRMED_WRITE_PROPERTY) {
                    handler_write_property(service_request, service_request_len,
                        src, &service_data);
                } else if (service_choice ==
                    SERVICE_CONFIRMED_REINITIALIZE_DEVICE) {
                    handler_reinitialize_device(service_request,
                        service_request_len, src, &service_data);
                } else if (service_choice ==
                    SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL) {
                    handler_device_communication_control(service_request,
                        service_request_len, src, &service_data);
                } else {
                    handler_unrecognized_service(service_request,
                        service_request_len, src, &service_data);
                }
                break;
            case PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST:
                service_choice = apdu[1];
                service_request = &apdu[2];
                service_request_len = apdu_len - 2;
                if (apdu_unconfirmed_dcc_disabled(service_choice)) {
                    /* When network communications are disabled,
                       only DeviceCommunicationControl and ReinitializeDevice
                       APDUs shall be processed and no messages shall be
                       initiated. If communications have been initiation
                       disabled, then WhoIs may be processed. */
                    break;
                }
                if (service_choice == SERVICE_UNCONFIRMED_WHO_IS) {
                    handler_who_is(service_request, service_request_len, src);
                }
                break;
            case PDU_TYPE_SIMPLE_ACK:
            case PDU_TYPE_COMPLEX_ACK:
            case PDU_TYPE_SEGMENT_ACK:
            case PDU_TYPE_ERROR:
            case PDU_TYPE_REJECT:
            case PDU_TYPE_ABORT:
            default:
                break;
        }
    }
    return;
}
