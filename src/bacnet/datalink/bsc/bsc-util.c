/**
 * @file
 * @brief module for common function for BACnet/SC implementation
 * @author Kirill Neznamov
 * @date Jule 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC 
 * as an unpublished work.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */

#include "bacnet/datalink/bsc/bsc-util.h"
#include <stdlib.h>

unsigned long bsc_seconds_left(
    unsigned long timestamp_ms, unsigned long timeout_s)
{
    unsigned long cur = mstimer_now();
    unsigned long delta;

    if (cur > timestamp_ms) {
        delta = cur - timestamp_ms;
    } else {
        delta = timestamp_ms - cur;
    }

    if (delta > timeout_s * 1000) {
        return 0;
    }

    return timeout_s * 1000 - delta;
}

BACNET_SC_RET bsc_map_websocket_retcode(BACNET_WEBSOCKET_RET ret)
{
    switch(ret) {
        case BACNET_WEBSOCKET_SUCCESS:
             return BACNET_SC_SUCCESS;
        case BACNET_WEBSOCKET_CLOSED:
             return BACNET_SC_CLOSED;
        case BACNET_WEBSOCKET_NO_RESOURCES:
             return BACNET_SC_NO_RESOURCES;
        case BACNET_WEBSOCKET_OPERATION_IN_PROGRESS:
             return BACNET_SC_OPERATION_IN_PROGRESS;
        case BACNET_WEBSOCKET_BAD_PARAM:
             return BACNET_SC_BAD_PARAM;
        case BACNET_WEBSOCKET_TIMEDOUT:
             return BACNET_SC_TIMEDOUT;
        case BACNET_WEBSOCKET_INVALID_OPERATION:
             return BACNET_SC_INVALID_OPERATION;
        case BACNET_WEBSOCKET_BUFFER_TOO_SMALL:
             return BACNET_SC_BUFFER_TOO_SMALL;
        case BACNET_WEBSOCKET_OPERATION_CANCELED:
             return BACNET_SC_OPERATION_CANCELED;
        default:
            return BACNET_SC_CLOSED;
    }
}

char *bsc_vmac_to_string(BACNET_SC_VMAC_ADDRESS *vmac)
{
    static char buf[128];
    sprintf(buf, "%02x%02x%02x%02x%02x%02x", vmac->address[0], vmac->address[1],
        vmac->address[2], vmac->address[3], vmac->address[4], vmac->address[5]);
    return buf;
}

char *bsc_uuid_to_string(BACNET_SC_UUID *uuid)
{
    static char buf[128];
    sprintf(buf,
        "%02x%02x%02x%02x-%02x%02x%02x%02x-%02x%02x%02x%02x-%02x%02x%02x%02x",
        uuid->uuid[0], uuid->uuid[1], uuid->uuid[2], uuid->uuid[3],
        uuid->uuid[4], uuid->uuid[5], uuid->uuid[6], uuid->uuid[7],
        uuid->uuid[8], uuid->uuid[9], uuid->uuid[10], uuid->uuid[11],
        uuid->uuid[12], uuid->uuid[13], uuid->uuid[14], uuid->uuid[15]);
    return buf;
}

void bsc_generate_random_vmac(BACNET_SC_VMAC_ADDRESS *p)
{
    int i;

    for (i = 0; i < BVLC_SC_VMAC_SIZE; i++) {
        p->address[i] = rand() % 255;
        if (i == 0) {
            // According H.7.3 EUI-48 and Random-48 VMAC Address:
            // The Random-48 VMAC is a 6-octet VMAC address in which the least
            // significant 4 bits (Bit 3 to Bit 0) in the first octet shall be
            // B'0010' (X'2'), and all other 44 bits are randomly selected to be
            // 0 or 1.
            p->address[i] = p->address[i] & 0xF0 | 0x02;
        }
    }
}

#if 0
static BSC_CONNECTION *bsc_find_connection_for_vmac(
    BACNET_SC_VMAC_ADDRESS *vmac)
{
    BSC_CONNECTION *ret = NULL;
    BSC_CONNECTION *e = bsc_conn_list_head;

    debug_printf("bsc_find_connection_for_vmac() >>> vmac = %s\n",
        bsc_vmac_to_string(vmac));

    while (e) {
        if (!memcmp(&e->vmac, vmac, sizeof(*vmac))) {
            ret = e;
            break;
        }
        e = e->next;
    }

    debug_printf("bsc_find_connection_for_vmac() <<< ret = %p\n", ret);
    return ret;
}

static BSC_CONNECTION *bsc_find_connection_for_uuid(BACNET_SC_UUID *uuid)
{
    BSC_CONNECTION *ret = NULL;
    BSC_CONNECTION *e = bsc_conn_list_head;

    debug_printf("bsc_find_connection_for_uuid() >>> uuid = %s\n",
        bsc_uuid_to_string(uuid));

    while (e) {
        if (!memcmp(&e->uuid, uuid, sizeof(*uuid))) {
            ret = e;
            break;
        }
        e = e->next;
    }

    debug_printf("bsc_find_connection_for_uuid() <<< ret = %p\n", ret);
    return ret;
}
#endif