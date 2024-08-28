/**
 * @file
 * @author Mikhail Antropov <michail.antropov@dsr-corporation.com>
 * @date June 2023
 * @brief API for a Time Value object used by a BACnet device object
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_TIME_VALUE_OBJECT_H
#define BACNET_BASIC_TIME_VALUE_OBJECT_H

#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacerror.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"

/**
 * @brief Callback for gateway write present value request
 * @param  object_instance - object-instance number of the object
 * @param  old_value - BACNET_TIME value prior to write
 * @param  value - BACNET_TIME value of the write
 */
typedef void (*time_value_write_present_value_callback)(uint32_t object_instance,
    BACNET_TIME *old_value,
    BACNET_TIME *value);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void Time_Value_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary);
BACNET_STACK_EXPORT
bool Time_Value_Valid_Instance(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned Time_Value_Count(void);
BACNET_STACK_EXPORT
uint32_t Time_Value_Index_To_Instance(unsigned index);
BACNET_STACK_EXPORT
unsigned Time_Value_Instance_To_Index(uint32_t object_instance);

BACNET_STACK_EXPORT
bool Time_Value_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);
BACNET_STACK_EXPORT
bool Time_Value_Name_Set(uint32_t object_instance, char *new_name);
BACNET_STACK_EXPORT
const char *Time_Value_Name_ASCII(uint32_t object_instance);

BACNET_STACK_EXPORT
int Time_Value_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);

BACNET_STACK_EXPORT
bool Time_Value_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);

BACNET_STACK_EXPORT
bool Time_Value_Present_Value_Set(uint32_t object_instance, BACNET_TIME *value);
BACNET_STACK_EXPORT
bool Time_Value_Present_Value(uint32_t object_instance, BACNET_TIME *value);
BACNET_STACK_EXPORT
void Time_Value_Write_Present_Value_Callback_Set(
    time_value_write_present_value_callback cb);

BACNET_STACK_EXPORT
uint8_t Time_Value_Status_Flags(uint32_t object_instance);

BACNET_STACK_EXPORT
bool Time_Value_Out_Of_Service(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Time_Value_Out_Of_Service_Set(uint32_t object_instance, bool oos_flag);

BACNET_STACK_EXPORT
char *Time_Value_Description(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Time_Value_Description_Set(uint32_t object_instance, char *new_name);

BACNET_STACK_EXPORT
bool Time_Value_Write_Enabled(uint32_t instance);
BACNET_STACK_EXPORT
void Time_Value_Write_Enable(uint32_t instance);
BACNET_STACK_EXPORT
void Time_Value_Write_Disable(uint32_t instance);

BACNET_STACK_EXPORT
bool Time_Value_Encode_Value_List(
    uint32_t object_instance,
    BACNET_PROPERTY_VALUE * value_list);
BACNET_STACK_EXPORT
bool Time_Value_Change_Of_Value(
    uint32_t instance);
BACNET_STACK_EXPORT
void Time_Value_Change_Of_Value_Clear(
    uint32_t instance);

BACNET_STACK_EXPORT
uint32_t Time_Value_Create(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Time_Value_Delete(uint32_t object_instance);

BACNET_STACK_EXPORT
void Time_Value_Cleanup(void);
BACNET_STACK_EXPORT
void Time_Value_Init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
