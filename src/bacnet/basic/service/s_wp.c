/**
 * @file
 * @brief Send BACnet WriteProperty-Request
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2005
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/dcc.h"
#include "bacnet/whois.h"
/* some demo stuff needed */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"

static bool Debug_Enabled;

/**
 * @brief Enable/Disable debug output for WriteProperty requests
 * @param enable [in] true to enable, false to disable
 */
void Send_Write_Property_Debug(bool enable)
{
    Debug_Enabled = enable;
}

/**
 * @brief Send a WriteProperty-Request service message
 * @ingroup BIBB-DS-WP-A
 * @param device_id [in] ID of the destination device
 * @param data [in] The data representing the Life Safety Operation
 * @return invoke id of outgoing message, or zero on failure.
 */
uint8_t Send_Write_Property_Request_Data_Address(
    BACNET_ADDRESS *dest,
    uint16_t max_apdu,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    const uint8_t *application_data,
    int application_data_len,
    uint8_t priority,
    uint32_t array_index)
{
    BACNET_ADDRESS my_address;
    uint8_t invoke_id = 0;
    int len = 0;
    int pdu_len = 0;
    int bytes_sent = 0;
    BACNET_WRITE_PROPERTY_DATA data;
    BACNET_NPDU_DATA npdu_data;

    if (!dcc_communication_enabled()) {
        return 0;
    }

    invoke_id = tsm_next_free_invokeID();
    if (invoke_id) {
        /* encode the NPDU portion of the packet */
        datalink_get_my_address(&my_address);
        npdu_encode_npdu_data(&npdu_data, true, MESSAGE_PRIORITY_NORMAL);
        pdu_len = npdu_encode_pdu(
            &Handler_Transmit_Buffer[0], dest, &my_address, &npdu_data);
        /* encode the APDU portion of the packet */
        data.object_type = object_type;
        data.object_instance = object_instance;
        data.object_property = object_property;
        data.array_index = array_index;
        data.application_data_len = application_data_len;
        memcpy(
            &data.application_data[0], &application_data[0],
            application_data_len);
        data.priority = priority;
        len =
            wp_encode_apdu(&Handler_Transmit_Buffer[pdu_len], invoke_id, &data);
        pdu_len += len;
        /* will it fit in the sender?
           note: if there is a bottleneck router in between
           us and the destination, we won't know unless
           we have a way to check for that and update the
           max_apdu in the address binding table. */
        if ((unsigned)pdu_len < max_apdu) {
            tsm_set_confirmed_unsegmented_transaction(
                invoke_id, dest, &npdu_data, &Handler_Transmit_Buffer[0],
                (uint16_t)pdu_len);
            bytes_sent = datalink_send_pdu(
                dest, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
            if (bytes_sent <= 0) {
                debug_perror("Failed to Send WriteProperty Request");
            }
        } else {
            tsm_free_invoke_id(invoke_id);
            invoke_id = 0;
            debug_fprintf(
                stderr,
                "Failed to Send WriteProperty Request "
                "(exceeds destination maximum APDU)!\n");
        }
    }

    return invoke_id;
}
/**
 * @brief Send a WriteProperty-Request service message
 * @ingroup BIBB-DS-WP-A
 * @param device_id [in] ID of the destination device
 * @param data [in] The data representing the Life Safety Operation
 * @return invoke id of outgoing message, or zero on failure.
 */
uint8_t Send_Write_Property_Request_Data(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    const uint8_t *application_data,
    int application_data_len,
    uint8_t priority,
    uint32_t array_index)
{
    BACNET_ADDRESS dest = { 0 };
    unsigned max_apdu = 0;
    uint8_t invoke_id = 0;
    bool status = false;

    /* is the device bound? */
    status = address_get_by_device(device_id, &max_apdu, &dest);
    if (status) {
        invoke_id = Send_Write_Property_Request_Data_Address(
            &dest, max_apdu, object_type, object_instance, object_property,
            application_data, application_data_len, priority, array_index);
    }

    return invoke_id;
}

/**
 * @brief Encode the object_value and store it into application_data.
 * @param application_data  Buffer to store the encoded data
 * only
 * @param object_value  Pointer to the application value structure
 * @return Length of the encoded data in bytes, or 0 if it does not fit in
 * application_data
 */
static int encode_object_value(
    const BACNET_APPLICATION_DATA_VALUE *object_value,
    uint8_t *application_data,
    const int application_data_len)
{
    int apdu_len = 0, len = 0;
    while (object_value) {
        if (Debug_Enabled) {
            debug_printf(
                "WriteProperty service: "
                "%s tag=%d\n",
                (object_value->context_specific ? "context" : "application"),
                (int)(object_value->context_specific ? object_value->context_tag
                                                     : object_value->tag));
        }
        len = bacapp_encode_data(&application_data[apdu_len], object_value);
        if ((len + apdu_len) < application_data_len) {
            apdu_len += len;
        } else {
            return 0;
        }
        object_value = object_value->next;
    }
    return apdu_len;
}

/**
 * @brief Sends a Write Property request.
 * @ingroup BIBB-DS-WP-A
 * @param device_id [in] ID of the destination device
 * @param object_type [in]  Type of the object whose property is to be written.
 * @param object_instance [in] Instance # of the object to be written.
 * @param object_property [in] Property to be written.
 * @param object_value [in] The value to be written to the property.
 * @param priority [in] Write priority of 1 (highest) to 16 (lowest)
 * @param array_index [in] Optional: if the Property is an array,
 *   - 0 for the array size
 *   - 1 to n for individual array members
 *   - BACNET_ARRAY_ALL (~0) for the array value to be ignored (not sent)
 * @return invoke id of outgoing message, or 0 on failure.
 */
BACNET_STACK_EXPORT
uint8_t Send_Write_Property_Request_Address(
    BACNET_ADDRESS *dest,
    uint16_t max_apdu,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    const BACNET_APPLICATION_DATA_VALUE *object_value,
    uint8_t priority,
    uint32_t array_index)
{
    uint8_t application_data[MAX_APDU] = { 0 };
    int apdu_len = 0;

    apdu_len = encode_object_value(
        object_value, application_data, sizeof application_data);

    return Send_Write_Property_Request_Data_Address(
        dest, max_apdu, object_type, object_instance, object_property,
        &application_data[0], apdu_len, priority, array_index);
}

/**
 * @brief Sends a Write Property request.
 * @ingroup BIBB-DS-WP-A
 * @param device_id [in] ID of the destination device
 * @param object_type [in]  Type of the object whose property is to be written.
 * @param object_instance [in] Instance # of the object to be written.
 * @param object_property [in] Property to be written.
 * @param object_value [in] The value to be written to the property.
 * @param priority [in] Write priority of 1 (highest) to 16 (lowest)
 * @param array_index [in] Optional: if the Property is an array,
 *   - 0 for the array size
 *   - 1 to n for individual array members
 *   - BACNET_ARRAY_ALL (~0) for the array value to be ignored (not sent)
 * @return invoke id of outgoing message, or 0 on failure.
 */
uint8_t Send_Write_Property_Request(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    const BACNET_APPLICATION_DATA_VALUE *object_value,
    uint8_t priority,
    uint32_t array_index)
{
    uint8_t application_data[MAX_APDU] = { 0 };
    int apdu_len = 0;

    apdu_len = encode_object_value(
        object_value, application_data, sizeof application_data);

    return Send_Write_Property_Request_Data(
        device_id, object_type, object_instance, object_property,
        &application_data[0], apdu_len, priority, array_index);
}
