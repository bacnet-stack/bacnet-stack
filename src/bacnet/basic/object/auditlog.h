/**
 * @file
 * @author Mikhail Antropov
 * @date Jul 2023
 * @brief Auditlog object, customize for your use
 *
 * @section DESCRIPTION
 *
 * An Audit Log object combines audit notifications from operation sources and
 * operation targets and stores the combined record in an internal buffer for
 * subsequent retrieval. Each timestamped buffer entry is called an audit log
 * "record."
 *
 * @section LICENSE
 *
 * Copyright (C) 2023 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef AUDITLOG_H
#define AUDITLOG_H

#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdest.h"
#include "bacnet/cov.h"
#include "bacnet/datetime.h"
#include "bacnet/readrange.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Storage structures for Audit Log record
 *
 * Note. I've tried to minimize the storage requirements here
 * as the memory requirements for logging in embedded
 * implementations are frequently a big issue. For PC or
 * embedded Linux type setupz this may seem like overkill
 * but if you have limited memory and need to squeeze as much
 * logging capacity as possible every little byte counts!
 */

/*
 * BACnetAuditOperation
 */
enum {
    AL_OPERATION_READ = 0,
    AL_OPERATION_WRITE = 1,
    AL_OPERATION_CREATE = 2,
    AL_OPERATION_DELETE = 3,
    AL_OPERATION_LIFE_SAFETY = 4,
    AL_OPERATION_ACKNOWLEDGE_ALARM = 5,
    AL_OPERATION_DEVICE_DISABLE_COMM = 6,
    AL_OPERATION_DEVICE_ENABLE_COMM = 7,
    AL_OPERATION_DEVICE_RESET = 8,
    AL_OPERATION_DEVICE_BACKUP = 9,
    AL_OPERATION_DEVICE_RESTORE = 10,
    AL_OPERATION_SUBSCRIPTION = 11,
    AL_OPERATION_NOTIFICATION = 12,
    AL_OPERATION_AUDITING_FAILURE = 13,
    AL_OPERATION_NETWORK_CHANGES = 14,
    AL_OPERATION_GENERAL = 15,
};

/*
 * BACnetAuditNotification ::= SEQUENCE {
 *      source-timestamp [0] BACnetTimeStamp OPTIONAL,
 *      target-timestamp [1] BACnetTimeStamp OPTIONAL,
 *      source-device    [2] BACnetRecipient,
 *      source-object    [3] BACnetObjectIdentifier OPTIONAL,
 *      operation        [4] BACnetAuditOperation,
 *      source-comment   [5] CharacterString OPTIONAL,
 *      target-comment   [6] CharacterString OPTIONAL,
 *      invoke-id        [7] Unsigned8 OPTIONAL,
 *      source-user-id   [8] Unsigned16 OPTIONAL,
 *      source-user-role [9] Unsigned8 OPTIONAL,
 *      target-device   [10] BACnetRecipient,
 *      target-object   [11] BACnetObjectIdentifier OPTIONAL,
 *      target-property [12] BACnetPropertyReference OPTIONAL,
 *      target-priority [13] Unsigned (1..16) OPTIONAL,
 *      target-value    [14] ABSTRACT-SYNTAX.&Type OPTIONAL,
 *      current-value   [15] ABSTRACT-SYNTAX.&Type OPTIONAL,
 *      result          [16] Error OPTIONAL
 * }
 */
typedef struct al_notification {
    BACNET_RECIPIENT source_device; /* [2] BACnetRecipient */
    uint8_t operation; /* [4] BACnetAuditOperation */
    BACNET_RECIPIENT target_device; /* [10] BACnetRecipient */
} AL_NOTIFICATION;

/*
 * Data types associated with a BACnet Log Record. We use these for managing the
 * log buffer but they are also the tag numbers to use when encoding/decoding
 * the log datum field.
 */

#define AL_TYPE_STATUS 0
#define AL_TYPE_NOTIFICATION 1
#define AL_TYPE_TIME_CHANGE 2

/*
 * BACnetAuditLogRecord ::= SEQUENCE {
 *      timestamp [0] BACnetDateTime,
 *      log-datum [1] CHOICE {
 *          log-status [0] BACnetLogStatus,
 *          audit-notification [1] BACnetAuditNotification,
 *          time-change [2] REAL
 *      }
 * }
 */
typedef struct al_log_record {
    bacnet_time_t tTimeStamp; /* When the event occurred */
    uint8_t ucRecType; /* What type of Event */
    union {
        uint8_t ucLogStatus;
        AL_NOTIFICATION notification;
        float time_change;
    } Datum;
} AL_LOG_REC;

#define AL_MAX_ENTRIES 1000 /* Entries per datalog */

/* Structure containing config and status info for a Audit Log */

typedef struct al_log_info {
    bool bEnable; /* Audit log is active when this is true */
    bool out_of_service;
    uint32_t ulRecordCount; /* Count of items currently in the buffer */
    uint32_t ulTotalRecordCount; /* Count of all items that have ever been
                                    inserted into the buffer */
    int iIndex; /* Current insertion point */
} AL_LOG_INFO;

BACNET_STACK_EXPORT
void Audit_Log_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary);

BACNET_STACK_EXPORT
bool Audit_Log_Valid_Instance(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned Audit_Log_Count(void);
BACNET_STACK_EXPORT
uint32_t Audit_Log_Index_To_Instance(unsigned index);
BACNET_STACK_EXPORT
unsigned Audit_Log_Instance_To_Index(uint32_t instance);
BACNET_STACK_EXPORT
bool Audit_Log_Object_Instance_Add(uint32_t instance);

BACNET_STACK_EXPORT
bool Audit_Log_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);

BACNET_STACK_EXPORT
int Audit_Log_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);

BACNET_STACK_EXPORT
bool Audit_Log_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);

BACNET_STACK_EXPORT
void Audit_Log_Init(void);

BACNET_STACK_EXPORT
void AL_Insert_Status_Rec(
    uint32_t instance, BACNET_LOG_STATUS eStatus, bool bState);

BACNET_STACK_EXPORT
void AL_Insert_Notification_Rec(uint32_t instance, AL_NOTIFICATION *notif);

BACNET_STACK_EXPORT
bool AL_Enable(uint32_t instance);

BACNET_STACK_EXPORT
void AL_Enable_Set(uint32_t instance, bool bEnable);

BACNET_STACK_EXPORT
bool AL_Out_Of_Service(uint32_t instance);

BACNET_STACK_EXPORT
void AL_Out_Of_Service_Set(uint32_t instance, bool b);

BACNET_STACK_EXPORT
bacnet_time_t AL_BAC_Time_To_Local(BACNET_DATE_TIME *SourceTime);

BACNET_STACK_EXPORT
void AL_Local_Time_To_BAC(BACNET_DATE_TIME *DestTime, bacnet_time_t SourceTime);

BACNET_STACK_EXPORT
int AL_encode_entry(uint8_t *apdu, int iLog, int iEntry);

BACNET_STACK_EXPORT
int AL_encode_by_position(uint8_t *apdu, BACNET_READ_RANGE_DATA *pRequest);

BACNET_STACK_EXPORT
int AL_encode_by_sequence(uint8_t *apdu, BACNET_READ_RANGE_DATA *pRequest);

BACNET_STACK_EXPORT
int AL_encode_by_time(uint8_t *apdu, BACNET_READ_RANGE_DATA *pRequest);

BACNET_STACK_EXPORT
int AL_log_encode(uint8_t *apdu, BACNET_READ_RANGE_DATA *pRequest);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
