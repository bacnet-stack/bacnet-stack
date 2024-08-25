/*
 * Copyright (c) 2022 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* Analog Value Objects - customize for your use */

#include "object.h"
#include "bacnet/basic/object/av.h"

OBJECT_FUNCTIONS_WITHOUT_INIT(Analog_Value, ANALOG_VALUE_DESCR);

void Analog_Value_Init(void)
{
    if (!Object_List) {
        Object_List = Keylist_Create();

#if defined(INTRINSIC_REPORTING)
        /* Set handler for GetEventInformation function */
        handler_get_event_information_set(
            OBJECT_ANALOG_VALUE, Analog_Value_Event_Information);
        /* Set handler for AcknowledgeAlarm function */
        handler_alarm_ack_set(OBJECT_ANALOG_VALUE, Analog_Value_Alarm_Ack);
        /* Set handler for GetAlarmSummary Service */
        handler_get_alarm_summary_set(
            OBJECT_ANALOG_VALUE, Analog_Value_Alarm_Summary);
#endif
    }
}
