/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2023
 * @brief API for a basic BACnet Binary Lighting Output object implementation.
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_BINARY_LIGHTING_OUTPUT_H
#define BACNET_BASIC_OBJECT_BINARY_LIGHTING_OUTPUT_H
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacerror.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"

/**
 * @brief Callback for write value request
 * @param  object_instance - object-instance number of the object
 * @param  old_value - value prior to write
 * @param  value - value of the write
 */
typedef void (*binary_lighting_output_write_value_callback)(
    uint32_t object_instance,
    BACNET_BINARY_LIGHTING_PV old_value,
    BACNET_BINARY_LIGHTING_PV value);

/**
 * @brief Callback for blink warning notification
 * @param  object_instance - object-instance number of the object
 */
typedef void (*binary_lighting_output_blink_warn_callback)(
    uint32_t object_instance);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void Binary_Lighting_Output_Property_Lists(
    const int32_t **pRequired,
    const int32_t **pOptional,
    const int32_t **pProprietary);
BACNET_STACK_EXPORT
void Binary_Lighting_Output_Writable_Property_List(
    uint32_t object_instance, const int32_t **properties);

BACNET_STACK_EXPORT
bool Binary_Lighting_Output_Valid_Instance(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned Binary_Lighting_Output_Count(void);
BACNET_STACK_EXPORT
uint32_t Binary_Lighting_Output_Index_To_Instance(unsigned index);
BACNET_STACK_EXPORT
unsigned Binary_Lighting_Output_Instance_To_Index(uint32_t instance);
BACNET_STACK_EXPORT
bool Binary_Lighting_Output_Object_Instance_Add(uint32_t instance);

BACNET_STACK_EXPORT
BACNET_BINARY_LIGHTING_PV
Binary_Lighting_Output_Present_Value(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned
Binary_Lighting_Output_Present_Value_Priority(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Binary_Lighting_Output_Present_Value_Set(
    uint32_t object_instance,
    BACNET_BINARY_LIGHTING_PV value,
    unsigned priority);

BACNET_STACK_EXPORT
bool Binary_Lighting_Output_Priority_Array_Relinquished(
    uint32_t object_instance, unsigned priority);
BACNET_STACK_EXPORT
BACNET_BINARY_LIGHTING_PV Binary_Lighting_Output_Priority_Array_Value(
    uint32_t object_instance, unsigned priority);

BACNET_STACK_EXPORT
bool Binary_Lighting_Output_Present_Value_Relinquish(
    uint32_t object_instance, unsigned priority);

BACNET_STACK_EXPORT
BACNET_BINARY_LIGHTING_PV
Binary_Lighting_Output_Relinquish_Default(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Binary_Lighting_Output_Relinquish_Default_Set(
    uint32_t object_instance, BACNET_BINARY_LIGHTING_PV value);

BACNET_STACK_EXPORT
BACNET_RELIABILITY Binary_Lighting_Output_Reliability(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Binary_Lighting_Output_Reliability_Set(
    uint32_t object_instance, BACNET_RELIABILITY value);

BACNET_STACK_EXPORT
bool Binary_Lighting_Output_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);
BACNET_STACK_EXPORT
bool Binary_Lighting_Output_Name_Set(
    uint32_t object_instance, const char *new_name);
BACNET_STACK_EXPORT
const char *Binary_Lighting_Output_Name_ASCII(uint32_t object_instance);

BACNET_STACK_EXPORT
const char *Binary_Lighting_Output_Description(uint32_t instance);
BACNET_STACK_EXPORT
bool Binary_Lighting_Output_Description_Set(
    uint32_t instance, const char *new_name);

BACNET_STACK_EXPORT
bool Binary_Lighting_Output_Out_Of_Service(uint32_t instance);
BACNET_STACK_EXPORT
void Binary_Lighting_Output_Out_Of_Service_Set(
    uint32_t instance, bool oos_flag);

BACNET_STACK_EXPORT
BACNET_BINARY_LIGHTING_PV
Binary_Lighting_Output_Lighting_Command_Target_Value(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned Binary_Lighting_Output_Lighting_Command_Target_Priority(
    uint32_t object_instance);
BACNET_STACK_EXPORT
bool Binary_Lighting_Output_Lighting_Command_Set(
    uint32_t object_instance,
    BACNET_BINARY_LIGHTING_PV value,
    unsigned priority);

BACNET_STACK_EXPORT
BACNET_BINARY_LIGHTING_PV
Binary_Lighting_Output_Feedback_Value(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Binary_Lighting_Output_Feedback_Value_Set(
    uint32_t object_instance, BACNET_BINARY_LIGHTING_PV value);

BACNET_STACK_EXPORT
bool Binary_Lighting_Output_Blink_Warn_Enable(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Binary_Lighting_Output_Blink_Warn_Enable_Set(
    uint32_t object_instance, bool enable);

BACNET_STACK_EXPORT
uint32_t Binary_Lighting_Output_Egress_Time(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Binary_Lighting_Output_Egress_Time_Set(
    uint32_t object_instance, uint32_t seconds);
BACNET_STACK_EXPORT
bool Binary_Lighting_Output_Egress_Active(uint32_t object_instance);

BACNET_STACK_EXPORT
void Binary_Lighting_Output_Timer(
    uint32_t object_instance, uint16_t milliseconds);

BACNET_STACK_EXPORT
void Binary_Lighting_Output_Write_Value_Callback_Set(
    binary_lighting_output_write_value_callback cb);

BACNET_STACK_EXPORT
void Binary_Lighting_Output_Blink_Warn_Callback_Set(
    binary_lighting_output_blink_warn_callback cb);

BACNET_STACK_EXPORT
void *Binary_Lighting_Output_Context_Get(uint32_t object_instance);
BACNET_STACK_EXPORT
void Binary_Lighting_Output_Context_Set(
    uint32_t object_instance, void *context);

BACNET_STACK_EXPORT
uint32_t Binary_Lighting_Output_Create(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Binary_Lighting_Output_Delete(uint32_t object_instance);
BACNET_STACK_EXPORT
void Binary_Lighting_Output_Cleanup(void);
BACNET_STACK_EXPORT
void Binary_Lighting_Output_Init(void);

BACNET_STACK_EXPORT
int Binary_Lighting_Output_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);
BACNET_STACK_EXPORT
bool Binary_Lighting_Output_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
