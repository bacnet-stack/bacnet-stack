/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2014
 * @brief API for Integer Value objects
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_INTEGER_VALUE_H
#define BACNET_BASIC_OBJECT_INTEGER_VALUE_H
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacerror.h"
#include "bacnet/wp.h"
#include "bacnet/rp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Callback for gateway write present value request
 * @param  object_instance - object-instance number of the object
 * @param  old_value - integer preset-value prior to write
 * @param  value - integer preset-value of the write
 */
typedef void (*integer_value_write_present_value_callback)(
    uint32_t object_instance,
    int32_t old_value,
    int32_t value);

BACNET_STACK_EXPORT
void Integer_Value_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary);
BACNET_STACK_EXPORT
bool Integer_Value_Valid_Instance(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned Integer_Value_Count(void);
BACNET_STACK_EXPORT
uint32_t Integer_Value_Index_To_Instance(unsigned index);
BACNET_STACK_EXPORT
unsigned Integer_Value_Instance_To_Index(uint32_t object_instance);

BACNET_STACK_EXPORT
bool Integer_Value_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);
BACNET_STACK_EXPORT
bool Integer_Value_Name_Set(uint32_t object_instance, const char *new_name);
BACNET_STACK_EXPORT
const char *Integer_Value_Name_ASCII(uint32_t object_instance);

BACNET_STACK_EXPORT
int Integer_Value_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);

BACNET_STACK_EXPORT
bool Integer_Value_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);

BACNET_STACK_EXPORT
void Integer_Value_Write_Present_Value_Callback_Set(
    integer_value_write_present_value_callback cb);

BACNET_STACK_EXPORT
bool Integer_Value_Present_Value_Set(
    uint32_t object_instance, int32_t value, uint8_t priority);
BACNET_STACK_EXPORT
int32_t Integer_Value_Present_Value(uint32_t object_instance);

BACNET_STACK_EXPORT
bool Integer_Value_Change_Of_Value(uint32_t instance);
BACNET_STACK_EXPORT
void Integer_Value_Change_Of_Value_Clear(uint32_t instance);
BACNET_STACK_EXPORT
bool Integer_Value_Encode_Value_List(
    uint32_t object_instance, BACNET_PROPERTY_VALUE *value_list);
BACNET_STACK_EXPORT
uint32_t Integer_Value_COV_Increment(uint32_t object_instance);
BACNET_STACK_EXPORT
void Integer_Value_COV_Increment_Set(uint32_t object_instance, uint32_t value);

BACNET_STACK_EXPORT
bool Integer_Value_Description(
    uint32_t object_instance, BACNET_CHARACTER_STRING *description);
BACNET_STACK_EXPORT
bool Integer_Value_Description_Set(uint32_t instance, const char *new_name);
BACNET_STACK_EXPORT
const char *Integer_Value_Description_ANSI(uint32_t object_instance);

BACNET_STACK_EXPORT
uint16_t Integer_Value_Units(uint32_t instance);
BACNET_STACK_EXPORT
bool Integer_Value_Units_Set(uint32_t instance, uint16_t unit);

BACNET_STACK_EXPORT
bool Integer_Value_Out_Of_Service(uint32_t instance);
BACNET_STACK_EXPORT
void Integer_Value_Out_Of_Service_Set(uint32_t instance, bool oos_flag);

BACNET_STACK_EXPORT
void Integer_Value_Init(void);

BACNET_STACK_EXPORT
uint32_t Integer_Value_Create(uint32_t object_instance);

BACNET_STACK_EXPORT
bool Integer_Value_Delete(uint32_t object_instance);

BACNET_STACK_EXPORT
void Integer_Value_Cleanup(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
