/**
 * @file
 * @author Mikhail Antropov <michail.antropov@dsr-corporation.com>
 * @date June 2022
 * @brief API for a Calendar object used by a BACnet device object
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef BACNET_CALENDAR_OBJECT_H
#define BACNET_CALENDAR_OBJECT_H

#include <stdbool.h>
#include <stdint.h>
#include "bacnet/bacnet_stack_exports.h"
#include "bacnet/config.h"
#include "bacnet/bacdef.h"
#include "bacnet/calendar_entry.h"
#include "bacnet/bacenum.h"
#include "bacnet/bacerror.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"

/**
 * @brief Callback for gateway write present value request
 * @param  object_instance - object-instance number of the object
 * @param  old_value - bool value prior to write
 * @param  value - bool value of the write
 */
typedef void (*calendar_write_present_value_callback)(
    uint32_t object_instance, bool old_value, bool value);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void Calendar_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary);
BACNET_STACK_EXPORT
bool Calendar_Valid_Instance(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned Calendar_Count(void);
BACNET_STACK_EXPORT
uint32_t Calendar_Index_To_Instance(unsigned index);
BACNET_STACK_EXPORT
unsigned Calendar_Instance_To_Index(uint32_t object_instance);

BACNET_STACK_EXPORT
bool Calendar_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);
BACNET_STACK_EXPORT
bool Calendar_Name_Set(uint32_t object_instance, char *new_name);

BACNET_STACK_EXPORT
int Calendar_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);

BACNET_STACK_EXPORT
bool Calendar_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);

BACNET_STACK_EXPORT
bool Calendar_Present_Value(uint32_t object_instance);
BACNET_STACK_EXPORT
void Calendar_Write_Present_Value_Callback_Set(
    calendar_write_present_value_callback cb);

BACNET_STACK_EXPORT
BACNET_CALENDAR_ENTRY *Calendar_Date_List_Get(
    uint32_t object_instance, uint8_t index);
BACNET_STACK_EXPORT
bool Calendar_Date_List_Add(
    uint32_t object_instance, BACNET_CALENDAR_ENTRY *value);
BACNET_STACK_EXPORT
bool Calendar_Date_List_Delete_All(uint32_t object_instance);
BACNET_STACK_EXPORT
int Calendar_Date_List_Count(uint32_t object_instance);
BACNET_STACK_EXPORT
int Calendar_Date_List_Encode(
    uint32_t object_instance, uint8_t *apdu, int max_apdu);

BACNET_STACK_EXPORT
char *Calendar_Description(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Calendar_Description_Set(uint32_t object_instance, char *new_name);

BACNET_STACK_EXPORT
bool Calendar_Write_Enabled(uint32_t instance);
BACNET_STACK_EXPORT
void Calendar_Write_Enable(uint32_t instance);
BACNET_STACK_EXPORT
void Calendar_Write_Disable(uint32_t instance);

BACNET_STACK_EXPORT
uint32_t Calendar_Create(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Calendar_Delete(uint32_t object_instance);
BACNET_STACK_EXPORT
void Calendar_Cleanup(void);
BACNET_STACK_EXPORT
void Calendar_Init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
