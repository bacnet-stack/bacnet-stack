/**
 * @file
 * @brief A basic I-Have service handler
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2005
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bactext.h"
#include "bacnet/ihave.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"

/** Simple Handler for I-Have responses (just validates response).
 * @ingroup DMDOB
 * @param service_request [in] The received message to be handled.
 * @param service_len [in] Length of the service_request message.
 * @param src [in] The BACNET_ADDRESS of the message's source.
 */
void handler_i_have(
    uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src)
{
    int len = 0;
    BACNET_I_HAVE_DATA data;

    (void)service_len;
    (void)src;
    len = ihave_decode_service_request(service_request, service_len, &data);
    if (len != -1) {
#if PRINT_ENABLED
        fprintf(
            stderr, "I-Have: %s %lu from %s %lu!\r\n",
            bactext_object_type_name(data.object_id.type),
            (unsigned long)data.object_id.instance,
            bactext_object_type_name(data.device_id.type),
            (unsigned long)data.device_id.instance);
#endif
    } else {
#if PRINT_ENABLED
        fprintf(stderr, "I-Have: received, but unable to decode!\n");
#endif
    }

    return;
}
