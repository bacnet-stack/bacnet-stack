/**
 * @file
 * @brief API for Loop object type
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date November 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_LOOP_H
#define BACNET_BASIC_OBJECT_LOOP_H
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacerror.h"
#include "bacnet/timer_value.h"
#include "bacnet/wp.h"
#include "bacnet/rp.h"
#include "bacnet/list_element.h"

/**
 * @brief Callback for tracking the loop writes for logging or other purposes
 * @param  instance - loop object instance number
 * @param  status - true if write was successful
 * @param  wp_data - pointer to the write property data structure
 */
typedef void (*loop_write_property_callback)(
    uint32_t instance, bool status, BACNET_WRITE_PROPERTY_DATA *wp_data);
/* linked list structure for notifications */
struct loop_write_property_notification;
struct loop_write_property_notification {
    struct loop_write_property_notification *next;
    loop_write_property_callback callback;
};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void Loop_Property_Lists(
    const int32_t **pRequired,
    const int32_t **pOptional,
    const int32_t **pProprietary);
BACNET_STACK_EXPORT
void Loop_Writable_Property_List(
    uint32_t object_instance, const int32_t **properties);

BACNET_STACK_EXPORT
void Loop_Proprietary_Property_List_Set(const int32_t *pProprietary);
BACNET_STACK_EXPORT
void Loop_Read_Property_Proprietary_Callback_Set(read_property_function cb);
BACNET_STACK_EXPORT
void Loop_Write_Property_Proprietary_Callback_Set(write_property_function cb);

BACNET_STACK_EXPORT
bool Loop_Valid_Instance(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned Loop_Count(void);
BACNET_STACK_EXPORT
uint32_t Loop_Index_To_Instance(unsigned index);
BACNET_STACK_EXPORT
unsigned Loop_Instance_To_Index(uint32_t object_instance);

BACNET_STACK_EXPORT
bool Loop_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);
BACNET_STACK_EXPORT
bool Loop_Name_Set(uint32_t object_instance, const char *new_name);
BACNET_STACK_EXPORT
const char *Loop_Name_ASCII(uint32_t object_instance);

BACNET_STACK_EXPORT
bool Loop_Description(
    uint32_t object_instance, BACNET_CHARACTER_STRING *description);
BACNET_STACK_EXPORT
bool Loop_Description_Set(uint32_t instance, const char *new_name);
BACNET_STACK_EXPORT
const char *Loop_Description_ANSI(uint32_t object_instance);

BACNET_STACK_EXPORT
bool Loop_Out_Of_Service(uint32_t instance);
BACNET_STACK_EXPORT
void Loop_Out_Of_Service_Set(uint32_t instance, bool oos_flag);

BACNET_STACK_EXPORT
BACNET_RELIABILITY Loop_Reliability(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Loop_Reliability_Set(uint32_t object_instance, BACNET_RELIABILITY value);

BACNET_STACK_EXPORT
float Loop_Present_Value(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Loop_Present_Value_Set(uint32_t object_instance, float value);

BACNET_STACK_EXPORT
uint32_t Loop_Update_Interval(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Loop_Update_Interval_Set(uint32_t object_instance, uint32_t value);

BACNET_STACK_EXPORT
BACNET_ENGINEERING_UNITS Loop_Output_Units(uint32_t instance);
BACNET_STACK_EXPORT
bool Loop_Output_Units_Set(uint32_t instance, BACNET_ENGINEERING_UNITS units);

BACNET_STACK_EXPORT
bool Loop_Manipulated_Variable_Reference(
    uint32_t object_instance, BACNET_OBJECT_PROPERTY_REFERENCE *value);
BACNET_STACK_EXPORT
bool Loop_Manipulated_Variable_Reference_Set(
    uint32_t object_instance, BACNET_OBJECT_PROPERTY_REFERENCE *value);

BACNET_STACK_EXPORT
bool Loop_Controlled_Variable_Reference(
    uint32_t object_instance, BACNET_OBJECT_PROPERTY_REFERENCE *value);
BACNET_STACK_EXPORT
bool Loop_Controlled_Variable_Reference_Set(
    uint32_t object_instance, BACNET_OBJECT_PROPERTY_REFERENCE *value);

BACNET_STACK_EXPORT
float Loop_Controlled_Variable_Value(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Loop_Controlled_Variable_Value_Set(uint32_t object_instance, float value);

BACNET_STACK_EXPORT
BACNET_ENGINEERING_UNITS Loop_Controlled_Variable_Units(uint32_t instance);
BACNET_STACK_EXPORT
bool Loop_Controlled_Variable_Units_Set(
    uint32_t instance, BACNET_ENGINEERING_UNITS units);

BACNET_STACK_EXPORT
bool Loop_Setpoint_Reference(
    uint32_t instance, BACNET_OBJECT_PROPERTY_REFERENCE *value);
BACNET_STACK_EXPORT
bool Loop_Setpoint_Reference_Set(
    uint32_t instance, BACNET_OBJECT_PROPERTY_REFERENCE *value);

BACNET_STACK_EXPORT
float Loop_Setpoint(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Loop_Setpoint_Set(uint32_t object_instance, float value);

BACNET_STACK_EXPORT
BACNET_ACTION Loop_Action(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Loop_Action_Set(uint32_t object_instance, BACNET_ACTION value);

BACNET_STACK_EXPORT
float Loop_Proportional_Constant(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Loop_Proportional_Constant_Set(uint32_t object_instance, float value);
BACNET_STACK_EXPORT
BACNET_ENGINEERING_UNITS Loop_Proportional_Constant_Units(uint32_t instance);
BACNET_STACK_EXPORT
bool Loop_Proportional_Constant_Units_Set(
    uint32_t instance, BACNET_ENGINEERING_UNITS units);
BACNET_STACK_EXPORT
float Loop_Integral_Constant(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Loop_Integral_Constant_Set(uint32_t object_instance, float value);
BACNET_STACK_EXPORT
BACNET_ENGINEERING_UNITS Loop_Integral_Constant_Units(uint32_t instance);
BACNET_STACK_EXPORT
bool Loop_Integral_Constant_Units_Set(
    uint32_t instance, BACNET_ENGINEERING_UNITS units);
BACNET_STACK_EXPORT
float Loop_Derivative_Constant(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Loop_Derivative_Constant_Set(uint32_t object_instance, float value);
BACNET_STACK_EXPORT
BACNET_ENGINEERING_UNITS Loop_Derivative_Constant_Units(uint32_t instance);
BACNET_STACK_EXPORT
bool Loop_Derivative_Constant_Units_Set(
    uint32_t instance, BACNET_ENGINEERING_UNITS units);

BACNET_STACK_EXPORT
float Loop_Bias(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Loop_Bias_Set(uint32_t object_instance, float value);

BACNET_STACK_EXPORT
float Loop_Maximum_Output(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Loop_Maximum_Output_Set(uint32_t object_instance, float value);

BACNET_STACK_EXPORT
float Loop_Minimum_Output(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Loop_Minimum_Output_Set(uint32_t object_instance, float value);

BACNET_STACK_EXPORT
uint8_t Loop_Priority_For_Writing(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Loop_Priority_For_Writing_Set(uint32_t object_instance, uint8_t value);

BACNET_STACK_EXPORT
float Loop_COV_Increment(uint32_t instance);
BACNET_STACK_EXPORT
bool Loop_COV_Increment_Set(uint32_t instance, float value);

BACNET_STACK_EXPORT
bool Loop_Reliability_Evaluation_Inhibit(uint32_t instance);
BACNET_STACK_EXPORT
bool Loop_Reliability_Evaluation_Inhibit_Set(uint32_t instance, bool value);

BACNET_STACK_EXPORT
void Loop_Timer(uint32_t object_instance, uint16_t elapsed_milliseconds);

BACNET_STACK_EXPORT
void Loop_Write_Property_Internal_Callback_Set(write_property_function cb);
BACNET_STACK_EXPORT
void Loop_Read_Property_Internal_Callback_Set(read_property_function cb);
BACNET_STACK_EXPORT
void Loop_Write_Property_Notification_Add(
    struct loop_write_property_notification *notification);
BACNET_STACK_EXPORT
void Loop_Write_Property_Notify(
    uint32_t instance, bool status, BACNET_WRITE_PROPERTY_DATA *wp_data);

BACNET_STACK_EXPORT
int Loop_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);
BACNET_STACK_EXPORT
bool Loop_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);

BACNET_STACK_EXPORT
uint32_t Loop_Create(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Loop_Delete(uint32_t object_instance);

BACNET_STACK_EXPORT
void Loop_Cleanup(void);
BACNET_STACK_EXPORT
size_t Loop_Size(void);
BACNET_STACK_EXPORT
void Loop_Init(void);

BACNET_STACK_EXPORT
void *Loop_Context_Get(uint32_t object_instance);
BACNET_STACK_EXPORT
void Loop_Context_Set(uint32_t object_instance, void *context);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
