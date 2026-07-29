/**
 * @file
 * @brief Test-support API for the Notification_Class_common_reporting_function
 *  stub, allowing tests to inspect the event data that was reported.
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef ALERT_ENROLLMENT_TEST_STUBS_H
#define ALERT_ENROLLMENT_TEST_STUBS_H

#include "bacnet/bacstr.h"
#include "bacnet/event.h"

/* number of calls the stub can remember before it stops recording */
#define REPORTING_STUB_MAX_CALLS 4

void Reporting_Stub_Reset(void);
unsigned Reporting_Stub_Call_Count(void);
const BACNET_EVENT_NOTIFICATION_DATA *Reporting_Stub_Event_Data(unsigned index);
const BACNET_CHARACTER_STRING *Reporting_Stub_Message_Text(unsigned index);

#endif
