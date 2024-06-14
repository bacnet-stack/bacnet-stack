/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date July 2022
 * @brief API for a basic Color Temperature object
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_COLOR_TEMPERATURE_H
#define BACNET_BASIC_OBJECT_COLOR_TEMPERATURE_H
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacerror.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"

/**
 * @brief Callback for write present value request
 * @param  object_instance - object-instance number of the object
 * @param  old_value - 32-bit value prior to write
 * @param  value - 32-bit value of the write
 */
typedef void (*color_temperature_write_present_value_callback)(
    uint32_t object_instance, uint32_t old_value, uint32_t value);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void Color_Temperature_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary);
BACNET_STACK_EXPORT
bool Color_Temperature_Valid_Instance(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned Color_Temperature_Count(void);
BACNET_STACK_EXPORT
uint32_t Color_Temperature_Index_To_Instance(unsigned index);
BACNET_STACK_EXPORT
unsigned Color_Temperature_Instance_To_Index(uint32_t object_instance);

BACNET_STACK_EXPORT
bool Color_Temperature_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);
BACNET_STACK_EXPORT
bool Color_Temperature_Name_Set(uint32_t object_instance, char *new_name);

BACNET_STACK_EXPORT
int Color_Temperature_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);

BACNET_STACK_EXPORT
bool Color_Temperature_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);

BACNET_STACK_EXPORT
bool Color_Temperature_Present_Value_Set(
    uint32_t object_instance, uint32_t value);
BACNET_STACK_EXPORT
uint32_t Color_Temperature_Present_Value(uint32_t object_instance);
BACNET_STACK_EXPORT
void Color_Temperature_Write_Present_Value_Callback_Set(
    color_temperature_write_present_value_callback cb);

BACNET_STACK_EXPORT
bool Color_Temperature_Tracking_Value_Set(
    uint32_t object_instance, uint32_t value);
BACNET_STACK_EXPORT
uint32_t Color_Temperature_Tracking_Value(uint32_t object_instance);

BACNET_STACK_EXPORT
uint32_t Color_Temperature_Min_Pres_Value(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Color_Temperature_Min_Pres_Value_Set(
    uint32_t object_instance, uint32_t value);

BACNET_STACK_EXPORT
uint32_t Color_Temperature_Max_Pres_Value(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Color_Temperature_Max_Pres_Value_Set(
    uint32_t object_instance, uint32_t value);

BACNET_STACK_EXPORT
bool Color_Temperature_Command(
    uint32_t object_instance, BACNET_COLOR_COMMAND *value);
BACNET_STACK_EXPORT
bool Color_Temperature_Command_Set(
    uint32_t object_instance, BACNET_COLOR_COMMAND *value);

BACNET_STACK_EXPORT
bool Color_Temperature_Default_Color_Temperature_Set(
    uint32_t object_instance, uint32_t value);
BACNET_STACK_EXPORT
uint32_t Color_Temperature_Default_Color_Temperature(uint32_t object_instance);

BACNET_STACK_EXPORT
uint32_t Color_Temperature_Default_Fade_Time(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Color_Temperature_Default_Fade_Time_Set(
    uint32_t object_instance, uint32_t value);

BACNET_STACK_EXPORT
uint32_t Color_Temperature_Default_Ramp_Rate(
    uint32_t object_instance);
BACNET_STACK_EXPORT
bool Color_Temperature_Default_Ramp_Rate_Set(
    uint32_t object_instance, uint32_t value);

BACNET_STACK_EXPORT
uint32_t Color_Temperature_Default_Step_Increment(
    uint32_t object_instance);
BACNET_STACK_EXPORT
bool Color_Temperature_Default_Step_Increment_Set(
    uint32_t object_instance, uint32_t value);

BACNET_STACK_EXPORT
BACNET_COLOR_OPERATION_IN_PROGRESS Color_Temperature_In_Progress(
    uint32_t object_instance);
BACNET_STACK_EXPORT
bool Color_Temperature_In_Progress_Set(
    uint32_t object_instance, BACNET_COLOR_OPERATION_IN_PROGRESS value);

BACNET_STACK_EXPORT
BACNET_COLOR_TRANSITION Color_Temperature_Transition(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Color_Temperature_Transition_Set(
    uint32_t object_instance, BACNET_COLOR_TRANSITION value);

BACNET_STACK_EXPORT
char *Color_Temperature_Description(uint32_t instance);
BACNET_STACK_EXPORT
bool Color_Temperature_Description_Set(uint32_t instance, char *new_name);

BACNET_STACK_EXPORT
bool Color_Temperature_Write_Enabled(uint32_t instance);
BACNET_STACK_EXPORT
void Color_Temperature_Write_Enable(uint32_t instance);
BACNET_STACK_EXPORT
void Color_Temperature_Write_Disable(uint32_t instance);

BACNET_STACK_EXPORT
void Color_Temperature_Timer(uint32_t object_instance, uint16_t milliseconds);

BACNET_STACK_EXPORT
uint32_t Color_Temperature_Create(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Color_Temperature_Delete(uint32_t object_instance);
BACNET_STACK_EXPORT
void Color_Temperature_Cleanup(void);
BACNET_STACK_EXPORT
void Color_Temperature_Init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
