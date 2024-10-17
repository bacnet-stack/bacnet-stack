/**
 * @file
 * @brief WriteGroup service encode and decode
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date August 2023
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacerror.h"
#include "bacnet/write_group.h"

/**
 * @brief Encode the WriteGroup service request
 *
 * WriteGroup-Request ::= SEQUENCE {
 *   group-number [0] Unsigned32,
 *   write-priority [1] Unsigned (1..16),
 *   change-list [2] SEQUENCE OF BACnetGroupChannelValue,
 *   inhibit-delay [3] BOOLEAN OPTIONAL
 * }
 *
 * @param apdu  Pointer to the buffer for encoded values
 * @param data  Pointer to the service data used for encoding values
 *
 * @return Bytes encoded or zero on error.
 */
int bacnet_write_group_encode(
    uint8_t *apdu, const BACNET_WRITE_GROUP_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */
    BACNET_GROUP_CHANNEL_VALUE *value = NULL; /* value in list */
    BACNET_UNSIGNED_INTEGER unsigned_value;

    if (!data) {
        return 0;
    }
    /* group-number [0] Unsigned32 */
    len = encode_context_unsigned(apdu, 0, data->group_number);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* write-priority [1] Unsigned (1..16) */
    unsigned_value = data->write_priority;
    len = encode_context_unsigned(apdu, 1, unsigned_value);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    if (data->change_list) {
        /* change-list [2] SEQUENCE OF BACnetGroupChannelValue */
        len = encode_opening_tag(apdu, 2);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /* the first value includes a pointer to the next value, etc */
        value = data->change_list;
        while (value != NULL) {
            /* SEQUENCE OF BACnetGroupChannelValue */
            len = bacnet_group_channel_value_encode(apdu, value);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            /* is there another one to encode? */
            value = value->next;
        }
        len = encode_closing_tag(apdu, 2);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
    }
    /* inhibit-delay [3] BOOLEAN OPTIONAL */
    if (data->inhibit_delay == WRITE_GROUP_INHIBIT_DELAY_TRUE) {
        len = encode_context_boolean(apdu, 3, true);
        apdu_len += len;
    } else if (data->inhibit_delay == WRITE_GROUP_INHIBIT_DELAY_FALSE) {
        len = encode_context_boolean(apdu, 3, false);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Encode the WriteGroup service request
 * @param apdu  Pointer to the buffer for encoding into
 * @param apdu_size number of bytes available in the buffer
 * @param data  Pointer to the service data used for encoding values
 * @return number of bytes encoded, or zero if unable to encode or too large
 */
size_t bacnet_write_group_service_request_encode(
    uint8_t *apdu, size_t apdu_size, const BACNET_WRITE_GROUP_DATA *data)
{
    size_t apdu_len = 0; /* total length of the apdu, return value */

    apdu_len = bacnet_write_group_encode(NULL, data);
    if (apdu_len > apdu_size) {
        apdu_len = 0;
    } else {
        apdu_len = bacnet_write_group_encode(apdu, data);
    }

    return apdu_len;
}

/**
 * @brief Decode the WriteGroup service request
 *
 * WriteGroup-Request ::= SEQUENCE {
 *   group-number [0] Unsigned32,
 *   write-priority [1] Unsigned (1..16),
 *   change-list [2] SEQUENCE OF BACnetGroupChannelValue,
 *   inhibit-delay [3] BOOLEAN OPTIONAL
 * }
 *
 * @param apdu  Pointer to the buffer for decoding.
 * @param apdu_size  Count of valid bytes in the buffer.
 * @param data  Pointer to the property decoded data to be stored
 *
 * @return Bytes decoded or BACNET_STATUS_ERROR on error.
 */
int bacnet_write_group_service_request_decode(
    const uint8_t *apdu, size_t apdu_size, BACNET_WRITE_GROUP_DATA *data)
{
    int len = 0;
    int apdu_len = 0;
    bool closed = false;
    BACNET_GROUP_CHANNEL_VALUE *value = NULL; /* value in list */
    BACNET_UNSIGNED_INTEGER unsigned_value;
    bool boolean_value = false;

    /* group-number [0] Unsigned32 */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 0, &unsigned_value);
    if (len > 0) {
        /* This parameter is an unsigned integer in the
           range 1 – 4294967295 that represents the control
           group to be affected by this request.
           Control group zero shall never be used
           and shall be reserved. WriteGroup service
           requests containing a zero value for
           'Group Number' shall be ignored.*/
        if ((unsigned_value > 4294967295) || (unsigned_value < 1)) {
            return BACNET_STATUS_ERROR;
        }
        if (data) {
            data->group_number = (uint32_t)unsigned_value;
        }
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* write-priority [1] Unsigned (1..16) */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 1, &unsigned_value);
    if (len > 0) {
        /* This parameter is an unsigned integer in the range 1..16
           that represents the priority for writing that shall apply
           to any channel value changes that result in writes to properties
           of BACnet objects. */
        if ((unsigned_value > BACNET_MAX_PRIORITY) ||
            (unsigned_value < BACNET_MIN_PRIORITY)) {
            return BACNET_STATUS_ERROR;
        }
        if (data) {
            data->write_priority = (uint8_t)unsigned_value;
        }
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* change-list [2] SEQUENCE OF BACnetGroupChannelValue */
    if (bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 2, &len)) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    if (data) {
        value = data->change_list;
    }
    while (apdu_len < apdu_size) {
        if (bacnet_is_closing_tag_number(
                &apdu[apdu_len], apdu_size - apdu_len, 2, &len)) {
            /*  end of change-list [2] SEQUENCE OF BACnetGroupChannelValue */
            apdu_len += len;
            closed = true;
            break;
        }
        len = bacnet_group_channel_value_decode(
            &apdu[apdu_len], apdu_size - apdu_len, value);
        if (len <= 0) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
        if (value) {
            value = value->next;
        }
    }
    if (!closed) {
        return BACNET_STATUS_ERROR;
    }
    if (apdu_len < apdu_size) {
        /* inhibit-delay [3] BOOLEAN OPTIONAL */
        len = bacnet_boolean_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len, 3, &boolean_value);
        if (len > 0) {
            if (data) {
                if (boolean_value) {
                    data->inhibit_delay = WRITE_GROUP_INHIBIT_DELAY_TRUE;
                } else {
                    data->inhibit_delay = WRITE_GROUP_INHIBIT_DELAY_FALSE;
                }
            }
            apdu_len += len;
        } else {
            return BACNET_STATUS_ERROR;
        }
    } else {
        if (data) {
            data->inhibit_delay = WRITE_GROUP_INHIBIT_DELAY_NONE;
        }
    }

    return apdu_len;
}

/**
 * @brief Copy WriteGroup data to another WriteGroup data
 * @param dest  Pointer to the destination data
 * @param src  Pointer to the source data
 * @return true if the values are copied
 */
bool bacnet_write_group_copy(
    BACNET_WRITE_GROUP_DATA *dest, const BACNET_WRITE_GROUP_DATA *src)
{
    const BACNET_GROUP_CHANNEL_VALUE *value_src;
    BACNET_GROUP_CHANNEL_VALUE *value_dest;

    if (!dest || !src) {
        return false;
    }
    dest->group_number = src->group_number;
    dest->write_priority = src->write_priority;
    dest->inhibit_delay = src->inhibit_delay;
    if (src->change_list) {
        value_src = src->change_list;
        value_dest = dest->change_list;
        while (value_src && value_dest) {
            bacnet_group_channel_value_copy(value_dest, value_src);
            value_src = value_src->next;
            value_dest = value_dest->next;
        }
        if (value_src || value_dest) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Compare two WriteGroup service requests
 * @param data1  Pointer to the first data to compare
 * @param data2  Pointer to the second data to compare
 * @return true if the values are the same, else false
 */
bool bacnet_write_group_same(
    const BACNET_WRITE_GROUP_DATA *data1, const BACNET_WRITE_GROUP_DATA *data2)
{
    if (!data1 || !data2) {
        return false;
    }
    if (data1->group_number != data2->group_number) {
        return false;
    }
    if (data1->write_priority != data2->write_priority) {
        return false;
    }
    if (data1->inhibit_delay != data2->inhibit_delay) {
        return false;
    }
    if (data1->change_list == data2->change_list) {
        return true;
    }

    return bacnet_group_change_list_same(
        data1->change_list, data2->change_list);
}

/**
 * @brief Compare two BACnetGroupChannelValue value lists
 */
bool bacnet_group_change_list_same(
    const BACNET_GROUP_CHANNEL_VALUE *head1,
    const BACNET_GROUP_CHANNEL_VALUE *head2)
{
    const BACNET_GROUP_CHANNEL_VALUE *data1;
    const BACNET_GROUP_CHANNEL_VALUE *data2;

    data1 = head1;
    data2 = head2;
    while (data1 && data2) {
        if (!bacnet_group_channel_value_same(data1, data2)) {
            return false;
        }
        data1 = data1->next;
        data2 = data2->next;
    }
    if (data1 || data2) {
        return false;
    }

    return true;
}

/**
 * @brief Compare two BACnetGroupChannelValue values
 * @param value1  Pointer to the first value to compare
 * @param value2  Pointer to the second value to compare
 * @return true if the values are the same, else false
 */
bool bacnet_group_channel_value_same(
    const BACNET_GROUP_CHANNEL_VALUE *value1,
    const BACNET_GROUP_CHANNEL_VALUE *value2)
{
    if (!value1 || !value2) {
        return false;
    }
    if (value1->channel != value2->channel) {
        return false;
    }
    if (value1->overriding_priority != value2->overriding_priority) {
        return false;
    }
    if (!bacnet_channel_value_same(&value1->value, &value2->value)) {
        return false;
    }

    return true;
}

/**
 * @brief Encode a list of BACnetGroupChannelValue values
 *
 * BACnetGroupChannelValue ::= SEQUENCE {
 *   channel [0] Unsigned16,
 *   overriding-priority [1] Unsigned (1..16) OPTIONAL,
 *   value BACnetChannelValue
 * }
 *
 * @param apdu  Pointer to the buffer for encoded values
 * @param head  Pointer to the first value in the list
 * @return Bytes encoded or zero on error.
 */
int bacnet_group_channel_value_encode(
    uint8_t *apdu, const BACNET_GROUP_CHANNEL_VALUE *head)
{
    const BACNET_GROUP_CHANNEL_VALUE *value;
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    value = head;
    while (value) {
        /* channel [0] Unsigned16 */
        len = encode_context_unsigned(apdu, 0, value->channel);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /* overriding-priority [1] Unsigned (1..16) OPTIONAL */
        if ((value->overriding_priority >= BACNET_MIN_PRIORITY) &&
            (value->overriding_priority <= BACNET_MAX_PRIORITY)) {
            len = encode_context_unsigned(apdu, 1, value->overriding_priority);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
        }
        /* value BACnetChannelValue */
        len = bacnet_channel_value_type_encode(apdu, &value->value);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /* is there another one to encode? */
        value = value->next;
    }

    return apdu_len;
}

/**
 * @brief Decode a list of BACnetGroupChannelValue values
 *
 * BACnetGroupChannelValue ::= SEQUENCE {
 *   channel [0] Unsigned16,
 *   overriding-priority [1] Unsigned (1..16) OPTIONAL,
 *   value BACnetChannelValue
 * }
 *
 * @param apdu  Pointer to the buffer for encoded values
 * @param apdu_size  Count of valid bytes in the buffer
 * @param value  Pointer to the first value in the list
 * @return Bytes decoded or BACNET_STATUS_ERROR on error.
 */
int bacnet_group_channel_value_decode(
    const uint8_t *apdu, size_t apdu_size, BACNET_GROUP_CHANNEL_VALUE *value)
{
    int len = 0;
    int apdu_len = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value;
    BACNET_CHANNEL_VALUE channel_value;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    /* channel [0] Unsigned16 */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 0, &unsigned_value);
    if (len > 0) {
        if (unsigned_value > UINT16_MAX) {
            return BACNET_STATUS_ERROR;
        }
        if (value) {
            value->channel = (uint16_t)unsigned_value;
        }
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* overriding-priority [1] Unsigned (1..16) OPTIONAL */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 1, &unsigned_value);
    if (len > 0) {
        if ((unsigned_value >= BACNET_MIN_PRIORITY) &&
            (unsigned_value <= BACNET_MAX_PRIORITY)) {
            if (value) {
                value->overriding_priority = (uint8_t)unsigned_value;
            }
            apdu_len += len;
        } else {
            return BACNET_STATUS_ERROR;
        }
    } else {
        if (value) {
            value->overriding_priority = BACNET_NO_PRIORITY;
        }
    }
    /* value BACnetChannelValue */
    len = bacnet_channel_value_decode(
        &apdu[apdu_len], apdu_size - apdu_len, &channel_value);
    if (len > 0) {
        if (value) {
            memcpy(&value->value, &channel_value, sizeof(BACNET_CHANNEL_VALUE));
        }
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/**
 * @brief Copy BACnetGroupChannelValue data to another BACnetGroupChannelValue
 * data
 * @param dest  Pointer to the destination data
 * @param src  Pointer to the source data
 * @return true if values are able to be copied
 */
bool bacnet_group_channel_value_copy(
    BACNET_GROUP_CHANNEL_VALUE *dest, const BACNET_GROUP_CHANNEL_VALUE *src)
{
    if (!dest || !src) {
        return false;
    }
    dest->channel = src->channel;
    dest->overriding_priority = src->overriding_priority;

    return bacnet_channel_value_copy(&dest->value, &src->value);
}

/**
 * @brief Convert an array of BACnetGroupChannelValue to linked list
 * @param array pointer to element zero of the array
 * @param size number of elements in the array
 */
void bacnet_group_channel_value_link_array(
    BACNET_GROUP_CHANNEL_VALUE *array, size_t size)
{
    size_t i = 0;

    for (i = 0; i < size; i++) {
        if (i > 0) {
            array[i - 1].next = &array[i];
        }
        array[i].next = NULL;
    }
}
