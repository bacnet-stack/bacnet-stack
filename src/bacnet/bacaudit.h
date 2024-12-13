/**
 * @file
 * @brief API for BACnetAuditNotification and BACnetAuditLogRecord codec used
 * by Audit Log objects
 * @author Mikhail Antropov <michail.antropov@dsr-corporation.com>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date November 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_AUDIT_H
#define BACNET_AUDIT_H
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
/* BACnet Stack API */
#include "bacnet/bacdest.h"
#include "bacnet/cov.h"
#include "bacnet/datetime.h"

/**
 * @brief Smaller version of BACnet_Application_Data_Value
 * @note This must be a separate struct to avoid recursive structure.
 * Keeping it small also helps keep the size small.
 */
typedef struct BACnet_Audit_Value {
    uint8_t tag; /* application tag data type */
    union {
        /* NULL - not needed as it is encoded in the tag alone */
#if defined(BACAPP_BOOLEAN)
        bool Boolean;
#endif
#if defined(BACAPP_UNSIGNED)
        BACNET_UNSIGNED_INTEGER Unsigned_Int;
#endif
#if defined(BACAPP_SIGNED)
        int32_t Signed_Int;
#endif
#if defined(BACAPP_REAL)
        float Real;
#endif
#if defined(BACAPP_DOUBLE)
        double Double;
#endif
#if defined(BACAPP_ENUMERATED)
        uint32_t Enumerated;
#endif
    } type;
} BACNET_AUDIT_VALUE;

/**
 * @brief Storage structures for Audit Log record
 *
 * @note I've tried to minimize the storage requirements here
 * as the memory requirements for logging in embedded
 * implementations are frequently a big issue. For PC or
 * embedded Linux type setupz this may seem like overkill
 * but if you have limited memory and need to squeeze as much
 * logging capacity as possible every little byte counts!
 */
#ifdef BACNET_AUDIT_NOTIFICATION_OPTIONAL_ENABLE
#define BACNET_AUDIT_NOTIFICATION_SOURCE_TIMESTAMP_ENABLE
#define BACNET_AUDIT_NOTIFICATION_TARGET_TIMESTAMP_ENABLE
#define BACNET_AUDIT_NOTIFICATION_SOURCE_OBJECT_ENABLE
#define BACNET_AUDIT_NOTIFICATION_SOURCE_COMMENT_ENABLE
#define BACNET_AUDIT_NOTIFICATION_TARGET_COMMENT_ENABLE
#define BACNET_AUDIT_NOTIFICATION_INVOKE_ID_ENABLE
#define BACNET_AUDIT_NOTIFICATION_SOURCE_USER_ID_ENABLE
#define BACNET_AUDIT_NOTIFICATION_SOURCE_USER_ROLE_ENABLE
#define BACNET_AUDIT_NOTIFICATION_TARGET_OBJECT_ENABLE
#define BACNET_AUDIT_NOTIFICATION_TARGET_PROPERTY_ENABLE
#define BACNET_AUDIT_NOTIFICATION_TARGET_PRIORITY_ENABLE
#define BACNET_AUDIT_NOTIFICATION_TARGET_VALUE_ENABLE
#define BACNET_AUDIT_NOTIFICATION_CURRENT_VALUE_ENABLE
#define BACNET_AUDIT_NOTIFICATION_RESULT_ENABLE
#endif

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
typedef struct BACnetAuditNotification {
#ifdef BACNET_AUDIT_NOTIFICATION_SOURCE_TIMESTAMP_ENABLE
    /* source-timestamp [0] BACnetTimeStamp OPTIONAL */
    BACNET_TIMESTAMP source_timestamp;
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_TARGET_TIMESTAMP_ENABLE
    /* target-timestamp [1] BACnetTimeStamp OPTIONAL */
    BACNET_TIMESTAMP target_timestamp;
#endif
    /* source-device    [2] BACnetRecipient */
    BACNET_RECIPIENT source_device;
#ifdef BACNET_AUDIT_NOTIFICATION_SOURCE_OBJECT_ENABLE
    /* source-object    [3] BACnetObjectIdentifier OPTIONAL */
    BACNET_OBJECT_ID source_object;
#endif
    /* operation        [4] BACnetAuditOperation */
    uint8_t operation;
#ifdef BACNET_AUDIT_NOTIFICATION_SOURCE_COMMENT_ENABLE
    /* source-comment   [5] CharacterString OPTIONAL */
    BACNET_CHARACTER_STRING source_comment;
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_TARGET_COMMENT_ENABLE
    /* target-comment   [6] CharacterString OPTIONAL */
    BACNET_CHARACTER_STRING target_comment;
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_INVOKE_ID_ENABLE
    /* invoke-id        [7] Unsigned8 OPTIONAL */
    uint8_t invoke_id;
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_SOURCE_USER_ID_ENABLE
    /* source-user-id   [8] Unsigned16 OPTIONAL */
    uint16_t source_user_id;
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_SOURCE_USER_ROLE_ENABLE
    /* source-user-role [9] Unsigned8 OPTIONAL */
    uint8_t source_user_role;
#endif
    /* target-device   [10] BACnetRecipient */
    BACNET_RECIPIENT target_device;
#ifdef BACNET_AUDIT_NOTIFICATION_TARGET_OBJECT_ENABLE
    /* target-object   [11] BACnetObjectIdentifier OPTIONAL */
    BACNET_OBJECT_ID target_object;
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_TARGET_PROPERTY_ENABLE
    /* target-property [12] BACnetPropertyReference OPTIONAL */
    BACNET_PROPERTY_REFERENCE target_property;
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_TARGET_PRIORITY_ENABLE
    /* target-priority [13] Unsigned (1..16) OPTIONAL */
    uint8_t target_priority;
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_TARGET_VALUE_ENABLE
    /* target-value    [14] ABSTRACT-SYNTAX.&Type OPTIONAL */
    BACNET_AUDIT_VALUE target_value;
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_CURRENT_VALUE_ENABLE
    /* current-value   [15] ABSTRACT-SYNTAX.&Type OPTIONAL */
    BACNET_AUDIT_VALUE current_value;
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_RESULT_ENABLE
    /* result          [16] Error OPTIONAL */
    ERROR_CODE result;
#endif
} BACNET_AUDIT_NOTIFICATION;

/**
 * @brief Datum types associated with a BACnet Log Record. We use these for
 * managing the log buffer but they are also the tag numbers to use when
 * encoding or decoding the log datum field.
 */
#define AUDIT_LOG_DATUM_TAG_STATUS 0
#define AUDIT_LOG_DATUM_TAG_NOTIFICATION 1
#define AUDIT_LOG_DATUM_TAG_TIME_CHANGE 2

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
typedef struct BACnetAuditLogRecord {
    BACNET_DATE_TIME time_stamp;
    uint8_t tag;
    union {
        uint8_t log_status;
        BACNET_AUDIT_NOTIFICATION notification;
        float time_change;
    } datum;
} BACNET_AUDIT_LOG_RECORD;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int bacnet_audit_log_record_encode(
    uint8_t *apdu, const BACNET_AUDIT_LOG_RECORD *value);
BACNET_STACK_EXPORT
int bacnet_audit_log_record_decode(
    const uint8_t *apdu, uint32_t apdu_size, BACNET_AUDIT_LOG_RECORD *value);
BACNET_STACK_EXPORT
bool bacnet_audit_log_record_same(
    const BACNET_AUDIT_LOG_RECORD *value1,
    const BACNET_AUDIT_LOG_RECORD *value2);

BACNET_STACK_EXPORT
int bacnet_audit_log_notification_encode(
    uint8_t *apdu, const BACNET_AUDIT_NOTIFICATION *value);
BACNET_STACK_EXPORT
int bacnet_audit_log_notification_decode(
    const uint8_t *apdu, uint32_t apdu_size, BACNET_AUDIT_NOTIFICATION *value);
BACNET_STACK_EXPORT
int bacnet_audit_log_notification_context_decode(
    const uint8_t *apdu,
    uint16_t apdu_size,
    uint8_t tag_number,
    BACNET_AUDIT_NOTIFICATION *value);
BACNET_STACK_EXPORT
bool bacnet_audit_log_notification_same(
    const BACNET_AUDIT_NOTIFICATION *value1,
    const BACNET_AUDIT_NOTIFICATION *value2);

BACNET_STACK_EXPORT
int bacnet_audit_value_encode(uint8_t *apdu, const BACNET_AUDIT_VALUE *value);
BACNET_STACK_EXPORT
int bacnet_audit_value_context_encode(
    uint8_t *apdu, uint8_t tag_number, const BACNET_AUDIT_VALUE *value);
BACNET_STACK_EXPORT
int bacnet_audit_value_decode(
    const uint8_t *apdu, uint32_t apdu_size, BACNET_AUDIT_VALUE *value);
BACNET_STACK_EXPORT
bool bacnet_audit_value_same(
    const BACNET_AUDIT_VALUE *value1, const BACNET_AUDIT_VALUE *value2);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
