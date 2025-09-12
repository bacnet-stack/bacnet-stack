/**
 * @file
 * @brief A basic You-Are service handler
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date June 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacapp.h"
#include "bacnet/youare.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"

/**
 * A basic handler for You-Are responses.
 * @param service_request [in] The received message to be handled.
 * @param service_len [in] Length of the service_request message.
 * @param src [in] The BACNET_ADDRESS of the message's source.
 */
void handler_you_are_json_print(
    uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src)
{
    int len = 0;
    uint32_t device_id = 0;
    uint16_t vendor_id = 0;
    BACNET_CHARACTER_STRING model_name = { 0 };
    BACNET_CHARACTER_STRING serial_number = { 0 };
    BACNET_OCTET_STRING mac_address = { 0 };
    char name[MAX_CHARACTER_STRING_BYTES + 1] = { 0 };

    (void)src;
    len = you_are_request_decode(
        service_request, service_len, &device_id, &vendor_id, &model_name,
        &serial_number, &mac_address);
    if (len > 0) {
        debug_printf_stdout("{\n\"You-Are-Request\": {\n");
        debug_printf_stdout(" \"vendor-id\" : %u,\n", (unsigned)vendor_id);
        bacapp_snprintf_character_string(name, sizeof(name), &model_name);
        debug_printf_stdout(" \"model-name\" : %s,\n", name);
        bacapp_snprintf_character_string(name, sizeof(name), &serial_number);
        debug_printf_stdout(" \"serial-number\" : %s", name);
        if (device_id <= BACNET_MAX_INSTANCE) {
            debug_printf_stdout(",\n");
            debug_printf_stdout(
                " \"device-identifier\" : %lu", (unsigned long)device_id);
        }
        if (mac_address.length > 0) {
            debug_printf_stdout(",\n");
            bacapp_snprintf_octet_string(name, sizeof(name), &mac_address);
            debug_printf_stdout(" \"device-mac-address\" : \"%s\"", name);
        }
        debug_printf_stdout("\n }\n}\n");
    }

    return;
}

/**
 * A basic handler for You-Are-Request
 * @param service_request [in] The received message to be handled.
 * @param service_len [in] Length of the service_request message.
 * @param src [in] The BACNET_ADDRESS of the message's source.
 */
void handler_you_are_device_id_set(
    uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src)
{
    int len = 0;
    uint32_t device_id = 0;
    uint16_t vendor_id = 0;
    BACNET_CHARACTER_STRING model_name = { 0 }, device_model_name = { 0 };
    BACNET_CHARACTER_STRING serial_number = { 0 }, device_serial_number = { 0 };

    (void)src;
    len = you_are_request_decode(
        service_request, service_len, &device_id, &vendor_id, &model_name,
        &serial_number, NULL);
    if (len > 0) {
        handler_device_character_string_get(
            PROP_MODEL_NAME, &device_model_name);
        handler_device_character_string_get(
            PROP_SERIAL_NUMBER, &device_serial_number);
        if ((handler_device_vendor_identifier() == vendor_id) &&
            characterstring_same(&device_model_name, &model_name) &&
            characterstring_same(&device_serial_number, &serial_number)) {
            handler_device_object_instance_number_set(device_id);
        }
    }

    return;
}
