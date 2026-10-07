/**
 * @file
 * @brief Stub functions for unit test of a BACnet object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date December 2022
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include "bacnet/datetime.h"
#include "bacnet/bacdef.h"
#include "bacnet/npdu.h"
#include "bacnet/basic/npdu/h_npdu.h"
#include "bacnet/basic/object/nc.h"
#include "bacnet/basic/object/device.h"

uint8_t Send_CEvent_Notify_Address(
    uint8_t *pdu,
    uint16_t pdu_size,
    const BACNET_EVENT_NOTIFICATION_DATA *data,
    BACNET_ADDRESS *dest)
{
    (void)pdu;
    (void)pdu_size;
    (void)data;
    (void)dest;
    return 0;
}

int Send_UEvent_Notify(
    uint8_t *buffer,
    const BACNET_EVENT_NOTIFICATION_DATA *data,
    BACNET_ADDRESS *dest)
{
    (void)buffer;
    (void)data;
    (void)dest;
    return 0;
}

uint8_t
Send_CEvent_Notify(uint32_t device_id, BACNET_EVENT_NOTIFICATION_DATA *data)
{
    (void)device_id;
    (void)data;
    return 0;
}

void Send_WhoIs(int32_t low_limit, int32_t high_limit)
{
    (void)low_limit;
    (void)high_limit;
}

void Send_Who_Is_Router_To_Network(BACNET_ADDRESS *dst, int dnet)
{
    (void)dst;
    (void)dnet;
}

void npdu_set_i_am_router_to_network_handler(
    i_am_router_to_network_function pFunction)
{
    (void)pFunction;
}

uint32_t Device_Object_Instance_Number(void)
{
    return 0;
}

#ifdef BAC_ROUTING
static uint16_t Current_Device_Index;

uint16_t Routed_Device_Object_Index(void)
{
    return Current_Device_Index;
}

bool Set_Routed_Device_Object_Index(uint16_t index)
{
    if (index >= MAX_NUM_DEVICES) {
        return false;
    }
    Current_Device_Index = index;
    return true;
}
#endif
