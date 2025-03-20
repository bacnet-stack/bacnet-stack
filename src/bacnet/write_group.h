/**
 * @file
 * @brief API for BACnet WriteGroup service encoder and decoder
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_WRITE_GROUP_H
#define BACNET_WRITE_GROUP_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacapp.h"
#include "bacnet/channel_value.h"

/**
 * BACnetGroupChannelValue ::= SEQUENCE {
 *   channel [0] Unsigned16,
 *   overriding-priority [1] Unsigned (1..16) OPTIONAL,
 *   value BACnetChannelValue
 * }
 */
struct BACnet_Group_Channel_Value;
typedef struct BACnet_Group_Channel_Value {
    uint16_t channel;
    uint8_t overriding_priority;
    BACNET_CHANNEL_VALUE value;
    struct BACnet_Group_Channel_Value *next;
} BACNET_GROUP_CHANNEL_VALUE;

typedef enum {
    WRITE_GROUP_INHIBIT_DELAY_NONE = 0,
    WRITE_GROUP_INHIBIT_DELAY_TRUE = 1,
    WRITE_GROUP_INHIBIT_DELAY_FALSE = 2
} WRITE_GROUP_INHIBIT_DELAY;

/**
 * WriteGroup-Request ::= SEQUENCE {
 *   group-number [0] Unsigned32,
 *   write-priority [1] Unsigned (1..16),
 *   change-list [2] SEQUENCE OF BACnetGroupChannelValue,
 *   inhibit-delay [3] BOOLEAN OPTIONAL
 * }
 */
struct BACnet_Write_Group_Data;
typedef struct BACnet_Write_Group_Data {
    uint32_t group_number;
    uint8_t write_priority;
    /* simple linked list of values */
    BACNET_GROUP_CHANNEL_VALUE change_list;
    WRITE_GROUP_INHIBIT_DELAY inhibit_delay;
    struct BACnet_Write_Group_Data *next;
} BACNET_WRITE_GROUP_DATA;

/**
 * @brief generic callback for WriteGroup-Request iterator
 * @param data [in] The contents of the WriteGroup-Request message
 * @param change_list_index [in] The index of the current value in the change
 * list
 * @param change_list [in] The current value in the change list
 */
typedef void (*BACnet_Write_Group_Callback)(
    BACNET_WRITE_GROUP_DATA *data,
    uint32_t change_list_index,
    BACNET_GROUP_CHANNEL_VALUE *change_list);
struct BACnet_Write_Group_Notification;
typedef struct BACnet_Write_Group_Notification {
    struct BACnet_Write_Group_Notification *next;
    BACnet_Write_Group_Callback callback;
} BACNET_WRITE_GROUP_NOTIFICATION;

/**
 * @brief Process a WriteGroup-Request message, one value at a time
 * @param device_id [in] The device ID of the source of the message
 * @param data [in] The contents of the WriteGroup-Request message
 */
typedef void (*write_group_request_process)(
    uint32_t device_id, BACNET_WRITE_GROUP_DATA *data);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int bacnet_write_group_encode(
    uint8_t *apdu, const BACNET_WRITE_GROUP_DATA *data);
BACNET_STACK_EXPORT
bool bacnet_write_group_same(
    const BACNET_WRITE_GROUP_DATA *data1, const BACNET_WRITE_GROUP_DATA *data2);

BACNET_STACK_EXPORT
size_t bacnet_write_group_service_request_encode(
    uint8_t *apdu, size_t apdu_size, const BACNET_WRITE_GROUP_DATA *data);
BACNET_STACK_EXPORT
int bacnet_write_group_service_request_decode(
    const uint8_t *apdu, size_t apdu_size, BACNET_WRITE_GROUP_DATA *data);
BACNET_STACK_EXPORT
int bacnet_write_group_service_request_decode_iterate(
    const uint8_t *apdu,
    size_t apdu_size,
    BACNET_WRITE_GROUP_DATA *data,
    BACnet_Write_Group_Callback callback);

BACNET_STACK_EXPORT
bool bacnet_write_group_copy(
    BACNET_WRITE_GROUP_DATA *dest, const BACNET_WRITE_GROUP_DATA *src);

BACNET_STACK_EXPORT
bool bacnet_group_change_list_same(
    const BACNET_GROUP_CHANNEL_VALUE *head1,
    const BACNET_GROUP_CHANNEL_VALUE *head2);
BACNET_STACK_EXPORT
unsigned bacnet_write_group_change_list_count(BACNET_WRITE_GROUP_DATA *data);
BACNET_STACK_EXPORT
bool bacnet_write_group_change_list_append(
    BACNET_WRITE_GROUP_DATA *data, BACNET_GROUP_CHANNEL_VALUE *element);
BACNET_STACK_EXPORT
bool bacnet_write_group_change_list_array_link(
    BACNET_WRITE_GROUP_DATA *data,
    BACNET_GROUP_CHANNEL_VALUE *array,
    size_t size);
BACNET_STACK_EXPORT
BACNET_GROUP_CHANNEL_VALUE *bacnet_write_group_change_list_element(
    BACNET_WRITE_GROUP_DATA *data, unsigned index);

BACNET_STACK_EXPORT
void bacnet_write_group_channel_value_process(
    uint8_t *apdu,
    size_t apdu_len,
    BACNET_WRITE_GROUP_DATA *data,
    write_group_request_process callback);

BACNET_STACK_EXPORT
int bacnet_group_channel_value_encode(
    uint8_t *apdu, const BACNET_GROUP_CHANNEL_VALUE *value);
BACNET_STACK_EXPORT
int bacnet_group_channel_value_decode(
    const uint8_t *apdu, size_t apdu_size, BACNET_GROUP_CHANNEL_VALUE *value);
BACNET_STACK_EXPORT
bool bacnet_group_channel_value_same(
    const BACNET_GROUP_CHANNEL_VALUE *value1,
    const BACNET_GROUP_CHANNEL_VALUE *value2);
BACNET_STACK_EXPORT
bool bacnet_group_channel_value_copy(
    BACNET_GROUP_CHANNEL_VALUE *dest, const BACNET_GROUP_CHANNEL_VALUE *src);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
