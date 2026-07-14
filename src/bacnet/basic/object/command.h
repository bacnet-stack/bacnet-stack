/**
 * @file
 * @author Nikola Jelic <nikola.jelic@euroicc.com>
 * @date 2014
 * @brief API for basic  Command objects, customize for your use
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_COMMAND_H
#define BACNET_BASIC_OBJECT_COMMAND_H
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacaction.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void Command_Property_Lists(
    const int32_t **pRequired,
    const int32_t **pOptional,
    const int32_t **pProprietary);
BACNET_STACK_EXPORT
void Command_Writable_Property_List(
    uint32_t object_instance, const int32_t **properties);

BACNET_STACK_EXPORT
bool Command_Valid_Instance(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned Command_Count(void);
BACNET_STACK_EXPORT
uint32_t Command_Index_To_Instance(unsigned index);
BACNET_STACK_EXPORT
unsigned Command_Instance_To_Index(uint32_t instance);
BACNET_STACK_EXPORT
uint32_t Command_Create(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Command_Delete(uint32_t object_instance);
BACNET_STACK_EXPORT
void Command_Cleanup(void);

BACNET_STACK_EXPORT
bool Command_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);
BACNET_STACK_EXPORT
bool Command_Name_Set(uint32_t object_instance, const char *new_name);

BACNET_STACK_EXPORT
const char *Command_Description(uint32_t instance);
BACNET_STACK_EXPORT
bool Command_Description_Set(uint32_t instance, const char *new_name);

BACNET_STACK_EXPORT
int Command_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);
BACNET_STACK_EXPORT
bool Command_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);

BACNET_STACK_EXPORT
void Command_Write_Property_Internal_Callback_Set(write_property_function cb);

BACNET_STACK_EXPORT
uint32_t Command_Present_Value(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Command_Present_Value_Set(uint32_t object_instance, uint32_t value);

BACNET_STACK_EXPORT
bool Command_In_Process(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Command_In_Process_Set(uint32_t object_instance, bool value);

BACNET_STACK_EXPORT
bool Command_All_Writes_Successful(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Command_All_Writes_Successful_Set(uint32_t object_instance, bool value);

BACNET_STACK_EXPORT
bool Command_Change_Of_Value(uint32_t instance);
BACNET_STACK_EXPORT
void Command_Change_Of_Value_Clear(uint32_t instance);
BACNET_STACK_EXPORT
bool Command_Encode_Value_List(
    uint32_t object_instance, BACNET_PROPERTY_VALUE *value_list);
float Command_COV_Increment(uint32_t instance);
BACNET_STACK_EXPORT
void Command_COV_Increment_Set(uint32_t instance, float value);

BACNET_STACK_EXPORT
BACNET_ACTION_LIST *
Command_Action_List_Entry(uint32_t instance, unsigned index);
BACNET_STACK_EXPORT
unsigned Command_Action_List_Count(uint32_t instance);

/* note: header of Intrinsic_Reporting function is required
   even when INTRINSIC_REPORTING is not defined */
BACNET_STACK_EXPORT
void Command_Intrinsic_Reporting(uint32_t object_instance);

BACNET_STACK_EXPORT
void Command_Timer(uint32_t object_instance, uint16_t milliseconds);

BACNET_STACK_EXPORT
void Command_Init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
