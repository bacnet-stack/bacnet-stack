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

int bacapp_encode_assigned_access_rights(
    uint8_t *apdu, BACNET_ASSIGNED_ACCESS_RIGHTS *aar)
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

int bacapp_encode_context_assigned_access_rights(
    uint8_t *apdu, uint8_t tag, BACNET_ASSIGNED_ACCESS_RIGHTS *aar)
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

int bacapp_decode_assigned_access_rights(
    uint8_t *apdu, BACNET_ASSIGNED_ACCESS_RIGHTS *aar)
{
    int len;
    int apdu_len = 0;

    if (decode_is_context_tag(&apdu[apdu_len], 0)) {
        len = bacapp_decode_context_device_obj_ref(
            &apdu[apdu_len], 0, &aar->assigned_access_rights);
        if (len < 0) {
            return -1;
        } else {
            apdu_len += len;
        }
    } else {
        return -1;
    }

    if (decode_is_context_tag(&apdu[apdu_len], 1)) {
        len = decode_context_boolean2(&apdu[apdu_len], 1, &aar->enable);
        if (len < 0) {
            return -1;
        } else {
            apdu_len += len;
        }
    } else {
        return -1;
    }

    return apdu_len;
}

int bacapp_decode_context_assigned_access_rights(
    uint8_t *apdu, uint8_t tag, BACNET_ASSIGNED_ACCESS_RIGHTS *aar)
{
    int len = 0;
    int section_length;

    if (decode_is_opening_tag_number(&apdu[len], tag)) {
        len++;
        section_length = bacapp_decode_assigned_access_rights(&apdu[len], aar);

        if (section_length == -1) {
            len = -1;
        } else {
            len += section_length;
            if (decode_is_closing_tag_number(&apdu[len], tag)) {
                len++;
            } else {
                len = -1;
            }
        }
    } else {
        len = -1;
    }
    return len;
}
