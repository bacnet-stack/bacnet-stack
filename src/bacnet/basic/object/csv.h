/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2012
 * @brief API for a basic BACnet CharacterString Value object
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_CHARACTER_STRING_VALUE_H
#define BACNET_BASIC_OBJECT_CHARACTER_STRING_VALUE_H
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacerror.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void CharacterString_Value_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary);

BACNET_STACK_EXPORT
uint32_t CharacterString_Value_Create(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Characterstring_Value_Delete(uint32_t object_instance);
BACNET_STACK_EXPORT
void Characterstring_Value_Cleanup(void);
BACNET_STACK_EXPORT
bool CharacterString_Value_Valid_Instance(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned CharacterString_Value_Count(void);
BACNET_STACK_EXPORT
uint32_t CharacterString_Value_Index_To_Instance(unsigned index);
BACNET_STACK_EXPORT
unsigned CharacterString_Value_Instance_To_Index(uint32_t instance);

BACNET_STACK_EXPORT
int CharacterString_Value_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);

BACNET_STACK_EXPORT
bool CharacterString_Value_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);

/* optional API */
BACNET_STACK_EXPORT
bool CharacterString_Value_Object_Instance_Add(uint32_t instance);

BACNET_STACK_EXPORT
bool CharacterString_Value_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);
BACNET_STACK_EXPORT
bool CharacterString_Value_Name_Set(
    uint32_t object_instance, const char *new_name);

BACNET_STACK_EXPORT
bool CharacterString_Value_Present_Value(
    uint32_t object_instance, BACNET_CHARACTER_STRING *value);
BACNET_STACK_EXPORT
bool CharacterString_Value_Present_Value_Set(
    uint32_t object_instance, const BACNET_CHARACTER_STRING *value);
BACNET_STACK_EXPORT
bool CharacterString_Value_Present_Value_Backup_Set(
    uint32_t object_instance, BACNET_CHARACTER_STRING *present_value);
BACNET_STACK_EXPORT
bool CharacterString_Value_Description_Set(
    uint32_t object_instance, const char *new_descr);
BACNET_STACK_EXPORT
bool CharacterString_Value_Out_Of_Service(uint32_t object_instance);

BACNET_STACK_EXPORT
bool CharacterString_Value_Change_Of_Value(uint32_t instance);
BACNET_STACK_EXPORT
void CharacterString_Value_Change_Of_Value_Clear(uint32_t instance);
BACNET_STACK_EXPORT
bool CharacterString_Value_Encode_Value_List(
    uint32_t object_instance, BACNET_PROPERTY_VALUE *value_list);

BACNET_STACK_EXPORT
void CharacterString_Value_Init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
