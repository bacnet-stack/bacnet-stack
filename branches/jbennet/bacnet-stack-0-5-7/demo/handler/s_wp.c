/**************************************************************************
*
* Copyright (C) 2005 Steve Karg <skarg@users.sourceforge.net>
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
*
*********************************************************************/
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include "config.h"
#include "config.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "address.h"
#include "tsm.h"
#include "npdu.h"
#include "apdu.h"
#include "device.h"
#include "datalink.h"
#include "dcc.h"
#include "whois.h"
/* some demo stuff needed */
#include "handlers.h"
#include "session.h"
#include "clientsubscribeinvoker.h"

/** @file s_wp.c  Send a Write Property request. */

/** returns the invoke ID for confirmed request, or zero on failure */
uint8_t Send_Write_Property_Request_Data(
    struct bacnet_session_object *sess,
    struct ClientSubscribeInvoker *subscriber,
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    uint8_t * application_data,
    int application_data_len,
    uint8_t priority,
    int32_t array_index)
{
    BACNET_ADDRESS dest;
    unsigned max_apdu = 0;
    uint8_t invoke_id = 0;
    bool status = false;
    int len = 0;
    int pdu_len = 0;
    int bytes_sent = 0;
    BACNET_WRITE_PROPERTY_DATA data;
    BACNET_NPDU_DATA npdu_data;
    BACNET_APDU_FIXED_HEADER apdu_fixed_header;
    uint8_t Handler_Transmit_Buffer[MAX_PDU_SEND] = { 0 };
    uint8_t segmentation = 0;
    uint32_t maxsegments = 0;

    if (!dcc_communication_enabled(sess))
        return 0;

    /* is the device bound? */
    status =
        address_get_by_device(sess, device_id, &max_apdu, &segmentation,
        &maxsegments, &dest);
    /* is there a tsm available? */
    if (status)
        invoke_id = tsm_next_free_invokeID(sess);
    if (invoke_id) {
        npdu_encode_npdu_data(&npdu_data, true, MESSAGE_PRIORITY_NORMAL);
        apdu_init_fixed_header(&apdu_fixed_header,
            PDU_TYPE_CONFIRMED_SERVICE_REQUEST, invoke_id,
            SERVICE_CONFIRMED_WRITE_PROPERTY, max_apdu);
        /* encode the APDU portion of the packet */
        data.object_type = object_type;
        data.object_instance = object_instance;
        data.object_property = object_property;
        data.array_index = array_index;
        data.application_data_len = application_data_len;
        /* copy pointer */
        data.application_data = application_data;
        data.priority = priority;
        len =
            wp_encode_apdu(&Handler_Transmit_Buffer[0],
            sizeof(Handler_Transmit_Buffer), &data);
        pdu_len += len;
        if (len <= 0) {
            tsm_free_invoke_id_check(sess, invoke_id, NULL, true);
            return 0;
        }
        /* if a client subscriber is provided, then associate the invokeid with that client
           otherwise another thread might receive a message with this invokeid before we return
           from this function */
        if (subscriber && subscriber->SubscribeInvokeId) {
            subscriber->SubscribeInvokeId(invoke_id, subscriber->context);
        }

        /* Send data to the peer device, respecting APDU sizes, destination size,
           and segmented or unsegmented data sending possibilities */
        bytes_sent =
            tsm_set_confirmed_transaction(sess, invoke_id, &dest, &npdu_data,
            &apdu_fixed_header, &Handler_Transmit_Buffer[0], pdu_len);

        if (bytes_sent <= 0)
            invoke_id = 0;

#if PRINT_ENABLED
        if (bytes_sent <= 0)
            fprintf(stderr, "Failed to Send WriteProperty Request (%s)!\n",
                strerror(errno));
#endif
    }

    return invoke_id;
}


/** Sends a Write Property request.
 * @ingroup DSWP
 *  
 * @param device_id [in] ID of the destination device
 * @param object_type [in]  Type of the object whose property is to be written.
 * @param object_instance [in] Instance # of the object to be written.
 * @param object_property [in] Property to be written.
 * @param object_value [in] The value to be written to the property.
 * @param priority [in] Write priority of 1 (highest) to 16 (lowest)
 * @param array_index [in] Optional: if the Property is an array,  
 *        the index from 1 to n for the individual array member to be written.
 * @return invoke id of outgoing message, or 0 on failure. 
 */
uint8_t Send_Write_Property_Request(
    struct bacnet_session_object * sess,
    struct ClientSubscribeInvoker * subscriber,
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    BACNET_APPLICATION_DATA_VALUE * object_value,
    uint8_t priority,
    int32_t array_index)
{
    int apdu_len = 0, len = 0;
    uint8_t application_data[MAX_PDU_SEND] = { 0 };

    while (object_value) {
#if PRINT_ENABLED_DEBUG
        fprintf(stderr, "WriteProperty service: " "%s tag=%d\n",
            (object_value->context_specific ? "context" : "application"),
            (int) (object_value->
                context_specific ? object_value->context_tag : object_value->
                tag));
#endif
        len =
            bacapp_encode_data(&application_data[apdu_len],
            MAX_PDU_SEND - apdu_len, object_value);
        if (len < 0) {
            /* overflow ! */
            return 0;
        } else if ((len + apdu_len) < MAX_PDU_SEND) {
            apdu_len += len;
        } else {
            return 0;
        }
        object_value = object_value->next;
    }

    return Send_Write_Property_Request_Data(sess, subscriber, device_id,
        object_type, object_instance, object_property, &application_data[0],
        apdu_len, priority, array_index);
}
