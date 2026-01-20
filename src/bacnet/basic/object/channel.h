/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2013
 * @brief The Channel object is a command object without a priority array,
 * and the present-value property proxies an ANY data type (sort of)
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_CHANNEL_H
#define BACNET_BASIC_OBJECT_CHANNEL_H
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/rp.h"
#include "bacnet/wp.h"
#include "bacnet/write_group.h"
#include "bacnet/basic/object/lo.h"
#include "bacnet/channel_value.h"

/**
 * @brief Callback for tracking the channel writes for logging or other purposes
 * @param  instance - channel object instance number
 * @param  status - true if write was successful
 * @param  wp_data - pointer to the write property data structure
 */
typedef void (*channel_write_property_callback)(
    uint32_t instance, bool status, BACNET_WRITE_PROPERTY_DATA *wp_data);
/* linked list structure for notifications */
struct channel_write_property_notification;
struct channel_write_property_notification {
    struct channel_write_property_notification *next;
    channel_write_property_callback callback;
};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
BACNET_STACK_EXPORT
void Channel_Property_Lists(
    const int32_t **pRequired,
    const int32_t **pOptional,
    const int32_t **pProprietary);
BACNET_STACK_EXPORT
bool Channel_Valid_Instance(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned Channel_Count(void);
BACNET_STACK_EXPORT
uint32_t Channel_Index_To_Instance(unsigned index);
BACNET_STACK_EXPORT
unsigned Channel_Instance_To_Index(uint32_t instance);
BACNET_STACK_EXPORT
bool Channel_Object_Instance_Add(uint32_t instance);

BACNET_STACK_EXPORT
bool Channel_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);
BACNET_STACK_EXPORT
bool Channel_Name_Set(uint32_t object_instance, const char *new_name);
BACNET_STACK_EXPORT
const char *Channel_Name_ASCII(uint32_t object_instance);

BACNET_STACK_EXPORT
int Channel_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);
BACNET_STACK_EXPORT
bool Channel_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);
BACNET_STACK_EXPORT
void Channel_Write_Group(
    BACNET_WRITE_GROUP_DATA *data,
    uint32_t change_list_index,
    BACNET_GROUP_CHANNEL_VALUE *change_list);

BACNET_STACK_EXPORT
BACNET_CHANNEL_VALUE *Channel_Present_Value(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Channel_Present_Value_Set(
    uint32_t object_instance,
    uint8_t priority,
    const BACNET_CHANNEL_VALUE *value);

BACNET_STACK_EXPORT
bool Channel_Out_Of_Service(uint32_t object_instance);
BACNET_STACK_EXPORT
void Channel_Out_Of_Service_Set(uint32_t object_instance, bool oos_flag);

BACNET_STACK_EXPORT
unsigned Channel_Last_Priority(uint32_t object_instance);
BACNET_STACK_EXPORT
BACNET_WRITE_STATUS Channel_Write_Status(uint32_t object_instance);
uint16_t Channel_Number(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Channel_Number_Set(uint32_t object_instance, uint16_t value);

BACNET_STACK_EXPORT
unsigned Channel_Reference_List_Member_Count(uint32_t object_instance);
BACNET_STACK_EXPORT
BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *Channel_Reference_List_Member_Element(
    uint32_t object_instance, unsigned element);
BACNET_STACK_EXPORT
bool Channel_Reference_List_Member_Element_Set(
    uint32_t object_instance,
    unsigned array_index,
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pMemberSrc);
BACNET_STACK_EXPORT
unsigned Channel_Reference_List_Member_Element_Add(
    uint32_t object_instance,
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pMemberSrc);
BACNET_STACK_EXPORT
uint16_t
Channel_Control_Groups_Element(uint32_t object_instance, int32_t array_index);
BACNET_STACK_EXPORT
bool Channel_Control_Groups_Element_Set(
    uint32_t object_instance, int32_t array_index, uint16_t value);

BACNET_STACK_EXPORT
bool Channel_Write_Member_Value(
    BACNET_WRITE_PROPERTY_DATA *wp_data, const BACNET_CHANNEL_VALUE *value);

BACNET_STACK_EXPORT
void Channel_Write_Property_Internal_Callback_Set(write_property_function cb);
BACNET_STACK_EXPORT
void Channel_Write_Property_Notification_Add(
    struct channel_write_property_notification *notification);
BACNET_STACK_EXPORT
void Channel_Write_Property_Notify(
    uint32_t instance, bool status, BACNET_WRITE_PROPERTY_DATA *wp_data);

BACNET_STACK_EXPORT
void *Channel_Context_Get(uint32_t object_instance);
BACNET_STACK_EXPORT
void Channel_Context_Set(uint32_t object_instance, void *context);

BACNET_STACK_EXPORT
uint32_t Channel_Create(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Channel_Delete(uint32_t object_instance);
BACNET_STACK_EXPORT
void Channel_Cleanup(void);
BACNET_STACK_EXPORT
void Channel_Init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
