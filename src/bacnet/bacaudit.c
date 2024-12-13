/**
 * @file
 * @brief BACnetActionCommand codec used by Command objects
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date November 2024
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bactimevalue.h"
#include "bacnet/timestamp.h"
/* me! */
#include "bacnet/bacaudit.h"

/**
 * @brief Encode the BACnetAuditValue
 * @param apdu - buffer of data to be encoded, or NULL for length
 * @param value - value to be encoded
 * @return the number of apdu bytes encoded
 */
int bacnet_audit_value_encode(uint8_t *apdu, const BACNET_AUDIT_VALUE *value)
{
    /* total length of the apdu, return value */
    int apdu_len = 0;

    if (!value) {
        return 0;
    }
    switch (value->tag) {
#if defined(BACAPP_NULL)
        case BACNET_APPLICATION_TAG_NULL:
            if (apdu) {
                apdu[0] = value->tag;
            }
            apdu_len++;
            break;
#endif
#if defined(BACAPP_BOOLEAN)
        case BACNET_APPLICATION_TAG_BOOLEAN:
            apdu_len = encode_application_boolean(apdu, value->type.Boolean);
            break;
#endif
#if defined(BACAPP_UNSIGNED)
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
            apdu_len =
                encode_application_unsigned(apdu, value->type.Unsigned_Int);
            break;
#endif
#if defined(BACAPP_SIGNED)
        case BACNET_APPLICATION_TAG_SIGNED_INT:
            apdu_len = encode_application_signed(apdu, value->type.Signed_Int);
            break;
#endif
#if defined(BACAPP_REAL)
        case BACNET_APPLICATION_TAG_REAL:
            apdu_len = encode_application_real(apdu, value->type.Real);
            break;
#endif
#if defined(BACAPP_DOUBLE)
        case BACNET_APPLICATION_TAG_DOUBLE:
            apdu_len = encode_application_double(apdu, value->type.Double);
            break;
#endif
#if defined(BACAPP_ENUMERATED)
        case BACNET_APPLICATION_TAG_ENUMERATED:
            apdu_len =
                encode_application_enumerated(apdu, value->type.Enumerated);
            break;
#endif
        default:
            break;
    }

    return apdu_len;
}

/**
 * @brief Encode the BACnetAuditValue
 * @param apdu - buffer of data to be encoded, or NULL for length
 * @param tag_number - context tag number to be encoded
 * @param value - value to be encoded
 * @return the number of apdu bytes encoded
 */
int bacnet_audit_value_context_encode(
    uint8_t *apdu, uint8_t tag_number, const BACNET_AUDIT_VALUE *value)
{
    int len;
    int apdu_len = 0;

    len = encode_opening_tag(apdu, tag_number);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = bacnet_audit_value_encode(apdu, value);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_closing_tag(apdu, tag_number);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Decode the BACnetAuditValue
 * @param apdu - buffer of data to be encoded, or NULL for length
 * @param tag_number - context tag number to be encoded
 * @param value - value to be encoded
 * @return the number of apdu bytes encoded, or BACNET_STATUS_ERROR if an error
 * occurs
 */
int bacnet_audit_value_decode(
    const uint8_t *apdu, uint32_t apdu_size, BACNET_AUDIT_VALUE *value)
{
    int len = 0;
    int apdu_len = 0;
    BACNET_TAG tag = { 0 };

    if (!value) {
        return BACNET_STATUS_ERROR;
    }
    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    len = bacnet_tag_decode(apdu, apdu_size, &tag);
    if ((len > 0) && tag.application) {
        if (value) {
            value->tag = tag.number;
        }
        switch (tag.number) {
#if defined(BACAPP_NULL)
            case BACNET_APPLICATION_TAG_NULL:
                apdu_len = len;
                break;
#endif
#if defined(BACAPP_BOOLEAN)
            case BACNET_APPLICATION_TAG_BOOLEAN:
                apdu_len = bacnet_boolean_application_decode(
                    apdu, apdu_size, &value->type.Boolean);
                break;
#endif
#if defined(BACAPP_UNSIGNED)
            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                apdu_len = bacnet_unsigned_application_decode(
                    apdu, apdu_size, &value->type.Unsigned_Int);
                break;
#endif
#if defined(BACAPP_SIGNED)
            case BACNET_APPLICATION_TAG_SIGNED_INT:
                apdu_len = bacnet_signed_application_decode(
                    apdu, apdu_size, &value->type.Signed_Int);
                break;
#endif
#if defined(BACAPP_REAL)
            case BACNET_APPLICATION_TAG_REAL:
                apdu_len = bacnet_real_application_decode(
                    apdu, apdu_size, &value->type.Real);
                break;
#endif
#if defined(BACAPP_DOUBLE)
            case BACNET_APPLICATION_TAG_DOUBLE:
                apdu_len = bacnet_double_application_decode(
                    apdu, apdu_size, &value->type.Double);
                break;
#endif
#if defined(BACAPP_ENUMERATED)
            case BACNET_APPLICATION_TAG_ENUMERATED:
                apdu_len = bacnet_enumerated_application_decode(
                    apdu, apdu_size, &value->type.Enumerated);
                break;
#endif
            default:
                break;
        }
    } else {
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/**
 * @brief Compare two BACnetActionPropertyValue complex datatypes
 * @param value1 [in] The first structure to compare
 * @param value2 [in] The second structure to compare
 * @return true if the two structures are the same
 */
bool bacnet_audit_value_same(
    const BACNET_AUDIT_VALUE *value1, const BACNET_AUDIT_VALUE *value2)
{
    bool status = false; /*return value */

    if ((value1 == NULL) || (value2 == NULL)) {
        return false;
    }
    if (value1->tag == value2->tag) {
        switch (value1->tag) {
#if defined(BACAPP_NULL)
            case BACNET_APPLICATION_TAG_NULL:
                status = true;
                break;
#endif
#if defined(BACAPP_BOOLEAN)
            case BACNET_APPLICATION_TAG_BOOLEAN:
                status = value1->type.Boolean == value2->type.Boolean;
                break;
#endif
#if defined(BACAPP_UNSIGNED)
            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                status = value1->type.Unsigned_Int == value2->type.Unsigned_Int;
                break;
#endif
#if defined(BACAPP_SIGNED)
            case BACNET_APPLICATION_TAG_SIGNED_INT:
                status = value1->type.Signed_Int == value2->type.Signed_Int;
                break;
#endif
#if defined(BACAPP_REAL)
            case BACNET_APPLICATION_TAG_REAL:
                status = !islessgreater(value1->type.Real, value2->type.Real);
                break;
#endif
#if defined(BACAPP_DOUBLE)
            case BACNET_APPLICATION_TAG_DOUBLE:
                status =
                    !islessgreater(value1->type.Double, value2->type.Double);
                break;
#endif
#if defined(BACAPP_ENUMERATED)
            case BACNET_APPLICATION_TAG_ENUMERATED:
                status = value1->type.Enumerated == value2->type.Enumerated;
                break;
#endif
            default:
                break;
        }
    }

    return status;
}

/**
 * @brief Encode the BACnetAuditNotification
 * @param apdu - buffer of data to be encoded, or NULL for length
 * @param value - value to be encoded
 * @return the number of apdu bytes encoded
 */
int bacnet_audit_log_notification_encode(
    uint8_t *apdu, const BACNET_AUDIT_NOTIFICATION *value)
{
    int len, apdu_len = 0; /* total length of the apdu, return value */

    if (!value) {
        return 0;
    }
#ifdef BACNET_AUDIT_NOTIFICATION_SOURCE_TIMESTAMP_ENABLE
    /* source-timestamp [0] BACnetTimeStamp OPTIONAL */
    len = bacapp_encode_context_timestamp(apdu, 0, &value->source_timestamp);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_TARGET_TIMESTAMP_ENABLE
    /* target-timestamp [1] BACnetTimeStamp OPTIONAL */
    len = bacapp_encode_context_timestamp(apdu, 0, &value->target_timestamp);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
#endif
    /* source-device    [2] BACnetRecipient */
    len = bacnet_recipient_context_encode(apdu, 2, &value->source_device);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
#ifdef BACNET_AUDIT_NOTIFICATION_SOURCE_OBJECT_ENABLE
    /* source-object    [3] BACnetObjectIdentifier OPTIONAL */
    len = encode_context_object_id(apdu, 3, &value->source_object);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
#endif
    /* operation        [4] BACnetAuditOperation */
    len = encode_context_unsigned(apdu, 4, value->operation);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
#ifdef BACNET_AUDIT_NOTIFICATION_SOURCE_COMMENT_ENABLE
    /* source-comment   [5] CharacterString OPTIONAL */
    len = encode_context_character_string(apdu, 5, &value->source_comment);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_TARGET_COMMENT_ENABLE
    /* target-comment   [6] CharacterString OPTIONAL */
    len = encode_context_character_string(apdu, 6, &value->target_comment);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_INVOKE_ID_ENABLE
    /* invoke-id        [7] Unsigned8 OPTIONAL */
    len = encode_context_unsigned(apdu, 7, value->invoke_id);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_SOURCE_USER_ID_ENABLE
    /* source-user-id   [8] Unsigned16 OPTIONAL */
    len = encode_context_unsigned(apdu, 8, value->source_user_id);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_SOURCE_USER_ROLE_ENABLE
    /* source-user-role [9] Unsigned8 OPTIONAL */
    len = encode_context_unsigned(apdu, 9, value->source_user_role);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
#endif
    /* target-device   [10] BACnetRecipient */
    len = bacnet_recipient_context_encode(apdu, 10, &value->target_device);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
#ifdef BACNET_AUDIT_NOTIFICATION_TARGET_OBJECT_ENABLE
    /* target-object   [11] BACnetObjectIdentifier OPTIONAL */
    len = encode_context_object_id(apdu, 11, &value->target_object);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_TARGET_PROPERTY_ENABLE
    /* target-property [12] BACnetPropertyReference OPTIONAL */
    len = bacnet_property_reference_context_encode(
        apdu, 12, &value->target_property);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_TARGET_PRIORITY_ENABLE
    /* target-priority [13] Unsigned (1..16) OPTIONAL */
    len = encode_context_unsigned(apdu, 13, value->target_priority);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_TARGET_VALUE_ENABLE
    /* target-value    [14] ABSTRACT-SYNTAX.&Type OPTIONAL */
    len = bacnet_audit_value_context_encode(apdu, 14, &value->target_value);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_CURRENT_VALUE_ENABLE
    /* current-value   [15] ABSTRACT-SYNTAX.&Type OPTIONAL */
    len = bacnet_audit_value_context_encode(apdu, 15, &value->current_value);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_RESULT_ENABLE
    /* result          [16] Error OPTIONAL */
    len = encode_context_enumerated(apdu, 16, value->result);
    apdu_len += len;
#endif

    return apdu_len;
}

/**
 * @brief Decode the BACnetAuditNotification
 * @param apdu - buffer of data to be decoded
 * @param value - value to hold the decoded data
 * @return the number of apdu bytes encoded, or BACNET_STATUS_ERROR if an error
 * occurs
 */
int bacnet_audit_log_notification_decode(
    const uint8_t *apdu, uint32_t apdu_size, BACNET_AUDIT_NOTIFICATION *value)
{
    int len = 0;
    int apdu_len = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value;

    if (!value) {
        return BACNET_STATUS_ERROR;
    }
    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
#ifdef BACNET_AUDIT_NOTIFICATION_SOURCE_TIMESTAMP_ENABLE
    /* source-timestamp [0] BACnetTimeStamp OPTIONAL */
    len = bacnet_timestamp_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 0, &value->source_timestamp);
    if (len > 0) {
        apdu_len += len;
    }
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_TARGET_TIMESTAMP_ENABLE
    /* target-timestamp [1] BACnetTimeStamp OPTIONAL */
    len = bacnet_timestamp_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 1, &value->target_timestamp);
    if (len > 0) {
        apdu_len += len;
    }
#endif
    /* source-device    [2] BACnetRecipient */
    len = bacnet_recipient_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 2, &value->source_device);
    if (len > 0) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
#ifdef BACNET_AUDIT_NOTIFICATION_SOURCE_OBJECT_ENABLE
    /* source-object    [3] BACnetObjectIdentifier OPTIONAL */
    len = bacnet_object_id_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 3, &value->source_object.type,
        &value->source_object.instance);
    if (len > 0) {
        apdu_len += len;
    }
#endif
    /* operation        [4] BACnetAuditOperation */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 4, &unsigned_value);
    if (len > 0) {
        if (unsigned_value < AUDIT_OPERATION_MAX) {
            value->operation = (BACNET_AUDIT_OPERATION)unsigned_value;
        } else {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
#ifdef BACNET_AUDIT_NOTIFICATION_SOURCE_COMMENT_ENABLE
    /* source-comment   [5] CharacterString OPTIONAL */
    len = encode_context_character_string(
        &apdu[apdu_len], apdu_size - apdu_len, 5, &value->source_comment);
    if (len > 0) {
        apdu_len += len;
    }
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_TARGET_COMMENT_ENABLE
    /* target-comment   [6] CharacterString OPTIONAL */
    len = bacnet_character_string_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 6, &value->target_comment);
    if (len > 0) {
        apdu_len += len;
    }
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_INVOKE_ID_ENABLE
    /* invoke-id        [7] Unsigned8 OPTIONAL */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 7, &unsigned_value);
    if (len > 0) {
        apdu_len += len;
        if (unsigned_value > UINT8_MAX) {
            return BACNET_STATUS_ERROR;
        }
        value->invoke_id = (uint8_t)unsigned_value;
    }
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_SOURCE_USER_ID_ENABLE
    /* source-user-id   [8] Unsigned16 OPTIONAL */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 8, &unsigned_value);
    if (len > 0) {
        apdu_len += len;
        if (unsigned_value > UINT16_MAX) {
            return BACNET_STATUS_ERROR;
        }
        value->source_user_id = (uint16_t)unsigned_value;
    }
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_SOURCE_USER_ROLE_ENABLE
    /* source-user-role [9] Unsigned8 OPTIONAL */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 9, &unsigned_value);
    if (len > 0) {
        apdu_len += len;
        if (unsigned_value > UINT8_MAX) {
            return BACNET_STATUS_ERROR;
        }
        value->source_user_role = (uint8_t)unsigned_value;
    }
#endif
    /* target-device   [10] BACnetRecipient */
    len = bacnet_recipient_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 10, &value->target_device);
    if (len > 0) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
#ifdef BACNET_AUDIT_NOTIFICATION_TARGET_OBJECT_ENABLE
    /* target-object   [11] BACnetObjectIdentifier OPTIONAL */
    len = bacnet_object_id_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 11, &value->target_object.type,
        &value->target_object.instance);
    if (len > 0) {
        apdu_len += len;
    }
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_TARGET_PROPERTY_ENABLE
    /* target-property [12] BACnetPropertyReference OPTIONAL */
    len = bacnet_property_reference_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 12, &value->target_property);
    if (len > 0) {
        apdu_len += len;
    }
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_TARGET_PRIORITY_ENABLE
    /* target-priority [13] Unsigned (1..16) OPTIONAL */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 13, &unsigned_value);
    if (len > 0) {
        apdu_len += len;
        if (unsigned_value > UINT8_MAX) {
            return BACNET_STATUS_ERROR;
        }
        value->target_priority = (uint8_t)unsigned_value;
    }
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_TARGET_VALUE_ENABLE
    /* target-value    [14] ABSTRACT-SYNTAX.&Type OPTIONAL */
    len = bacnet_audit_value_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 14, &value->target_value);
    if (len > 0) {
        apdu_len += len;
    }
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_CURRENT_VALUE_ENABLE
    /* current-value   [15] ABSTRACT-SYNTAX.&Type OPTIONAL */
    len = bacnet_audit_value_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 15, &value->current_value);
    if (len > 0) {
        apdu_len += len;
    }
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_RESULT_ENABLE
    /* result          [16] Error OPTIONAL */
    len = bacnet_enumerated_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 16, value->result);
    if (len > 0) {
        apdu_len += len;
    }
#endif

    return apdu_len;
}

/**
 * @brief Decode a context BACnetAuditLogNotification complext data.
 * Check for an opening tag and a closing tag as well.
 *
 * @param apdu  Pointer to the buffer containing the encoded value
 * @param apdu_size Size of the buffer containing the encoded value
 * @param tag_number  Tag number
 * @param value  Pointer to the structure that shall be decoded into.
 *
 * @return number of bytes decoded or BACNET_STATUS_ERROR on failure.
 */
int bacnet_audit_log_notification_context_decode(
    const uint8_t *apdu,
    uint16_t apdu_size,
    uint8_t tag_number,
    BACNET_AUDIT_NOTIFICATION *value)
{
    int len = 0;
    int apdu_len = 0;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    if (!bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag_number, &len)) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    len = bacnet_audit_log_notification_decode(
        &apdu[apdu_len], apdu_size - apdu_len, value);
    if (len > 0) {
        apdu_len += len;
        if (bacnet_is_closing_tag_number(
                &apdu[apdu_len], apdu_size - apdu_len, tag_number, &len)) {
            apdu_len += len;
        } else {
            return BACNET_STATUS_ERROR;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/**
 * @brief Compare two BACnetAuditNotification for same-ness
 * @param value1 - value 1 structure
 * @param value2 - value 2 structure
 * @return true if the values are the same
 */
bool bacnet_audit_log_notification_same(
    const BACNET_AUDIT_NOTIFICATION *value1,
    const BACNET_AUDIT_NOTIFICATION *value2)
{
    if (!value1 || !value2) {
        return false;
    }
    if (value1->operation != value2->operation) {
        return false;
    }
    if (!bacnet_recipient_same(
            &value1->source_device, &value2->source_device)) {
        return false;
    }
    if (!bacnet_recipient_same(
            &value1->target_device, &value2->target_device)) {
        return false;
    }
#ifdef BACNET_AUDIT_NOTIFICATION_SOURCE_TIMESTAMP_ENABLE
    if (!bacnet_timestamp_same(
            &value1->source_timestamp, &value2->source_timestamp)) {
        return false;
    }
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_TARGET_TIMESTAMP_ENABLE
    if (!bacnet_timestamp_same(
            &value1->target_timestamp, &value2->target_timestamp)) {
        return false;
    }
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_SOURCE_OBJECT_ENABLE
    if (!bacnet_object_id_same(
            &value1->source_object, &value2->source_object)) {
        return false;
    }
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_SOURCE_COMMENT_ENABLE
    if (!bacnet_character_string_same(
            &value1->source_comment, &value2->source_comment)) {
        return false;
    }
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_TARGET_COMMENT_ENABLE
    if (!bacnet_character_string_same(
            &value1->target_comment, &value2->target_comment)) {
        return false;
    }
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_INVOKE_ID_ENABLE
    if (value1->invoke_id != value2->invoke_id) {
        return false;
    }
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_SOURCE_USER_ID_ENABLE
    if (value1->source_user_id != value2->source_user_id) {
        return false;
    }
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_SOURCE_USER_ROLE_ENABLE
    if (value1->source_user_role != value2->source_user_role) {
        return false;
    }
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_TARGET_OBJECT_ENABLE
    if (!bacnet_object_id_same(
            &value1->target_object, &value2->target_object)) {
        return false;
    }
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_TARGET_PROPERTY_ENABLE
    if (!bacnet_property_reference_same(
            &value1->target_property, &value2->target_property)) {
        return false;
    }
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_TARGET_PRIORITY_ENABLE
    if (value1->target_priority != value2->target_priority) {
        return false;
    }
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_TARGET_VALUE_ENABLE
    if (!bacnet_audit_value_same(
            &value1->target_value, &value2->target_value)) {
        return false;
    }
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_CURRENT_VALUE_ENABLE
    if (!bacnet_audit_value_same(
            &value1->current_value, &value2->current_value)) {
        return false;
    }
#endif
#ifdef BACNET_AUDIT_NOTIFICATION_RESULT_ENABLE
    if (value1->result != value2->result) {
        return false;
    }
#endif

    return true;
}

/**
 * @brief Encode property value according to the application tag
 *
 * BACnetAuditLogRecord ::= SEQUENCE {
 *      timestamp [0] BACnetDateTime,
 *      log-datum [1] CHOICE {
 *          log-status [0] BACnetLogStatus,
 *          audit-notification [1] BACnetAuditNotification,
 *          time-change [2] REAL
 *      }
 * }
 *
 * @param apdu - Pointer to the buffer to encode to, or NULL for length
 * @param value - Pointer to the property value to encode from
 * @return number of bytes encoded
 */
int bacnet_audit_log_record_encode(
    uint8_t *apdu, const BACNET_AUDIT_LOG_RECORD *value)
{
    int len, apdu_len = 0; /* total length of the apdu, return value */
    BACNET_BIT_STRING log_status = { 0 };

    if (!value) {
        return 0;
    }
    /* timestamp [0] BACnetDateTime */
    len = bacapp_encode_context_datetime(apdu, 0, &value->time_stamp);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* log-datum [1] CHOICE */
    len = encode_opening_tag(apdu, 1);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    switch (value->tag) {
        case AUDIT_LOG_DATUM_TAG_STATUS:
            /* log-status [0] BACnetLogStatus */
            bitstring_bits_used_set(&log_status, LOG_STATUS_MAX);
            bitstring_set_octet(&log_status, 0, value->datum.log_status);
            len = encode_context_bitstring(apdu, value->tag, &log_status);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            break;
        case AUDIT_LOG_DATUM_TAG_NOTIFICATION:
            /* audit-notification [1] BACnetAuditNotification */
            len = encode_opening_tag(apdu, value->tag);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = bacnet_audit_log_notification_encode(
                apdu, &value->datum.notification);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = encode_closing_tag(apdu, value->tag);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            break;
        case AUDIT_LOG_DATUM_TAG_TIME_CHANGE:
            /* time-change [2] REAL */
            len =
                encode_context_real(apdu, value->tag, value->datum.time_change);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            break;
        default:
            break;
    }
    /* log-datum [1] CHOICE */
    len = encode_closing_tag(apdu, 1);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Decode property value from the application buffer
 * @param apdu - Pointer to the buffer to decode from
 * @param apdu_size Size of the buffer to decode from
 * @param value - Pointer to the property value to decode to
 * @return number of bytes encoded, or BACNET_STATUS_ERROR if an error occurs
 */
int bacnet_audit_log_record_decode(
    const uint8_t *apdu, uint32_t apdu_size, BACNET_AUDIT_LOG_RECORD *value)
{
    int len = 0;
    int apdu_len = 0;
    BACNET_TAG tag = { 0 };
    BACNET_DATE_TIME bdatetime = { 0 };
    BACNET_BIT_STRING log_status = { 0 };
    BACNET_AUDIT_NOTIFICATION notification = { 0 };
    float real_value = 0.0f;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    /* timestamp [0] BACnetDateTime */
    len = bacnet_datetime_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 0, &bdatetime);
    if (len > 0) {
        if (value) {
            datetime_copy(&value->time_stamp, &bdatetime);
        }
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* log-datum [1] CHOICE */
    if (bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 1, &len)) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    len = bacnet_tag_decode(&apdu[apdu_len], apdu_size - apdu_len, &tag);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    if (value) {
        value->tag = tag.number;
    }
    /* ignore the len. len is included in context decoder below. */
    switch (tag.number) {
        case AUDIT_LOG_DATUM_TAG_STATUS:
            /* log-status [0] BACnetLogStatus */
            len = bacnet_bitstring_context_decode(
                &apdu[apdu_len], apdu_size - apdu_len, tag.number, &log_status);
            if (len > 0) {
                if (value) {
                    value->datum.log_status = bitstring_octet(&log_status, 0);
                }
                apdu_len += len;
            } else {
                return BACNET_STATUS_ERROR;
            }
            break;
        case AUDIT_LOG_DATUM_TAG_NOTIFICATION:
            /* audit-notification [1] BACnetAuditNotification */
            len = bacnet_audit_log_notification_context_decode(
                &apdu[apdu_len], apdu_size - apdu_len, tag.number,
                &notification);
            if (len > 0) {
                if (value) {
                    memmove(
                        &value->datum.notification, &notification,
                        sizeof(BACNET_AUDIT_NOTIFICATION));
                }
                apdu_len += len;
            } else {
                return BACNET_STATUS_ERROR;
            }
            break;
        case AUDIT_LOG_DATUM_TAG_TIME_CHANGE:
            /* time-change [2] REAL */
            len = bacnet_real_context_decode(
                &apdu[apdu_len], apdu_size - apdu_len, tag.number, &real_value);
            if (len > 0) {
                if (value) {
                    value->datum.time_change = real_value;
                }
                apdu_len += len;
            } else {
                return BACNET_STATUS_ERROR;
            }
            break;
        default:
            return BACNET_STATUS_ERROR;
    }
    /* log-datum [1] CHOICE */
    if (bacnet_is_closing_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 1, &len)) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/**
 * @brief Compare two BACnetActionPropertyValue complex datatypes
 * @param value1 [in] The first structure to compare
 * @param value2 [in] The second structure to compare
 * @return true if the two structures are the same
 */
bool bacnet_audit_log_record_same(
    const BACNET_AUDIT_LOG_RECORD *value1,
    const BACNET_AUDIT_LOG_RECORD *value2)
{
    bool status = false; /*return value */

    if ((value1 == NULL) || (value2 == NULL)) {
        return false;
    }
    /* does the tag match? */
    if (value1->tag == value2->tag) {
        status = true;
    }
    if (status) {
        status = false;
        /* does the timestamp match? */
        if (datetime_compare(&value1->time_stamp, &value2->time_stamp) == 0) {
            status = true;
        }
    }
    if (status) {
        status = false;
        switch (value1->tag) {
            case AUDIT_LOG_DATUM_TAG_STATUS:
                if (value1->datum.log_status == value2->datum.log_status) {
                    status = true;
                }
                break;
            case AUDIT_LOG_DATUM_TAG_NOTIFICATION:
                if (bacnet_audit_log_notification_same(
                        &value1->datum.notification,
                        &value2->datum.notification)) {
                    status = true;
                }
                break;
            case AUDIT_LOG_DATUM_TAG_TIME_CHANGE:
                if (!islessgreater(
                        value1->datum.time_change, value2->datum.time_change)) {
                    status = true;
                }
                break;
            default:
                status = false;
                break;
        }
    }

    return status;
}
