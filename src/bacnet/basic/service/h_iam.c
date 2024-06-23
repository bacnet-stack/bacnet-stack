/**
 * @file
 * @brief A basic I-Am service handler
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
#include "bacnet/iam.h"
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"

/** Handler for I-Am responses.
 * Will add the responder to our cache, or update its binding.
 * @ingroup DMDDB
 * @param service_request [in] The received message to be handled.
 * @param service_len [in] Length of the service_request message.
 * @param src [in] The BACNET_ADDRESS of the message's source.
 */
void handler_i_am_add(
    uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src)
{
    int len = 0;
    uint32_t device_id = 0;
    unsigned max_apdu = 0;
    int segmentation = 0;
    uint16_t vendor_id = 0;

    (void)service_len;
    len = iam_decode_service_request(
        service_request, &device_id, &max_apdu, &segmentation, &vendor_id);
#if PRINT_ENABLED
    fprintf(stderr, "Received I-Am Request");
#endif
    if (len != -1) {
#if PRINT_ENABLED
        fprintf(stderr, " from %lu, MAC = %d.%d.%d.%d.%d.%d\n",
            (unsigned long)device_id, src->mac[0], src->mac[1], src->mac[2],
            src->mac[3], src->mac[4], src->mac[5]);
#endif
        address_add(device_id, max_apdu, src);
    } else {
#if PRINT_ENABLED
        fprintf(stderr, ", but unable to decode it.\n");
#endif
    }

    return;
}

/** Handler for I-Am responses (older binding-update-only version).
 * Will update the responder's binding, but if already in our cache.
 * @note This handler is deprecated, in favor of handler_i_am_add().
 *
 * @param service_request [in] The received message to be handled.
 * @param service_len [in] Length of the service_request message.
 * @param src [in] The BACNET_ADDRESS of the message's source.
 */
void handler_i_am_bind(
    uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src)
{
    int len = 0;
    uint32_t device_id = 0;
    unsigned max_apdu = 0;
    int segmentation = 0;
    uint16_t vendor_id = 0;

    (void)service_len;
    len = iam_decode_service_request(
        service_request, &device_id, &max_apdu, &segmentation, &vendor_id);
    if (len > 0) {
        /* only add address if requested to bind */
        address_add_binding(device_id, max_apdu, src);
    }

    return;
}
