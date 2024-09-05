/**
 * @file
 * @brief AddListElement and RemoveListElement service initiation
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date January 2023
 * @section LICENSE
 *
 * Copyright (C) 2023 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef SEND_LIST_ELEMENT_H
#define SEND_LIST_ELEMENT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/apdu.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
uint8_t Send_List_Element_Request_Data(
    BACNET_CONFIRMED_SERVICE service,
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    uint8_t *application_data,
    int application_data_len,
    uint32_t array_index);

BACNET_STACK_EXPORT
uint8_t Send_Add_List_Element_Request_Data(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    uint8_t *application_data,
    int application_data_len,
    uint32_t array_index);
BACNET_STACK_EXPORT
uint8_t Send_Add_List_Element_Request(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    BACNET_APPLICATION_DATA_VALUE *object_value,
    uint32_t array_index);

BACNET_STACK_EXPORT
uint8_t Send_Remove_List_Element_Request_Data(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    uint8_t *application_data,
    int application_data_len,
    uint32_t array_index);
BACNET_STACK_EXPORT
uint8_t Send_Remove_List_Element_Request(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    BACNET_APPLICATION_DATA_VALUE *object_value,
    uint32_t array_index);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
