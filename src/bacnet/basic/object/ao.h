/**
 * @file
 * @author Steve Karg
 * @date 2005
 * @brief API for a basic BACnet Analog Output Object implementation.
 * An Analog Output object is an object with a present-value that
 * uses an single precision floating point data type.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2005
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_ANALOG_OUTPUT_H
#define BACNET_BASIC_OBJECT_ANALOG_OUTPUT_H
#include <stdbool.h>
#include <stdint.h>
#include "bacnet/bacdef.h" /* Must be before all other bacnet *.h files */
#include "bacnet/bacerror.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"

/**
 * @brief Callback for gateway write present value request
 * @param  object_instance - object-instance number of the object
 * @param  old_value - floating point analog value prior to write
 * @param  value - floating point analog value of the write
 */
typedef void (*analog_output_write_present_value_callback)(
    uint32_t object_instance, float old_value, float value);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void Analog_Output_Property_Lists(
    const int32_t **pRequired,
    const int32_t **pOptional,
    const int32_t **pProprietary);
BACNET_STACK_EXPORT
bool Analog_Output_Valid_Instance(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned Analog_Output_Count(void);
BACNET_STACK_EXPORT
uint32_t Analog_Output_Index_To_Instance(unsigned index);
BACNET_STACK_EXPORT
unsigned Analog_Output_Instance_To_Index(uint32_t instance);
BACNET_STACK_EXPORT
bool Analog_Output_Object_Instance_Add(uint32_t instance);

BACNET_STACK_EXPORT
float Analog_Output_Present_Value(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Analog_Output_Present_Value_Set(
    uint32_t object_instance, float value, unsigned priority);
BACNET_STACK_EXPORT
bool Analog_Output_Present_Value_Relinquish(
    uint32_t object_instance, unsigned priority);
BACNET_STACK_EXPORT
unsigned Analog_Output_Present_Value_Priority(uint32_t object_instance);
BACNET_STACK_EXPORT
void Analog_Output_Write_Present_Value_Callback_Set(
    analog_output_write_present_value_callback cb);

BACNET_STACK_EXPORT
bool Analog_Output_Priority_Array_Relinquished(
    uint32_t object_instance, unsigned priority);
BACNET_STACK_EXPORT
float Analog_Output_Priority_Array_Value(
    uint32_t object_instance, unsigned priority);

BACNET_STACK_EXPORT
float Analog_Output_Relinquish_Default(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Analog_Output_Relinquish_Default_Set(
    uint32_t object_instance, float value);

BACNET_STACK_EXPORT
bool Analog_Output_Change_Of_Value(uint32_t instance);
BACNET_STACK_EXPORT
void Analog_Output_Change_Of_Value_Clear(uint32_t instance);
BACNET_STACK_EXPORT
bool Analog_Output_Encode_Value_List(
    uint32_t object_instance, BACNET_PROPERTY_VALUE *value_list);
BACNET_STACK_EXPORT
float Analog_Output_COV_Increment(uint32_t instance);
BACNET_STACK_EXPORT
void Analog_Output_COV_Increment_Set(uint32_t instance, float value);

BACNET_STACK_EXPORT
bool Analog_Output_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);
BACNET_STACK_EXPORT
bool Analog_Output_Name_Set(uint32_t object_instance, const char *new_name);
BACNET_STACK_EXPORT
const char *Analog_Output_Name_ASCII(uint32_t object_instance);

BACNET_STACK_EXPORT
const char *Analog_Output_Description(uint32_t instance);
BACNET_STACK_EXPORT
bool Analog_Output_Description_Set(uint32_t instance, const char *new_name);

BACNET_STACK_EXPORT
bool Analog_Output_Units_Set(uint32_t instance, BACNET_ENGINEERING_UNITS units);
BACNET_STACK_EXPORT
BACNET_ENGINEERING_UNITS Analog_Output_Units(uint32_t instance);

BACNET_STACK_EXPORT
bool Analog_Output_Out_Of_Service(uint32_t instance);
BACNET_STACK_EXPORT
void Analog_Output_Out_Of_Service_Set(uint32_t instance, bool oos_flag);

BACNET_STACK_EXPORT
bool Analog_Output_Overridden(uint32_t instance);
BACNET_STACK_EXPORT
void Analog_Output_Overridden_Set(uint32_t instance, bool oos_flag);

BACNET_STACK_EXPORT
BACNET_RELIABILITY Analog_Output_Reliability(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Analog_Output_Reliability_Set(
    uint32_t object_instance, BACNET_RELIABILITY value);

BACNET_STACK_EXPORT
float Analog_Output_Min_Pres_Value(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Analog_Output_Min_Pres_Value_Set(uint32_t object_instance, float value);
BACNET_STACK_EXPORT
float Analog_Output_Max_Pres_Value(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Analog_Output_Max_Pres_Value_Set(uint32_t object_instance, float value);

BACNET_STACK_EXPORT
int Analog_Output_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);
BACNET_STACK_EXPORT
bool Analog_Output_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);

BACNET_STACK_EXPORT
void *Analog_Output_Context_Get(uint32_t object_instance);
BACNET_STACK_EXPORT
void Analog_Output_Context_Set(uint32_t object_instance, void *context);

BACNET_STACK_EXPORT
uint32_t Analog_Output_Create(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Analog_Output_Delete(uint32_t object_instance);
BACNET_STACK_EXPORT
void Analog_Output_Cleanup(void);
BACNET_STACK_EXPORT
void Analog_Output_Init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
