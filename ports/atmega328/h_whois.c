/**
 * @brief This module manages the BACnet Who-Is handler
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2005
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "bacnet/config.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/whois.h"
#include "bacnet/iam.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"

bool Send_I_Am_Flag = true;

void handler_who_is(
    uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src)
{
    int len = 0;
    int32_t low_limit = 0;
    int32_t high_limit = 0;
    int32_t target_device;

    (void)src;
    len = whois_decode_service_request(
        service_request, service_len, &low_limit, &high_limit);
    if (len == 0) {
        Send_I_Am_Flag = true;
    } else if (len != BACNET_STATUS_ERROR) {
        /* is my device id within the limits? */
        target_device = Device_Object_Instance_Number();
        if ((target_device >= low_limit) && (target_device <= high_limit)) {
            Send_I_Am_Flag = true;
        }
    }

    return;
}
