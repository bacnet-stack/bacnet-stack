/**
 * @file
 * @brief Stub functions for unit test of a BACnet object
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "bacnet/bacdef.h"
#include "bacnet/event.h"
#include "stubs.h"

/* records the event data passed to each call, so tests can verify that
   Alert_Enrollment_Intrinsic_Reporting reported the alert it queued */
static BACNET_EVENT_NOTIFICATION_DATA Reporting_Stub_Data[REPORTING_STUB_MAX_CALLS];
static BACNET_CHARACTER_STRING Reporting_Stub_Text[REPORTING_STUB_MAX_CALLS];
static bool Reporting_Stub_Has_Text[REPORTING_STUB_MAX_CALLS];
static unsigned Reporting_Stub_Count;

void Notification_Class_common_reporting_function(
    BACNET_EVENT_NOTIFICATION_DATA *event_data)
{
    if (event_data && (Reporting_Stub_Count < REPORTING_STUB_MAX_CALLS)) {
        /* messageText points at a caller-local BACNET_CHARACTER_STRING that
           is only valid for the duration of this call, so snapshot its
           contents rather than keeping the pointer */
        Reporting_Stub_Data[Reporting_Stub_Count] = *event_data;
        if (event_data->messageText) {
            Reporting_Stub_Text[Reporting_Stub_Count] = *event_data->messageText;
            Reporting_Stub_Has_Text[Reporting_Stub_Count] = true;
        } else {
            Reporting_Stub_Has_Text[Reporting_Stub_Count] = false;
        }
        Reporting_Stub_Count++;
    }
}

void Reporting_Stub_Reset(void)
{
    Reporting_Stub_Count = 0;
    memset(Reporting_Stub_Data, 0, sizeof(Reporting_Stub_Data));
    memset(Reporting_Stub_Text, 0, sizeof(Reporting_Stub_Text));
    memset(Reporting_Stub_Has_Text, 0, sizeof(Reporting_Stub_Has_Text));
}

unsigned Reporting_Stub_Call_Count(void)
{
    return Reporting_Stub_Count;
}

const BACNET_EVENT_NOTIFICATION_DATA *Reporting_Stub_Event_Data(unsigned index)
{
    if (index < Reporting_Stub_Count) {
        return &Reporting_Stub_Data[index];
    }
    return NULL;
}

const BACNET_CHARACTER_STRING *Reporting_Stub_Message_Text(unsigned index)
{
    if ((index < Reporting_Stub_Count) && Reporting_Stub_Has_Text[index]) {
        return &Reporting_Stub_Text[index];
    }
    return NULL;
}
