/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @brief API for a basic Lighting Output object
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_LIGHTING_OUTPUT_H
#define BACNET_BASIC_OBJECT_LIGHTING_OUTPUT_H
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacerror.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"
#include "bacnet/basic/sys/lighting_command.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void Lighting_Output_Property_Lists(
    const int32_t **pRequired,
    const int32_t **pOptional,
    const int32_t **pProprietary);
BACNET_STACK_EXPORT
bool Lighting_Output_Valid_Instance(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned Lighting_Output_Count(void);
BACNET_STACK_EXPORT
uint32_t Lighting_Output_Index_To_Instance(unsigned index);
BACNET_STACK_EXPORT
unsigned Lighting_Output_Instance_To_Index(uint32_t instance);
BACNET_STACK_EXPORT
bool Lighting_Output_Object_Instance_Add(uint32_t instance);

BACNET_STACK_EXPORT
float Lighting_Output_Present_Value(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned Lighting_Output_Present_Value_Priority(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Lighting_Output_Present_Value_Set(
    uint32_t object_instance, float value, unsigned priority);

BACNET_STACK_EXPORT
bool Lighting_Output_Priority_Array_Relinquished(
    uint32_t object_instance, unsigned priority);
BACNET_STACK_EXPORT
float Lighting_Output_Priority_Array_Value(
    uint32_t object_instance, unsigned priority);

BACNET_STACK_EXPORT
bool Lighting_Output_Present_Value_Relinquish(
    uint32_t object_instance, unsigned priority);
BACNET_STACK_EXPORT
bool Lighting_Output_Present_Value_Relinquish_All(uint32_t object_instance);
BACNET_STACK_EXPORT
float Lighting_Output_Relinquish_Default(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Lighting_Output_Relinquish_Default_Set(
    uint32_t object_instance, float value);

BACNET_STACK_EXPORT
float Lighting_Output_Last_On_Value(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Lighting_Output_Last_On_Value_Set(uint32_t object_instance, float value);
BACNET_STACK_EXPORT
float Lighting_Output_Default_On_Value(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Lighting_Output_Default_On_Value_Set(
    uint32_t object_instance, float value);
BACNET_STACK_EXPORT
float Lighting_Output_High_End_Trim(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Lighting_Output_High_End_Trim_Set(uint32_t object_instance, float value);
BACNET_STACK_EXPORT
float Lighting_Output_Low_End_Trim(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Lighting_Output_Low_End_Trim_Set(uint32_t object_instance, float value);
BACNET_STACK_EXPORT
uint32_t Lighting_Output_Trim_Fade_Time(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Lighting_Output_Trim_Fade_Time_Set(
    uint32_t object_instance, uint32_t value);

BACNET_STACK_EXPORT
bool Lighting_Output_Overridden_Set(uint32_t object_instance, float value);
BACNET_STACK_EXPORT
bool Lighting_Output_Overridden_Clear(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Lighting_Output_Overridden_Status(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Lighting_Output_Overridden_Momentary(
    uint32_t object_instance, float value);

BACNET_STACK_EXPORT
bool Lighting_Output_Change_Of_Value(uint32_t instance);
BACNET_STACK_EXPORT
void Lighting_Output_Change_Of_Value_Clear(uint32_t instance);
BACNET_STACK_EXPORT
bool Lighting_Output_Encode_Value_List(
    uint32_t object_instance, BACNET_PROPERTY_VALUE *value_list);
BACNET_STACK_EXPORT
float Lighting_Output_COV_Increment(uint32_t instance);
BACNET_STACK_EXPORT
void Lighting_Output_COV_Increment_Set(uint32_t instance, float value);

BACNET_STACK_EXPORT
bool Lighting_Output_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);
BACNET_STACK_EXPORT
bool Lighting_Output_Name_Set(uint32_t object_instance, const char *new_name);
BACNET_STACK_EXPORT
const char *Lighting_Output_Name_ASCII(uint32_t object_instance);

BACNET_STACK_EXPORT
const char *Lighting_Output_Description(uint32_t instance);
BACNET_STACK_EXPORT
bool Lighting_Output_Description_Set(uint32_t instance, const char *new_name);

BACNET_STACK_EXPORT
bool Lighting_Output_Out_Of_Service(uint32_t instance);
BACNET_STACK_EXPORT
void Lighting_Output_Out_Of_Service_Set(uint32_t instance, bool oos_flag);

BACNET_STACK_EXPORT
bool Lighting_Output_Lighting_Command_Refresh(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Lighting_Output_Lighting_Command_Set(
    uint32_t object_instance, const BACNET_LIGHTING_COMMAND *value);
BACNET_STACK_EXPORT
bool Lighting_Output_Lighting_Command(
    uint32_t object_instance, BACNET_LIGHTING_COMMAND *value);

BACNET_STACK_EXPORT
BACNET_LIGHTING_IN_PROGRESS
Lighting_Output_In_Progress(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Lighting_Output_In_Progress_Set(
    uint32_t object_instance, BACNET_LIGHTING_IN_PROGRESS in_progress);

BACNET_STACK_EXPORT
float Lighting_Output_Tracking_Value(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Lighting_Output_Tracking_Value_Set(uint32_t object_instance, float value);

BACNET_STACK_EXPORT
bool Lighting_Output_Blink_Warn_Enable(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Lighting_Output_Blink_Warn_Enable_Set(
    uint32_t object_instance, bool enable);
BACNET_STACK_EXPORT
bool Lighting_Output_Blink_Warn_Feature_Set(
    uint32_t object_instance,
    float off_value,
    uint16_t interval,
    uint16_t count);

BACNET_STACK_EXPORT
uint32_t Lighting_Output_Egress_Time(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Lighting_Output_Egress_Time_Set(
    uint32_t object_instance, uint32_t seconds);
BACNET_STACK_EXPORT
bool Lighting_Output_Egress_Active(uint32_t object_instance);

BACNET_STACK_EXPORT
uint32_t Lighting_Output_Default_Fade_Time(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Lighting_Output_Default_Fade_Time_Set(
    uint32_t object_instance, uint32_t milliseconds);

BACNET_STACK_EXPORT
float Lighting_Output_Default_Ramp_Rate(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Lighting_Output_Default_Ramp_Rate_Set(
    uint32_t object_instance, float percent_per_second);

BACNET_STACK_EXPORT
float Lighting_Output_Default_Step_Increment(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Lighting_Output_Default_Step_Increment_Set(
    uint32_t object_instance, float step_increment);

BACNET_STACK_EXPORT
BACNET_LIGHTING_TRANSITION Lighting_Output_Transition(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Lighting_Output_Transition_Set(
    uint32_t object_instance, BACNET_LIGHTING_TRANSITION value);

BACNET_STACK_EXPORT
unsigned Lighting_Output_Default_Priority(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Lighting_Output_Default_Priority_Set(
    uint32_t object_instance, unsigned priority);

BACNET_STACK_EXPORT
bool Lighting_Output_Color_Override(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Lighting_Output_Color_Override_Set(uint32_t object_instance, bool value);

BACNET_STACK_EXPORT
bool Lighting_Output_Color_Reference(
    uint32_t object_instance, BACNET_OBJECT_ID *value);
BACNET_STACK_EXPORT
bool Lighting_Output_Color_Reference_Set(
    uint32_t object_instance, const BACNET_OBJECT_ID *value);

BACNET_STACK_EXPORT
bool Lighting_Output_Override_Color_Reference(
    uint32_t object_instance, BACNET_OBJECT_ID *value);
BACNET_STACK_EXPORT
bool Lighting_Output_Override_Color_Reference_Set(
    uint32_t object_instance, const BACNET_OBJECT_ID *value);

BACNET_STACK_EXPORT
float Lighting_Output_Feedback_Value(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Lighting_Output_Feedback_Value_Set(uint32_t object_instance, float value);

BACNET_STACK_EXPORT
float Lighting_Output_Power(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Lighting_Output_Power_Set(uint32_t object_instance, float value);

BACNET_STACK_EXPORT
float Lighting_Output_Instantaneous_Power(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Lighting_Output_Instantaneous_Power_Set(
    uint32_t object_instance, float value);

BACNET_STACK_EXPORT
void Lighting_Output_Timer(uint32_t object_instance, uint16_t milliseconds);

BACNET_STACK_EXPORT
void Lighting_Output_Write_Present_Value_Callback_Set(
    lighting_command_tracking_value_callback cb);

BACNET_STACK_EXPORT
void *Lighting_Output_Context_Get(uint32_t object_instance);
BACNET_STACK_EXPORT
void Lighting_Output_Context_Set(uint32_t object_instance, void *context);

BACNET_STACK_EXPORT
uint32_t Lighting_Output_Create(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Lighting_Output_Delete(uint32_t object_instance);
BACNET_STACK_EXPORT
void Lighting_Output_Cleanup(void);
BACNET_STACK_EXPORT
void Lighting_Output_Init(void);

BACNET_STACK_EXPORT
int Lighting_Output_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);
BACNET_STACK_EXPORT
bool Lighting_Output_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
