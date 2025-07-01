/**
 * @file
 * @brief CreateObject service initiation
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date August 2023
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacerror.h"
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"
#include "bacnet/abort.h"
#include "bacnet/bactext.h"
#include "bacnet/dcc.h"
#include "bacnet/create_object.h"
#include "bacnet/whois.h"
/* some demo stuff needed */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"

/**
 * @brief Send a CreateObject service message
 * @param device_id [in] ID of the destination device
 * @param object_type [in]  Type of the object whose property is to be written.
 * @param object_instance [in] Instance # of the object to be written.
 * @return invoke id of outgoing message, or 0 on failure.
 * @return the invoke ID for confirmed request, or zero on failure
 */
uint8_t Send_Create_Object_Request_Data(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_VALUE *values)
{
    BACNET_ADDRESS dest;
    BACNET_ADDRESS my_address;
    unsigned max_apdu = 0;
    uint8_t invoke_id = 0;
    bool status = false;
    int len = 0;
    int pdu_len = 0;
    int bytes_sent = 0;
    BACNET_CREATE_OBJECT_DATA data = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };
    uint8_t service = SERVICE_CONFIRMED_CREATE_OBJECT;
#if BACNET_SEGMENTATION_ENABLED
    uint8_t segmentation = 0;
    uint16_t maxsegments = 0;
#endif

    if (!dcc_communication_enabled()) {
        return 0;
    }
    /* is the device bound? */
    status = address_get_by_device(
        device_id, &max_apdu, &dest
#if BACNET_SEGMENTATION_ENABLED
        ,&segmentation, &maxsegments
#endif
    );
    /* is there a tsm available? */
    if (status) {
        invoke_id = tsm_next_free_invokeID();
    }
    if (invoke_id) {
        /* encode the NPDU portion of the packet */
        datalink_get_my_address(&my_address);
        npdu_encode_npdu_data(&npdu_data, true, MESSAGE_PRIORITY_NORMAL);
        pdu_len = npdu_encode_pdu(
            &Handler_Transmit_Buffer[0], &dest, &my_address, &npdu_data);
        /* encode the APDU header portion of the packet */
        Handler_Transmit_Buffer[pdu_len++] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        Handler_Transmit_Buffer[pdu_len++] =
            encode_max_segs_max_apdu(0, MAX_APDU);
        Handler_Transmit_Buffer[pdu_len++] = invoke_id;
        Handler_Transmit_Buffer[pdu_len++] = service;
        /* encode the APDU service */
        data.object_type = object_type;
        data.object_instance = object_instance;
        data.list_of_initial_values = values;
        /* get the length of the APDU */
        len = create_object_encode_service_request(NULL, &data);
        pdu_len += len;
        /* will it fit in the sender and our buffer?
           note: if there is a bottleneck router in between
           us and the destination, we won't know unless
           we have a way to check for that and update the
           max_apdu in the address binding table. */
        if (((unsigned)pdu_len < max_apdu) &&
            (pdu_len < sizeof(Handler_Transmit_Buffer))) {
            /* shift back to the service portion of the buffer */
            pdu_len -= len;
            len = create_object_encode_service_request(
                &Handler_Transmit_Buffer[pdu_len], &data);
            pdu_len += len;
            tsm_set_confirmed_unsegmented_transaction(
                invoke_id, &dest, &npdu_data, &Handler_Transmit_Buffer[0],
                (uint16_t)pdu_len);
            bytes_sent = datalink_send_pdu(
                &dest, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
            if (bytes_sent <= 0) {
                debug_perror("CreateObject: Failed to Send");
            }
        } else {
            tsm_free_invoke_id(invoke_id);
            invoke_id = 0;
            debug_printf_stderr(
                "%s service: Failed to Send "
                "(exceeds destination maximum APDU)!\n",
                bactext_confirmed_service_name(service));
        }
    }

    return invoke_id;
}

/**
 * @brief Send a CreateObject service message
 * @param device_id [in] ID of the destination device
 * @param object_type [in]  Type of the object whose property is to be written.
 * @param object_instance [in] Instance # of the object to be written.
 * @return invoke id of outgoing message, or 0 on failure.
 * @return the invoke ID for confirmed request, or zero on failure
 */
uint8_t Send_Create_Object_Request(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance)
{
    return Send_Create_Object_Request_Data(
        device_id, object_type, object_instance, NULL);
}
