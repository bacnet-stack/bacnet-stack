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
#include <stdio.h>
#include <stdlib.h>
#include "bacnet/bacdef.h"
#include "bacnet/alarm_ack.h"
#include "bacnet/datetime.h"
#include "bacnet/event.h"
#include "bacnet/getevent.h"
#include "bacnet/get_alarm_sum.h"
#include "bacnet/npdu.h"

bool datetime_local(
    BACNET_DATE *bdate,
    BACNET_TIME *btime,
    int16_t *utc_offset_minutes,
    bool *dst_active)
{
    (void)bdate;
    (void)btime;
    (void)utc_offset_minutes;
    (void)dst_active;

    return false;
}

void Notification_Class_common_reporting_function(
    BACNET_EVENT_NOTIFICATION_DATA *event_data)
{
    (void)event_data;
}

void Notification_Class_Get_Priorities(
    uint32_t Object_Instance, uint32_t *pPriorityArray)
{
    (void)Object_Instance;
    (void)pPriorityArray;
}

void handler_get_event_information_set(
    BACNET_OBJECT_TYPE object_type, get_event_info_function pFunction)
{
    (void)object_type;
    (void)pFunction;
}

void handler_alarm_ack_set(
    BACNET_OBJECT_TYPE object_type, alarm_ack_function pFunction)
{
    (void)object_type;
    (void)pFunction;
}

void handler_get_alarm_summary_set(
    BACNET_OBJECT_TYPE object_type, get_alarm_summary_function pFunction)
{
    (void)object_type;
    (void)pFunction;
}

void cov_change_detected_notify(void)
{
}
