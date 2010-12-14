/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2010 Julien Bennet

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to:
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330
 Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/
#ifndef BACOTHERTYPES_H
#define BACOTHERTYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "bacdef.h"
#include "datetime.h"
#include "bacdevobjpropref.h"
#include "timestamp.h"

/* Forward */
struct BACnet_Access_Error;
struct BACnet_Destination;
struct BACnet_Recipient;

/*BACnetScale */
/*BACnetPrescale */
/*BACnetAccumulatorRecord */
/*BACnetAccumulatorRecord */

/*/ One small value for the BACnetTimeValue */
typedef struct BACnet_Short_Application_Data_Value {
    uint8_t tag;        /* application tag data type */
    union {
#if defined (BACAPP_BOOLEAN)
        bool Boolean;
#endif
#if defined (BACAPP_UNSIGNED)
        uint32_t Unsigned_Int;
#endif
#if defined (BACAPP_SIGNED)
        int32_t Signed_Int;
#endif
#if defined (BACAPP_REAL)
        float Real;
#endif
#if defined (BACAPP_ENUMERATED)
        uint32_t Enumerated;
#endif
    } type;
} BACNET_SHORT_APPLICATION_DATA_VALUE;

/*/ BACnetTimeValue */
/* time  Time,  */
/* value  ABSTRACT-SYNTAX.&Type  -- any primitive datatype; complex types cannot be decoded  */
typedef struct BACnet_Time_Value {
    BACNET_TIME Time;
    BACNET_SHORT_APPLICATION_DATA_VALUE Value;
} BACNET_TIME_VALUE;

/* arbitrary value, shall be unlimited for B-OWS but we don't care, 640k shall be enough */
/* however we try not to boost the bacnet application value structure size,  */
/* so 7 x (this value) x sizeof(BACNET_TIME_VALUE) fits. */
#define MAX_DAY_SCHEDULE_VALUES 40
#define MAX_SPECIAL_EVENT_VALUES 255

/*/BACnetDailySchedule ::= SEQUENCE {  */
/* day-schedule [0] SEQUENCE OF BACnetTimeValue  */
typedef struct BACnet_Daily_Schedule {
    BACNET_TIME_VALUE daySchedule[MAX_DAY_SCHEDULE_VALUES];     /* null value means no timevalue */
} BACNET_DAILY_SCHEDULE;

/*/ weekly-schedule [123] SEQUENCE SIZE(7) OF BACnetDailySchedule OPTIONAL */
typedef struct BACnet_Weekly_Schedule {
    BACNET_DAILY_SCHEDULE weeklySchedule[7];
} BACNET_WEEKLY_SCHEDULE;

/*/ BACnetCalendarEntry */
typedef struct BACnet_Calendar_Entry {
    /*/ Flag : choice between [0] Date [1] DateRange [2] Weeknday */
    uint8_t TagEntryType;
    union {
        /*date  [0] Date,  */
        BACNET_DATE date;
        /*dateRange [1] BACnetDateRange,  */
        BACNET_DATE_RANGE dateRange;
        /*weekNDay [2] BACnetWeekNDay */
        BACNET_WEEKNDAY weekNDay;
    } Entry;
} BACNET_CALENDAR_ENTRY;

/*/BACnetSpecialEvent */
typedef struct BACnet_Special_Event {
    /*/ Flag : choice between [0] calendarEntry [1] ObjectIdentifier */
    uint8_t TagSpecialEventType;
    union {
        /*   calendarEntry  [0] BACnetCalendarEntry,  */
        BACNET_CALENDAR_ENTRY calendarEntry;
        /*   calendarReference [1] BACnetObjectIdentifier  */
        BACNET_OBJECT_ID calendarReference;
    } period;
    /* listOfTimeValues [2] SEQUENCE OF BACnetTimeValue,  */
    BACNET_TIME_VALUE listOfTimeValues[MAX_SPECIAL_EVENT_VALUES];
    /* eventPriority [3] Unsigned (1..16) */
    uint8_t eventPriority;
} BACNET_SPECIAL_EVENT;

/*/ BACnetRecipient */
typedef struct BACnet_Recipient {
    /*/ Flag : choice between [0] BACnetObjectIdentifier, [1] BACnetAddress */
    uint8_t TagRecipientType;
    union {
        /*/ ObjectIdentifier : the recipient is an object to be discovered */
        BACNET_OBJECT_ID Device;
        /*/ Address : the complete recipient address */
        BACNET_ADDRESS Address;
    } RecipientType;
} BACNET_RECIPIENT;

/*/ BACnetRecipientProcess */
typedef struct BACnet_Recipient_Process {
    BACNET_RECIPIENT recipient;
    uint32_t processIdentifier;
} BACNET_RECIPIENT_PROCESS;

/*/ BACnetCOVSubscription */
typedef struct BACnet_COV_Subscription {
    BACNET_RECIPIENT_PROCESS recipient;
    BACNET_OBJECT_PROPERTY_REFERENCE monitoredPropertyReference;
    bool issueConfirmedNotifications;
    uint32_t timeRemaining;
    float covIncrement;
} BACNET_COV_SUBSCRIPTION;

/*/ BACnetAddressBinding */
typedef struct BACnet_Address_Binding {
    BACNET_OBJECT_ID deviceObjectIdentifier;
    BACNET_ADDRESS deviceAddress;
} BACNET_ADDRESS_BINDING;

typedef struct BACnet_Destination {
    /*/ BACnetDaysOfWeek : the set of dats of the week on which  */
    /*/ this destination may be used between from and to time */
    BACNET_BIT_STRING ValidDays;
    /*/ Begin of the time window (inclusive) during which the destination is viable */
    BACNET_TIME FromTime;
    /*/ End of the time window (inclusive) during which the destination is viable */
    BACNET_TIME ToTime;
    /*/ The destination device(s) to receive notifications */
    BACNET_RECIPIENT Recipient;
    /*/ Handle of a process within the recipient device */
    uint32_t ProcessIdentifier;
    /*/ True if confirmed notifications are to be sent, false if unconfirmed notifications */
    bool IssueConfirmedNotifications;
    /*/ BACnetEventTransitionBits : 3 flags that indicate the transitions  */
    /*/ {to-offnormal, to-fault, to-normal } for which this recipient is suitable */
    BACNET_BIT_STRING Transitions;
} BACNET_DESTINATION;

typedef struct BACnet_Access_Error {
    BACNET_ERROR_CLASS error_class;
    BACNET_ERROR_CODE error_code;
} BACNET_ACCESS_ERROR;

/* arbitrary value : maximum list of property references to be read on a single object */
#define MAX_LIST_OF_PROPERTY_REFERENCES 100
/* empty value marker */
#define EMPTY_PROPERTY_REFERENCE_ID ((BACNET_PROPERTY_ID)(~0))

/* ReadAccessSpecification */
typedef struct BACnet_Read_Access_Specification {
    BACNET_OBJECT_ID objectIdentifier;
                     BACNET_PROPERTY_REF
        listOfPropertyReferences[MAX_LIST_OF_PROPERTY_REFERENCES];
} BACNET_READ_ACCESS_SPECIFICATION;

#endif /*BACOTHERTYPES_H */
