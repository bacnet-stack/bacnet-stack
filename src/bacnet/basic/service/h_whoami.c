/**
 * @file
 * @brief A basic Who-Am-I service handler
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
#include "bacnet/whoami.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"

/**
 * A basic handler for Who-Am-I responses.
 * @param service_request [in] The received message to be handled.
 * @param service_len [in] Length of the service_request message.
 * @param src [in] The BACNET_ADDRESS of the message's source.
 */
void handler_who_am_i_json_print(
    uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src)
{
    int len = 0;
    uint16_t vendor_id = 0;
    BACNET_CHARACTER_STRING model_name = { 0 };
    BACNET_CHARACTER_STRING serial_number = { 0 };
    char name[MAX_CHARACTER_STRING_BYTES + 1] = { 0 };

    (void)src;
    len = who_am_i_request_decode(
        service_request, service_len, &vendor_id, &model_name, &serial_number);
    if (len > 0) {
        debug_printf_stdout("{\n\"Who-Am-I-Request\": {\n");
        debug_printf_stdout(" \"vendor-id\" : %u,\n", (unsigned)vendor_id);
        bacapp_snprintf_character_string(name, sizeof(name), &model_name);
        debug_printf_stdout(" \"model-name\" : %s,\n", name);
        bacapp_snprintf_character_string(name, sizeof(name), &serial_number);
        debug_printf_stdout(" \"serial-number\" : %s", name);
        debug_printf_stdout("\n }\n}\n");
    }

    return;
}
