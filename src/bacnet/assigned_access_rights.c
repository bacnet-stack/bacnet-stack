/**
 * @file
 * @brief BACnetAssignedAccessRights structure and codecs
 * @author Nikola Jelic <nikola.jelic@euroicc.com>
 * @date 2015
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include "bacnet/assigned_access_rights.h"
#include "bacnet/bacdcode.h"

/**
 * @brief Encode a BACnetAssignedAccessRights structure into an APDU buffer.
 *
 * BACnetAssignedAccessRights ::= SEQUENCE {
 *   assigned-access-rights [0]BACnetDeviceObjectReference,
 *   enable [1] Boolean
 * }
 *
 * @param apdu [in] The APDU buffer, or NULL for length
 * @param aar [in] The BACnetAssignedAccessRights structure to encode
 * @return number of bytes encoded, or negative on error
 */
int bacapp_encode_assigned_access_rights(
    uint8_t *apdu, const BACNET_ASSIGNED_ACCESS_RIGHTS *aar)
{
    int len;
    int apdu_len = 0;

    len = bacapp_encode_context_device_obj_ref(
        &apdu[apdu_len], 0, &aar->assigned_access_rights);
    if (len < 0) {
        return -1;
    } else {
        apdu_len += len;
    }

    len = encode_context_boolean(&apdu[apdu_len], 1, aar->enable);
    if (len < 0) {
        return -1;
    } else {
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Encode a BACnetAssignedAccessRights structure into an APDU buffer,
 * with context tag.
 *
 * @param apdu [in] The APDU buffer, or NULL for length
 * @param tag [in] The context tag number to use for the opening and closing
 * tags
 * @param aar [in] The BACnetAssignedAccessRights structure to encode
 * @return number of bytes encoded, or negative on error
 */
int bacapp_encode_context_assigned_access_rights(
    uint8_t *apdu, uint8_t tag, const BACNET_ASSIGNED_ACCESS_RIGHTS *aar)
{
    int len;
    int apdu_len = 0;

    len = encode_opening_tag(&apdu[apdu_len], tag);
    apdu_len += len;

    len = bacapp_encode_assigned_access_rights(&apdu[apdu_len], aar);
    apdu_len += len;

    len = encode_closing_tag(&apdu[apdu_len], tag);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Decode a BACnetAssignedAccessRights structure from an APDU buffer.
 *
 * @param apdu [in] The APDU buffer to decode
 * @param apdu_size [in] The size of the APDU buffer
 * @param aar [out] The BACnetAssignedAccessRights structure to fill with
 * decoded values
 * @return number of bytes decoded, or negative on error
 */
int bacapp_decode_assigned_access_rights(
    const uint8_t *apdu, size_t apdu_size, BACNET_ASSIGNED_ACCESS_RIGHTS *data)
{
    int len = 0, apdu_len = 0;
    bool enable = false;
    BACNET_DEVICE_OBJECT_REFERENCE *reference = NULL;

    if (data) {
        reference = &data->assigned_access_rights;
    }
    len = bacnet_device_object_reference_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 0, reference);
    if (len < 0) {
        return BACNET_STATUS_ERROR;
    } else {
        apdu_len += len;
        /* reference already decoded in place */
    }
    len = bacnet_boolean_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 1, &enable);
    if (len < 0) {
        return BACNET_STATUS_ERROR;
    } else {
        apdu_len += len;
        if (data) {
            data->enable = enable;
        }
    }

    return apdu_len;
}

/**
 * @brief Decode a BACnetAssignedAccessRights structure from an APDU buffer,
 * with context tag.
 *
 * @param apdu [in] The APDU buffer to decode
 * @param apdu_size [in] The size of the APDU buffer
 * @param tag [in] The context tag number to use for the opening and closing
 * tags
 * @param aar [out] The BACnetAssignedAccessRights structure to fill with
 * decoded values
 * @return number of bytes decoded, or negative on error
 */
int bacapp_decode_context_assigned_access_rights(
    const uint8_t *apdu,
    size_t apdu_size,
    uint8_t tag,
    BACNET_ASSIGNED_ACCESS_RIGHTS *data)
{
    int len = 0, apdu_len = 0;

    if (bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag, &len)) {
        apdu_len += len;
        len = bacapp_decode_assigned_access_rights(
            &apdu[apdu_len], apdu_size - apdu_len, data);
        if (len < 0) {
            apdu_len = len;
        } else {
            apdu_len += len;
            if (bacnet_is_closing_tag_number(
                    &apdu[apdu_len], apdu_size - apdu_len, tag, &len)) {
                apdu_len += len;
            } else {
                apdu_len = BACNET_STATUS_ERROR;
            }
        }
    } else {
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}
