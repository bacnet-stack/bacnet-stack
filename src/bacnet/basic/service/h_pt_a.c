/**
 * @file
 * @brief Handles Confirmed Private Transfer ACKs.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2026
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"
#include "bacnet/ptransfer.h"
/* basic services */
#include "bacnet/basic/services.h"
#include "bacnet/basic/service/h_upt.h"

void handler_confirmed_private_transfer_ack(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_ACK_DATA *service_data)
{
    BACNET_PRIVATE_TRANSFER_DATA data = { 0 };
    int len = 0;

    (void)src;
    (void)service_data;

    len = ptransfer_decode_service_request(service_request, service_len, &data);
    if (len > 0) {
        private_transfer_print_data(&data);
    }
}
