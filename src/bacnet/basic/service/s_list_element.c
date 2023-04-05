/**
 * @file
 * @brief AddListElement and RemoveListElement service initiation
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date January 2023
 * @section LICENSE
 *
 * Copyright (C) 2023 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "bacnet/config.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacerror.h"
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"
#include "bacnet/abort.h"
#include "bacnet/bactext.h"
#include "bacnet/dcc.h"
#include "bacnet/list_element.h"
#include "bacnet/whois.h"
/* some demo stuff needed */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"

/**
 * @brief Send a ListElement service message
 * @param service [in] AddListElement or RemoveListElement
 * @param device_id [in] ID of the destination device
 * @param object_type [in]  Type of the object whose property is to be written.
 * @param object_instance [in] Instance # of the object to be written.
 * @param object_property [in] Property to be written.
 * @param object_value [in] The value to be written to the property.
 * @param array_index [in] Optional: if the Property is an array,
 *   - 0 for the array size
 *   - 1 to n for individual array members
 *   - BACNET_ARRAY_ALL (~0) for the array value to be ignored (not sent)
 * @return invoke id of outgoing message, or 0 on failure.
 * @return the invoke ID for confirmed request, or zero on failure
 */
uint8_t Send_List_Element_Request_Data(
    BACNET_CONFIRMED_SERVICE service,
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    uint8_t *application_data,
    int application_data_len,
    uint32_t array_index)
{
    BACNET_ADDRESS dest;
    BACNET_ADDRESS my_address;
    unsigned max_apdu = 0;
    uint8_t invoke_id = 0;
    bool status = false;
    int len = 0;
    int pdu_len = 0;
    int bytes_sent = 0;
    BACNET_LIST_ELEMENT_DATA data;
    BACNET_NPDU_DATA npdu_data;

    if (!dcc_communication_enabled()) {
        return 0;
    }
    /* is the device bound? */
    status = address_get_by_device(device_id, &max_apdu, &dest);
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
        data.object_property = object_property;
        data.array_index = array_index;
        data.application_data_len = application_data_len;
        data.application_data = application_data;
        len = list_element_encode_service_request(
            &Handler_Transmit_Buffer[pdu_len], &data);
        pdu_len += len;
        /* will it fit in the sender?
           note: if there is a bottleneck router in between
           us and the destination, we won't know unless
           we have a way to check for that and update the
           max_apdu in the address binding table. */
        if ((unsigned)pdu_len < max_apdu) {
            tsm_set_confirmed_unsegmented_transaction(invoke_id, &dest,
                &npdu_data, &Handler_Transmit_Buffer[0], (uint16_t)pdu_len);
            bytes_sent = datalink_send_pdu(
                &dest, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
            if (bytes_sent <= 0) {
                debug_perror("%s service: Failed to Send %i/%i (%s)!\n",
                    bactext_confirmed_service_name(service), bytes_sent,
                    pdu_len, strerror(errno));
            }
        } else {
            tsm_free_invoke_id(invoke_id);
            invoke_id = 0;
            debug_perror("%s service: Failed to Send "
                "(exceeds destination maximum APDU)!\n",
                bactext_confirmed_service_name(service));
        }
    }

    return invoke_id;
}

/**
 * @brief Send an AddListElement service message
 * @param device_id [in] ID of the destination device
 * @param object_type [in]  Type of the object whose property is to be written.
 * @param object_instance [in] Instance # of the object to be written.
 * @param object_property [in] Property to be written.
 * @param object_value [in] The value to be written to the property.
 * @param array_index [in] Optional: if the Property is an array,
 *   - 0 for the array size
 *   - 1 to n for individual array members
 *   - BACNET_ARRAY_ALL (~0) for the array value to be ignored (not sent)
 * @return invoke id of outgoing message, or 0 on failure.
 * @return the invoke ID for confirmed request, or zero on failure
 */
uint8_t Send_Add_List_Element_Request_Data(uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    uint8_t *application_data,
    int application_data_len,
    uint32_t array_index)
{
    return Send_List_Element_Request_Data(
        SERVICE_CONFIRMED_ADD_LIST_ELEMENT,
        device_id,
        object_type,
        object_instance,
        object_property,
        application_data,
        application_data_len,
        array_index);

}

/**
 * @brief Send a RemoveListElement service message
 * @param device_id [in] ID of the destination device
 * @param object_type [in]  Type of the object whose property is to be written.
 * @param object_instance [in] Instance # of the object to be written.
 * @param object_property [in] Property to be written.
 * @param object_value [in] The value to be written to the property.
 * @param array_index [in] Optional: if the Property is an array,
 *   - 0 for the array size
 *   - 1 to n for individual array members
 *   - BACNET_ARRAY_ALL (~0) for the array value to be ignored (not sent)
 * @return invoke id of outgoing message, or 0 on failure.
 * @return the invoke ID for confirmed request, or zero on failure
 */
uint8_t Send_Remove_List_Element_Request_Data(uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    uint8_t *application_data,
    int application_data_len,
    uint32_t array_index)
{
    return Send_List_Element_Request_Data(
        SERVICE_CONFIRMED_REMOVE_LIST_ELEMENT,
        device_id,
        object_type,
        object_instance,
        object_property,
        application_data,
        application_data_len,
        array_index);

}

/**
 * @brief Sends an AddListElement service message
 * @param device_id [in] ID of the destination device
 * @param object_type [in]  Type of the object whose property is to be written.
 * @param object_instance [in] Instance # of the object to be written.
 * @param object_property [in] Property to be written.
 * @param object_value [in] The value to be written to the property.
 * @param array_index [in] Optional: if the Property is an array,
 *   - 0 for the array size
 *   - 1 to n for individual array members
 *   - BACNET_ARRAY_ALL (~0) for the array value to be ignored (not sent)
 * @return invoke id of outgoing message, or 0 on failure.
 */
uint8_t Send_Add_List_Element_Request(uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    BACNET_APPLICATION_DATA_VALUE *object_value,
    uint32_t array_index)
{
    uint8_t application_data[MAX_APDU] = { 0 };
    int apdu_len = 0, len = 0;

    while (object_value) {
        debug_printf("AddListElement service: "
                     "%s tag=%d\n",
            (object_value->context_specific ? "context" : "application"),
            (int)(object_value->context_specific ? object_value->context_tag
                                                 : object_value->tag));
        len = bacapp_encode_data(&application_data[apdu_len], object_value);
        if ((len + apdu_len) < MAX_APDU) {
            apdu_len += len;
        } else {
            return 0;
        }
        object_value = object_value->next;
    }

    return Send_Add_List_Element_Request_Data(device_id, object_type,
        object_instance, object_property, &application_data[0], apdu_len,
        array_index);
}

/**
 * @brief Sends an RemoveListElement service message
 * @param device_id [in] ID of the destination device
 * @param object_type [in]  Type of the object whose property is to be written.
 * @param object_instance [in] Instance # of the object to be written.
 * @param object_property [in] Property to be written.
 * @param object_value [in] The value to be written to the property.
 * @param array_index [in] Optional: if the Property is an array,
 *   - 0 for the array size
 *   - 1 to n for individual array members
 *   - BACNET_ARRAY_ALL (~0) for the array value to be ignored (not sent)
 * @return invoke id of outgoing message, or 0 on failure.
 */
uint8_t Send_Remove_List_Element_Request(uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    BACNET_APPLICATION_DATA_VALUE *object_value,
    uint32_t array_index)
{
    uint8_t application_data[MAX_APDU] = { 0 };
    int apdu_len = 0, len = 0;

    while (object_value) {
        debug_perror("RemoveListElement service: "
                     "%s tag=%d\n",
            (object_value->context_specific ? "context" : "application"),
            (int)(object_value->context_specific ? object_value->context_tag
                                                 : object_value->tag));
        len = bacapp_encode_data(&application_data[apdu_len], object_value);
        if ((len + apdu_len) < MAX_APDU) {
            apdu_len += len;
        } else {
            return 0;
        }
        object_value = object_value->next;
    }

    return Send_Remove_List_Element_Request_Data(device_id, object_type,
        object_instance, object_property, &application_data[0], apdu_len,
        array_index);
}
