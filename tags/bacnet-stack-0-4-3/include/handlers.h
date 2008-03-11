/**************************************************************************
*
* Copyright (C) 2005-2006 Steve Karg <skarg@users.sourceforge.net>
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
#ifndef HANDLERS_H
#define HANDLERS_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "bacdef.h"
#include "apdu.h"
#include "bacapp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    void handler_unrecognized_service(
        uint8_t * service_request,
        uint16_t service_len,
        BACNET_ADDRESS * dest,
        BACNET_CONFIRMED_SERVICE_DATA * service_data);

    void handler_who_is(
        uint8_t * service_request,
        uint16_t service_len,
        BACNET_ADDRESS * src);

    void handler_who_has(
        uint8_t * service_request,
        uint16_t service_len,
        BACNET_ADDRESS * src);

    void handler_i_am_add(
        uint8_t * service_request,
        uint16_t service_len,
        BACNET_ADDRESS * src);

    void handler_i_am_bind(
        uint8_t * service_request,
        uint16_t service_len,
        BACNET_ADDRESS * src);

    void handler_read_property(
        uint8_t * service_request,
        uint16_t service_len,
        BACNET_ADDRESS * src,
        BACNET_CONFIRMED_SERVICE_DATA * service_data);

    void handler_read_property_ack(
        uint8_t * service_request,
        uint16_t service_len,
        BACNET_ADDRESS * src,
        BACNET_CONFIRMED_SERVICE_ACK_DATA * service_data);

    void handler_write_property(
        uint8_t * service_request,
        uint16_t service_len,
        BACNET_ADDRESS * src,
        BACNET_CONFIRMED_SERVICE_DATA * service_data);

    void handler_atomic_read_file(
        uint8_t * service_request,
        uint16_t service_len,
        BACNET_ADDRESS * src,
        BACNET_CONFIRMED_SERVICE_DATA * service_data);

    void handler_atomic_read_file_ack(
        uint8_t * service_request,
        uint16_t service_len,
        BACNET_ADDRESS * src,
        BACNET_CONFIRMED_SERVICE_ACK_DATA * service_data);

    void handler_atomic_write_file(
        uint8_t * service_request,
        uint16_t service_len,
        BACNET_ADDRESS * src,
        BACNET_CONFIRMED_SERVICE_DATA * service_data);

    void handler_reinitialize_device(
        uint8_t * service_request,
        uint16_t service_len,
        BACNET_ADDRESS * src,
        BACNET_CONFIRMED_SERVICE_DATA * service_data);

    void handler_device_communication_control(
        uint8_t * service_request,
        uint16_t service_len,
        BACNET_ADDRESS * src,
        BACNET_CONFIRMED_SERVICE_DATA * service_data);

    void handler_i_have(
        uint8_t * service_request,
        uint16_t service_len,
        BACNET_ADDRESS * src);

    void handler_timesync(
        uint8_t * service_request,
        uint16_t service_len,
        BACNET_ADDRESS * src);

    void handler_timesync_utc(
        uint8_t * service_request,
        uint16_t service_len,
        BACNET_ADDRESS * src);

    void handler_read_property_multiple(
        uint8_t * service_request,
        uint16_t service_len,
        BACNET_ADDRESS * src,
        BACNET_CONFIRMED_SERVICE_DATA * service_data);

    /* Encodes the property APDU and returns the length,
       or sets the error, and returns -1 */
    /* resides in h_rp.c */
    int Encode_Property_APDU(
        uint8_t * apdu,
        BACNET_OBJECT_TYPE object_type,
        uint32_t object_instance,
        BACNET_PROPERTY_ID property,
        int32_t array_index,
        BACNET_ERROR_CLASS * error_class,
        BACNET_ERROR_CODE * error_code);

    void handler_cov_subscribe(
        uint8_t * service_request,
        uint16_t service_len,
        BACNET_ADDRESS * src,
        BACNET_CONFIRMED_SERVICE_DATA * service_data);
    void handler_cov_task(
        uint32_t elapsed_seconds);
    int handler_cov_encode_subscriptions(
        uint8_t * apdu,
        int max_apdu);


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
