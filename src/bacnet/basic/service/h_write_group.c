/**
 * @file
 * @brief The WriteGroup-Request service handler
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date October 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"
#include "bacnet/abort.h"
#include "bacnet/write_group.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/sys/debug.h"

/* WriteGroup-Request notification callbacks list */
static BACNET_WRITE_GROUP_NOTIFICATION Write_Group_Notification_Head;

/**
 * @brief Print the contents of a WriteGroup-Request
 * @param data [in] The decoded WriteGroup-Request message
 */
void handler_write_group_print_data(BACNET_WRITE_GROUP_DATA *data)
{
    if (!data) {
        return;
    }
    debug_printf(
        "WriteGroup:group-number=%lu\r\n", (unsigned long)data->group_number);
    debug_printf(
        "WriteGroup:write-priority=%lu\r\n",
        (unsigned long)data->write_priority);
}

/**
 * @brief generic callback for WriteGroup-Request iterator
 * @param data [in] The contents of the WriteGroup-Request message
 * @param change_list_index [in] The index of the current value in the change
 * list
 * @param change_list [in] The current value in the change list
 */
static void handler_write_group_notification_callback(
    BACNET_WRITE_GROUP_DATA *data,
    uint32_t change_list_index,
    BACNET_GROUP_CHANNEL_VALUE *change_list)
{
    BACNET_WRITE_GROUP_NOTIFICATION *head;

    handler_write_group_print_data(data);
    head = &Write_Group_Notification_Head;
    do {
        if (head->callback) {
            head->callback(data, change_list_index, change_list);
        }
        head = head->next;
    } while (head);
}

/**
 * @brief Add a WriteGroup notification callback
 * @param cb - WriteGroup notification callback to be added
 */
void handler_write_group_notification_add(BACNET_WRITE_GROUP_NOTIFICATION *cb)
{
    BACNET_WRITE_GROUP_NOTIFICATION *head;

    head = &Write_Group_Notification_Head;
    do {
        if (head->next == cb) {
            /* already here! */
            break;
        } else if (!head->next) {
            /* first available free node */
            head->next = cb;
            break;
        }
        head = head->next;
    } while (head);
}

/**
 * @brief A basic WriteGroup-Request service handler
 * @param service_request [in] The contents of the service request
 * @param service_len [in] The length of the service request
 * @param src [in] The source of the message
 */
void handler_write_group(
    uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src)
{
    BACNET_WRITE_GROUP_DATA data = { 0 };
    int len = 0;

    (void)src;
    debug_printf("Received WriteGroup-Request!\n");

    len = bacnet_write_group_service_request_decode_iterate(
        service_request, service_len, &data,
        handler_write_group_notification_callback);
    if (len <= 0) {
        debug_printf("WriteGroup-Request failed to decode!\n");
    }
}
