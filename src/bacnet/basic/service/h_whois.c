/**
 * @file
 * @brief A basic Who-Is service handler
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2005
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/whois.h"
#include "bacnet/iam.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"

/** @file h_whois.c  Handles Who-Is requests. */

/** Handler for Who-Is requests, with broadcast I-Am response.
 * @ingroup DMDDB
 * @param service_request [in] The received message to be handled.
 * @param service_len [in] Length of the service_request message.
 * @param src [in] The BACNET_ADDRESS of the message's source (ignored).
 */
void handler_who_is(
    uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src)
{
    int len = 0;
    int32_t low_limit = 0;
    int32_t high_limit = 0;

    (void)src;
    len = whois_decode_service_request(
        service_request, service_len, &low_limit, &high_limit);
    if (len == 0) {
        Send_I_Am_Broadcast(&Handler_Transmit_Buffer[0]);
    } else if (len != BACNET_STATUS_ERROR) {
        /* is my device id within the limits? */
        if ((handler_device_object_instance_number() >= (uint32_t)low_limit) &&
            (handler_device_object_instance_number() <= (uint32_t)high_limit)) {
            Send_I_Am_Broadcast(&Handler_Transmit_Buffer[0]);
        }
    }

    return;
}

/** Handler for Who-Is requests, with Unicast I-Am response (per Addendum
 * 135-2004q).
 * @ingroup DMDDB
 * @param service_request [in] The received message to be handled.
 * @param service_len [in] Length of the service_request message.
 * @param src [in] The BACNET_ADDRESS of the message's source that the
 *                 response will be sent back to.
 */
void handler_who_is_unicast(
    uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src)
{
    int len = 0;
    int32_t low_limit = 0;
    int32_t high_limit = 0;

    len = whois_decode_service_request(
        service_request, service_len, &low_limit, &high_limit);
    if (len == 0) {
        /* If no limits, then always respond */
        Send_I_Am_Unicast(&Handler_Transmit_Buffer[0], src);
    } else if (len != BACNET_STATUS_ERROR) {
        /* is my device id within the limits? */
        if ((handler_device_object_instance_number() >= (uint32_t)low_limit) &&
            (handler_device_object_instance_number() <= (uint32_t)high_limit)) {
            Send_I_Am_Unicast(&Handler_Transmit_Buffer[0], src);
        }
    }

    return;
}

/** Handler for Who-Is requests, with broadcast I-Am or Who-Am-I response.
 * @ingroup DMDDB
 * @param service_request [in] The received message to be handled.
 * @param service_len [in] Length of the service_request message.
 * @param src [in] The BACNET_ADDRESS of the message's source (ignored).
 */
void handler_who_is_who_am_i_unicast(
    uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src)
{
    int len = 0;
    int32_t low_limit = 0;
    int32_t high_limit = 0;
    BACNET_CHARACTER_STRING model_name = { 0 }, serial_number = { 0 };

    len = whois_decode_service_request(
        service_request, service_len, &low_limit, &high_limit);
    if (len == 0) {
        if (handler_device_object_instance_number() == BACNET_MAX_INSTANCE) {
            /* The Who-Am-I service is also used to respond to a Who-Is
               service request that uses the Device Object_Identifier
               instance number of 4194303. */
            handler_device_character_string_get(PROP_MODEL_NAME, &model_name);
            handler_device_character_string_get(
                PROP_SERIAL_NUMBER, &serial_number);
            Send_Who_Am_I_To_Network(
                src, handler_device_vendor_identifier(), &model_name,
                &serial_number);
        } else {
            /* If no limits, then always respond */
            Send_I_Am_Unicast(&Handler_Transmit_Buffer[0], src);
        }
    } else if (len != BACNET_STATUS_ERROR) {
        /* is my device id within the limits? */
        if ((handler_device_object_instance_number() >= (uint32_t)low_limit) &&
            (handler_device_object_instance_number() <= (uint32_t)high_limit)) {
            if (handler_device_object_instance_number() ==
                BACNET_MAX_INSTANCE) {
                /* The Who-Am-I service is also used to respond to a Who-Is
                service request that uses the Device Object_Identifier
                instance number of 4194303. */
                handler_device_character_string_get(
                    PROP_MODEL_NAME, &model_name);
                handler_device_character_string_get(
                    PROP_SERIAL_NUMBER, &serial_number);
                Send_Who_Am_I_To_Network(
                    src, handler_device_vendor_identifier(), &model_name,
                    &serial_number);
            } else {
                Send_I_Am_Unicast(&Handler_Transmit_Buffer[0], src);
            }
        }
    }
}

#ifdef BAC_ROUTING /* was for BAC_ROUTING - delete in 2/2012 if still unused \
                    */
/* EKH: I restored this to BAC_ROUTING (from DEPRECATED) because I found that
   the server demo with the built-in
   virtual Router did not insert the SADRs of the virtual devices on the virtual
   network without it */

/** Local function to check Who-Is requests against our Device IDs.
 * Will check the gateway (root Device) and all virtual routed
 * Devices against the range and respond for each that matches.
 *
 * @param service_request [in] The received message to be handled.
 * @param service_len [in] Length of the service_request message.
 * @param src [in] The BACNET_ADDRESS of the message's source.
 * @param is_unicast [in] True if should send unicast response(s)
 *          back to the src, else False if should broadcast
 * response(s).
 */
static void check_who_is_for_routing(
    const uint8_t *service_request,
    uint16_t service_len,
    const BACNET_ADDRESS *src,
    bool is_unicast)
{
    int len = 0;
    int32_t low_limit = 0;
    int32_t high_limit = 0;
    int32_t dev_instance;
    int cursor = 0; /* Starting hint */
    int my_list[2] = { 0, -1 }; /* Not really used, so dummy values */
    BACNET_ADDRESS bcast_net;

    len = whois_decode_service_request(
        service_request, service_len, &low_limit, &high_limit);
    if (len == BACNET_STATUS_ERROR) {
        /* Invalid; just leave */
        return;
    }
    /* Go through all devices, starting with the root gateway Device */
    memset(&bcast_net, 0, sizeof(BACNET_ADDRESS));
    bcast_net.net = BACNET_BROADCAST_NETWORK; /* That's all we have to set */

    while (Routed_Device_GetNext(&bcast_net, my_list, &cursor)) {
        dev_instance = handler_device_object_instance_number();
        /* If len == 0, no limits and always respond */
        if ((len == 0) ||
            ((dev_instance >= low_limit) && (dev_instance <= high_limit))) {
            if (is_unicast) {
                Send_I_Am_Unicast(&Handler_Transmit_Buffer[0], src);
            } else {
                Send_I_Am_Broadcast(&Handler_Transmit_Buffer[0]);
            }
        }
    }
}

/** Handler for Who-Is requests in the virtual routing setup,
 * with broadcast I-Am response(s).
 * @ingroup DMDDB
 * Will check the gateway (root Device) and all virtual routed
 * Devices against the range and respond for each that matches.
 *
 * @ingroup DMDDB
 * @param service_request [in] The received message to be handled.
 * @param service_len [in] Length of the service_request message.
 * @param src [in] The BACNET_ADDRESS of the message's source (ignored).
 */
void handler_who_is_bcast_for_routing(
    uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src)
{
    check_who_is_for_routing(service_request, service_len, src, false);
}

/** Handler for Who-Is requests in the virtual routing setup,
 * with unicast I-Am response(s) returned to the src.
 * Will check the gateway (root Device) and all virtual routed
 * Devices against the range and respond for each that matches.
 *
 * @ingroup DMDDB
 * @param service_request [in] The received message to be handled.
 * @param service_len [in] Length of the service_request message.
 * @param src [in] The BACNET_ADDRESS of the message's source that the
 *                 response will be sent back to.
 */
void handler_who_is_unicast_for_routing(
    uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src)
{
    check_who_is_for_routing(service_request, service_len, src, true);
}
#endif /* BAC_ROUTING */
