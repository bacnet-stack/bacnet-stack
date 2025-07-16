/**
 * @file
 * @brief BACnet EventNotification encode and decode functions
 * @author John Minack <minack@users.sourceforge.net>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2008
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_EVENT_H_
#define BACNET_EVENT_H_

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/timestamp.h"
#include "bacnet/bacpropstates.h"
#include "bacnet/bacdevobjpropref.h"
#include "bacnet/authentication_factor.h"

typedef enum {
    CHANGE_OF_VALUE_BITS,
    CHANGE_OF_VALUE_REAL
} CHANGE_OF_VALUE_TYPE;

typedef enum {
    COMMAND_FAILURE_BINARY_PV,
    COMMAND_FAILURE_UNSIGNED
} COMMAND_FAILURE_TYPE;

/*
** Based on UnconfirmedEventNotification-Request
*/

/** Enable decoding of complex-event-type property-values. If set to 0, the
 * values are decoded and discarded. */
#ifndef BACNET_DECODE_COMPLEX_EVENT_TYPE_PARAMETERS
#define BACNET_DECODE_COMPLEX_EVENT_TYPE_PARAMETERS 1
#endif

/** Max complex-event-type property-values to decode. Events with more values
 * fail to decode. */
#ifndef BACNET_COMPLEX_EVENT_TYPE_MAX_PARAMETERS
#define BACNET_COMPLEX_EVENT_TYPE_MAX_PARAMETERS 5
#endif

typedef struct BACnet_Event_Notification_Data {
    uint32_t processIdentifier;
    BACNET_OBJECT_ID initiatingObjectIdentifier;
    BACNET_OBJECT_ID eventObjectIdentifier;
    BACNET_TIMESTAMP timeStamp;
    uint32_t notificationClass;
    uint8_t priority;
    BACNET_EVENT_TYPE eventType;
    /* OPTIONAL - Set to NULL if not being used */
    BACNET_CHARACTER_STRING *messageText;
    BACNET_NOTIFY_TYPE notifyType;
    bool ackRequired;
    BACNET_EVENT_STATE fromState;
    BACNET_EVENT_STATE toState;
    /*
     ** Each of these structures in the union maps to a particular eventtype
     ** Based on BACnetNotificationParameters
     */

    union {
        /*
         ** EVENT_CHANGE_OF_BITSTRING
         */
        struct {
            BACNET_BIT_STRING referencedBitString;
            BACNET_BIT_STRING statusFlags;
        } changeOfBitstring;
        /*
         ** EVENT_CHANGE_OF_STATE
         */
        struct {
            BACNET_PROPERTY_STATE newState;
            BACNET_BIT_STRING statusFlags;
        } changeOfState;
        /*
         ** EVENT_CHANGE_OF_VALUE
         */
        struct {
            union {
                BACNET_BIT_STRING changedBits;
                float changeValue;
            } newValue;
            CHANGE_OF_VALUE_TYPE tag;
            BACNET_BIT_STRING statusFlags;
        } changeOfValue;
        /*
         ** EVENT_COMMAND_FAILURE
         */
        struct {
            union {
                BACNET_BINARY_PV binaryValue;
                BACNET_UNSIGNED_INTEGER unsignedValue;
            } commandValue;
            COMMAND_FAILURE_TYPE tag;
            BACNET_BIT_STRING statusFlags;
            union {
                BACNET_BINARY_PV binaryValue;
                BACNET_UNSIGNED_INTEGER unsignedValue;
            } feedbackValue;
        } commandFailure;
        /*
         ** EVENT_FLOATING_LIMIT
         */
        struct {
            float referenceValue;
            BACNET_BIT_STRING statusFlags;
            float setPointValue;
            float errorLimit;
        } floatingLimit;
        /*
         ** EVENT_OUT_OF_RANGE
         */
        struct {
            float exceedingValue;
            BACNET_BIT_STRING statusFlags;
            float deadband;
            float exceededLimit;
        } outOfRange;
        /*
         ** EVENT_CHANGE_OF_LIFE_SAFETY
         */
        struct {
            BACNET_LIFE_SAFETY_STATE newState;
            BACNET_LIFE_SAFETY_MODE newMode;
            BACNET_BIT_STRING statusFlags;
            BACNET_LIFE_SAFETY_OPERATION operationExpected;
        } changeOfLifeSafety;
        /*
         ** EVENT_EXTENDED
         **
         ** Not Supported!
         */
        /*
         ** EVENT_BUFFER_READY
         */
        struct {
            BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE bufferProperty;
            uint32_t previousNotification;
            uint32_t currentNotification;
        } bufferReady;
        /*
         ** EVENT_UNSIGNED_RANGE
         */
        struct {
            uint32_t exceedingValue;
            BACNET_BIT_STRING statusFlags;
            uint32_t exceededLimit;
        } unsignedRange;
        /*
         ** EVENT_ACCESS_EVENT
         */
        struct {
            BACNET_ACCESS_EVENT accessEvent;
            BACNET_BIT_STRING statusFlags;
            BACNET_UNSIGNED_INTEGER accessEventTag;
            BACNET_TIMESTAMP accessEventTime;
            BACNET_DEVICE_OBJECT_REFERENCE accessCredential;
            BACNET_AUTHENTICATION_FACTOR authenticationFactor;
            /* OPTIONAL - Set authenticationFactor.format_type to
               AUTHENTICATION_FACTOR_MAX if not being used */
        } accessEvent;
#if (BACNET_DECODE_COMPLEX_EVENT_TYPE_PARAMETERS == 1)
        /*
         * complex-event-type - a sequence of values, used for proprietary event
         * types
         */
        struct {
            BACNET_PROPERTY_VALUE
            values[BACNET_COMPLEX_EVENT_TYPE_MAX_PARAMETERS];
        } complexEventType;
#endif
    } notificationParams;
} BACNET_EVENT_NOTIFICATION_DATA;

/*  BACnetEventParameter ::= CHOICE {
        -- These choices have a one-to-one correspondence with
        -- the Event_Type enumeration with the exception of the
        -- complex-event-type, which is used for proprietary event types.
        change-of-bitstring [0] SEQUENCE {
            time-delay [0] Unsigned,
            bitmask [1] BitString,
            list-of-bitstring-values [2] SEQUENCE OF BitString
        },
        change-of-state [1] SEQUENCE {
            time-delay [0] Unsigned,
            list-of-values [1] SEQUENCE OF BACnetPropertyStates
        },
        change-of-value [2] SEQUENCE {
            time-delay [0] Unsigned,
            cov-criteria [1] CHOICE {
                bitmask [0] BitString,
                referenced-property-increment [1] Real
            }
        },
        command-failure [3] SEQUENCE {
            time-delay [0] Unsigned,
            feedback-property-reference [1] BACnetDeviceObjectPropertyReference
        },
        floating-limit [4] SEQUENCE {
            time-delay [0] Unsigned,
            setpoint-reference [1] BACnetDeviceObjectPropertyReference,
            low-diff-limit [2] Real,
            high-diff-limit [3] Real,
            deadband [4] Real
        },
        out-of-range [5] SEQUENCE {
            time-delay [0] Unsigned,
            low-limit [1] Real,
            high-limit [2] Real,
            deadband [3] Real
        },
        -- CHOICE [6] has been intentionally omitted.
        -- It parallels the complex-event-type CHOICE [6] of the
        -- BACnetNotificationParameters production which was
        -- introduced to allow the addition of proprietary event
        -- algorithms whose event parameters are not necessarily
        -- network-visible.
        -- context tag 7 is deprecated
        change-of-life-safety [8] SEQUENCE {
            time-delay[0] Unsigned,
            list-of-life-safety-alarm-values[1] SEQUENCE OF
                BACnetLifeSafetyState,
            list-of-alarm-values[2] SEQUENCE OF BACnetLifeSafetyState,
            mode-property-reference[3] BACnetDeviceObjectPropertyReference
        },
        extended [9] SEQUENCE {
            vendor-id [0] Unsigned16,
            extended-event-type [1] Unsigned,
            parameters[2] SEQUENCE OF CHOICE {
                null Null,
                real Real,
                unsigned Unsigned,
                boolean Boolean,
                integer Integer,
                double Double,
                octetstring OctetString,
                characterstring CharacterString,
                bitstring BitString,
                enumerated ENUMERATED,
                date DatePattern,
                time TimePattern,
                objectidentifier BACnetObjectIdentifier,
                reference [0] BACnetDeviceObjectPropertyReference
        }
    },
    buffer-ready [10] SEQUENCE {
        notification-threshold [0] Unsigned,
        previous-notification-count[1] Unsigned32
    },
    unsigned-range [11] SEQUENCE {
        time-delay[0] Unsigned,
        low-limit[1] Unsigned,
        high-limit[2] Unsigned
    },
    -- context tag 12 is reserved for future addenda
    access-event [13] SEQUENCE {
        list-of-access-events[0] SEQUENCE OF BACnetAccessEvent,
        access-event-time-reference[1] BACnetDeviceObjectPropertyReference
    },
    double-out-of-range [14] SEQUENCE {
        time-delay[0] Unsigned,
        low-limit[1] Double,
        high-limit[2] Double,
        deadband[3] Double
    },
    signed-out-of-range [15] SEQUENCE {
        time-delay[0] Unsigned,
        low-limit[1] Integer,
        high-limit[2] Integer,
        deadband[3] Unsigned
    },
    unsigned-out-of-range [16] SEQUENCE {
        time-delay[0] Unsigned,
        low-limit[1] Unsigned,
        high-limit[2] Unsigned,
        deadband[3] Unsigned
    },
    change-of-characterstring [17] SEQUENCE {
        time-delay[0] Unsigned,
        list-of-alarm-values[1] SEQUENCE OF CharacterString
    },
    change-of-status-flags [18] SEQUENCE {
        time-delay[0] Unsigned,
        selected-flags[1] BACnetStatusFlags
    },
    -- CHOICE [19] has been intentionally omitted.
    -- It parallels the change-of-reliability event type CHOICE[19]
    -- of the BACnetNotificationParameters production
    -- which was introduced for the notification of event state
    -- changes to FAULT and from FAULT, which does not have
    -- event parameters.
    none [20] Null,
    change-of-discrete-value [21] SEQUENCE {
        new-value [0] CHOICE {
            boolean Boolean,
            unsigned Unsigned,
            integer Integer,
            enumerated ENUMERATED,
            characterstring CharacterString,
            octetstring OctetString,
            datePattern Date,
            timePattern Time,
            objectidentifier BACnetObjectIdentifier,
            datetime [0] BACnetDateTime
        },
        status-flags [1] BACnetStatusFlags
    },
    change-of-timer [22] SEQUENCE {
        time-delay [0] Unsigned,
        alarm-values[1] SEQUENCE OF BACnetTimerState,
        update-time-reference[2] BACnetDeviceObjectPropertyReference
    }
*/
typedef struct BACnetEventParameter {
    BACNET_EVENT_TYPE event_type;
    union {
        struct {
            uint32_t time_delay;
            BACNET_BIT_STRING bitmask;
            struct {
                BACNET_BIT_STRING value;
                void *next;
            } list_of_bitstring_values;
        } change_of_bitstring;
        struct {
            uint32_t time_delay;
            struct {
                BACNET_PROPERTY_STATE value;
                void *next;
            } list_of_values;
        } change_of_state;
        struct {
            uint32_t time_delay;
            union {
                BACNET_BIT_STRING bitmask;
                float referenced_property_increment;
            } cov_criteria;
        } change_of_value;
        struct {
            uint32_t time_delay;
            BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE feedback_property_reference;
        } command_failure;
        struct {
            uint32_t time_delay;
            BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE setpoint_reference;
            float low_diff_limit;
            float high_diff_limit;
            float deadband;
        } floating_limit;
        struct {
            uint32_t time_delay;
            float low_limit;
            float high_limit;
            float deadband;
        } out_of_range;
        struct {
            uint32_t time_delay;
            struct {
                BACNET_LIFE_SAFETY_STATE value;
                void *next;
            } list_of_life_safety_alarm_values;
            struct {
                BACNET_LIFE_SAFETY_STATE value;
                void *next;
            } list_of_alarm_values;
            BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE mode_property_reference;
        } change_of_life_safety;
        struct {
            uint16_t vendor_id;
            uint32_t extended_event_type;
            struct {
                uint8_t tag;
                union {
                    bool boolean_value;
                    float real_value;
                    BACNET_UNSIGNED_INTEGER unsigned_value;
                    int32_t integer_value;
                    double double_value;
                    BACNET_OCTET_STRING octet_string;
                    BACNET_CHARACTER_STRING character_string;
                    BACNET_BIT_STRING bit_string;
                    uint32_t enumerated;
                    BACNET_DATE date_pattern;
                    BACNET_TIME time_pattern;
                    BACNET_OBJECT_ID object_identifier;
                    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE reference;
                } parameters;
                void *next;
            } parameters;
        } extended;
        struct {
            uint32_t notification_threshold;
            uint32_t previous_notification_count;
        } buffer_ready;
        struct {
            uint32_t time_delay;
            uint32_t low_limit;
            uint32_t high_limit;
        } unsigned_range;
        struct {
            struct {
                BACNET_ACCESS_EVENT value;
                void *next;
            } list_of_access_events;
            BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE access_event_time_reference;
        } access_event;
        struct {
            uint32_t time_delay;
            double low_limit;
            double high_limit;
            double deadband;
        } double_out_of_range;
        struct {
            uint32_t time_delay;
            int32_t low_limit;
            int32_t high_limit;
            uint32_t deadband;
        } signed_out_of_range;
        struct {
            uint32_t time_delay;
            uint32_t low_limit;
            uint32_t high_limit;
            uint32_t deadband;
        } unsigned_out_of_range;
        struct {
            uint32_t time_delay;
            struct {
                BACNET_CHARACTER_STRING value;
                void *next;
            } list_of_alarm_values;
        } change_of_characterstring;
        struct {
            uint32_t time_delay;
            BACNET_STATUS_FLAGS selected_flags;
        } change_of_status_flags;
        struct {
            uint8_t tag;
            union {
                bool boolean_value;
                BACNET_UNSIGNED_INTEGER unsigned_value;
                int32_t integer_value;
                uint32_t enumerated;
                BACNET_CHARACTER_STRING character_string;
                BACNET_OCTET_STRING octet_string;
                BACNET_DATE date_pattern;
                BACNET_TIME time_pattern;
                BACNET_OBJECT_ID object_identifier;
                BACNET_DATE_TIME datetime;
            } new_value;
            BACNET_STATUS_FLAGS status_flags;
        } change_of_discrete_value;
        struct {
            uint32_t time_delay;
            struct {
                BACNET_TIMER_STATE value;
                void *next;
            } alarm_values;
            BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE update_time_reference;
        } change_of_timer;
    } parameter;
} BACNET_EVENT_PARAMETER;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/***************************************************
**
** Creates a Confirmed Event Notification APDU
**
****************************************************/
BACNET_STACK_EXPORT
int cevent_notify_encode_apdu(
    uint8_t *apdu,
    uint8_t invoke_id,
    const BACNET_EVENT_NOTIFICATION_DATA *data);

/***************************************************
**
** Creates an Unconfirmed Event Notification APDU
**
****************************************************/
BACNET_STACK_EXPORT
int uevent_notify_encode_apdu(
    uint8_t *apdu, const BACNET_EVENT_NOTIFICATION_DATA *data);

/***************************************************
**
** Encodes the service data part of Event Notification
**
****************************************************/
BACNET_STACK_EXPORT
int event_notify_encode_service_request(
    uint8_t *apdu, const BACNET_EVENT_NOTIFICATION_DATA *data);

BACNET_STACK_EXPORT
size_t event_notification_service_request_encode(
    uint8_t *apdu,
    size_t apdu_size,
    const BACNET_EVENT_NOTIFICATION_DATA *data);

/***************************************************
**
** Decodes the service data part of Event Notification
**
****************************************************/
BACNET_STACK_EXPORT
int event_notify_decode_service_request(
    const uint8_t *apdu,
    unsigned apdu_len,
    BACNET_EVENT_NOTIFICATION_DATA *data);

/***************************************************
**
** Sends an Unconfirmed Event Notification to a dest
**
****************************************************/
BACNET_STACK_EXPORT
int uevent_notify_send(
    uint8_t *buffer,
    BACNET_EVENT_NOTIFICATION_DATA *data,
    BACNET_ADDRESS *dest);

#ifdef __cplusplus
}
#endif /* __cplusplus */
/** @defgroup ALMEVNT Alarm and Event Management BIBBs
 * These BIBBs prescribe the BACnet capabilities required to interoperably
 * perform the alarm and event management functions enumerated in 22.2.1.2
 * for the BACnet devices defined therein.
 */
/** @defgroup EVNOTFCN Alarm and Event-Notification (AE-N)
 * @ingroup ALMEVNT
 * 13.6 ConfirmedCOVNotification Service <br>
 * The ConfirmedCOVNotification service is used to notify subscribers about
 * changes that may have occurred to the properties of a particular object.
 * Subscriptions for COV notifications are made using the SubscribeCOV service
 * or the SubscribeCOVProperty service.
 *
 * 13.7 UnconfirmedCOVNotification Service <br>
 * The UnconfirmedCOVNotification Service is used to notify subscribers about
 * changes that may have occurred to the properties of a particular object,
 * or to distribute object properties of wide interest (such as outside air
 * conditions) to many devices simultaneously without a subscription.
 * Subscriptions for COV notifications are made using the SubscribeCOV service.
 * For unsubscribed notifications, the algorithm for determining when to issue
 * this service is a local matter and may be based on a change of value,
 * periodic updating, or some other criteria.
 */
/** @defgroup ALMACK  Alarm and Event-ACK (AE-ACK)
 * @ingroup ALMEVNT
 * 13.5 AcknowledgeAlarm Service <br>
 * In some systems a device may need to know that an operator has seen the alarm
 * notification. The AcknowledgeAlarm service is used by a notification-client
 * to acknowledge that a human operator has seen and responded to an event
 * notification with 'AckRequired' = TRUE. Ensuring that the acknowledgment
 * actually comes from a person with appropriate authority is a local matter.
 * This service may be used in conjunction with either the
 * ConfirmedEventNotification service or the
 * UnconfirmedEventNotificationservice.
 */
#endif /* BACNET_EVENT_H_ */
