/**
 * @file
 * @brief Stub functions for unit test of a BACnet object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date December 2022
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include "bacnet/datetime.h"
#include "bacnet/bacdef.h"
#include "bacnet/npdu.h"
#include "bacnet/basic/object/nc.h"

uint32_t Device_Object_Instance_Number(void)
{
    return 0;
}

int Send_UEvent_Notify(
    uint8_t *buffer, BACNET_EVENT_NOTIFICATION_DATA *data, BACNET_ADDRESS *dest)
{
    (void)buffer;
    (void)data;
    (void)dest;
    return 0;
}

uint8_t Send_CEvent_Notify(
    uint32_t device_id, BACNET_EVENT_NOTIFICATION_DATA *data)
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

bool datetime_local(
    BACNET_DATE * bdate,
    BACNET_TIME * btime,
    int16_t * utc_offset_minutes,
    bool * dst_active)
{
    (void)bdate;
    (void)btime;
    (void)utc_offset_minutes;
    (void)dst_active;
    return true;
}
