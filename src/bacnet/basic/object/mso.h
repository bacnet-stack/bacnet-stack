/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2005
 * @brief API for basic Multi-State Output objects
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_MULTI_STATE_OUTPUT_H
#define BACNET_BASIC_OBJECT_MULTI_STATE_OUTPUT_H

#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacerror.h"
#include "bacnet/cov.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"

/**
 * @brief Callback for gateway write present value request
 * @param  object_instance - object-instance number of the object
 * @param  old_value - multistate preset-value prior to write
 * @param  value - multistate preset-value of the write
 */
typedef void (*multistate_output_write_present_value_callback)(
    uint32_t object_instance, uint32_t old_value, uint32_t value);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void Multistate_Output_Property_Lists(
    const int32_t **pRequired,
    const int32_t **pOptional,
    const int32_t **pProprietary);
BACNET_STACK_EXPORT
void Multistate_Output_Writable_Property_List(
    uint32_t object_instance, const int32_t **properties);

BACNET_STACK_EXPORT
bool Multistate_Output_Valid_Instance(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned Multistate_Output_Count(void);
BACNET_STACK_EXPORT
uint32_t Multistate_Output_Index_To_Instance(unsigned index);
BACNET_STACK_EXPORT
unsigned Multistate_Output_Instance_To_Index(uint32_t object_instance);

BACNET_STACK_EXPORT
int Multistate_Output_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);

BACNET_STACK_EXPORT
bool Multistate_Output_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);

BACNET_STACK_EXPORT
bool Multistate_Output_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);
BACNET_STACK_EXPORT
bool Multistate_Output_Name_Set(uint32_t object_instance, const char *new_name);
BACNET_STACK_EXPORT
const char *Multistate_Output_Name_ASCII(uint32_t object_instance);

BACNET_STACK_EXPORT
uint32_t Multistate_Output_Present_Value(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Multistate_Output_Present_Value_Set(
    uint32_t object_instance, uint32_t value, unsigned priority);

BACNET_STACK_EXPORT
bool Multistate_Output_Priority_Array_Relinquished(
    uint32_t object_instance, unsigned priority);
BACNET_STACK_EXPORT
uint32_t Multistate_Output_Priority_Array_Value(
    uint32_t object_instance, unsigned priority);

BACNET_STACK_EXPORT
bool Multistate_Output_Present_Value_Relinquish(
    uint32_t instance, unsigned priority);
BACNET_STACK_EXPORT
unsigned Multistate_Output_Present_Value_Priority(uint32_t object_instance);
BACNET_STACK_EXPORT
void Multistate_Output_Write_Present_Value_Callback_Set(
    multistate_output_write_present_value_callback cb);

BACNET_STACK_EXPORT
bool Multistate_Output_Change_Of_Value(uint32_t instance);
BACNET_STACK_EXPORT
void Multistate_Output_Change_Of_Value_Clear(uint32_t instance);
BACNET_STACK_EXPORT
bool Multistate_Output_Encode_Value_List(
    uint32_t object_instance, BACNET_PROPERTY_VALUE *value_list);

BACNET_STACK_EXPORT
bool Multistate_Output_Out_Of_Service(uint32_t object_instance);
BACNET_STACK_EXPORT
void Multistate_Output_Out_Of_Service_Set(uint32_t object_instance, bool value);

BACNET_STACK_EXPORT
const char *Multistate_Output_Description(uint32_t instance);
BACNET_STACK_EXPORT
bool Multistate_Output_Description_Set(
    uint32_t object_instance, const char *text_string);

BACNET_STACK_EXPORT
bool Multistate_Output_State_Text_List_Set(
    uint32_t object_instance, const char *state_text_list);
BACNET_STACK_EXPORT
bool Multistate_Output_State_Text_Set(
    uint32_t object_instance, uint32_t state_index, char *new_name);
BACNET_STACK_EXPORT
bool Multistate_Output_Max_States_Set(
    uint32_t instance, uint32_t max_states_requested);
BACNET_STACK_EXPORT
uint32_t Multistate_Output_Max_States(uint32_t instance);
BACNET_STACK_EXPORT
const char *
Multistate_Output_State_Text(uint32_t object_instance, uint32_t state_index);

BACNET_STACK_EXPORT
BACNET_RELIABILITY Multistate_Output_Reliability(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Multistate_Output_Reliability_Set(
    uint32_t object_instance, BACNET_RELIABILITY value);

BACNET_STACK_EXPORT
uint32_t Multistate_Output_Relinquish_Default(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Multistate_Output_Relinquish_Default_Set(
    uint32_t object_instance, uint32_t value);

BACNET_STACK_EXPORT
void *Multistate_Output_Context_Get(uint32_t object_instance);
BACNET_STACK_EXPORT
void Multistate_Output_Context_Set(uint32_t object_instance, void *context);

BACNET_STACK_EXPORT
uint32_t Multistate_Output_Create(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Multistate_Output_Delete(uint32_t object_instance);
BACNET_STACK_EXPORT
void Multistate_Output_Cleanup(void);

BACNET_STACK_EXPORT
void Multistate_Output_Init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
