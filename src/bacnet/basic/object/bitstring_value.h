/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date June 2024
 * @brief The BitString Value object is an object with a present-value that
 * uses a BACnetBitString data type.
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_BITSTRING_VALUE_H
#define BACNET_BASIC_OBJECT_BITSTRING_VALUE_H

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
 * @param  old_value - BIT STRING value prior to write
 * @param  value - BIT STRING value of the write
 */
typedef void (*bitstring_value_write_present_value_callback)(
    uint32_t object_instance,
    BACNET_BIT_STRING *old_value,
    BACNET_BIT_STRING *value);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
BACNET_STACK_EXPORT
void BitString_Value_Write_Present_Value_Callback_Set(
    bitstring_value_write_present_value_callback cb);

BACNET_STACK_EXPORT
void BitString_Value_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary);

BACNET_STACK_EXPORT
bool BitString_Value_Valid_Instance(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned BitString_Value_Count(void);
BACNET_STACK_EXPORT
uint32_t BitString_Value_Index_To_Instance(unsigned index);
BACNET_STACK_EXPORT
unsigned BitString_Value_Instance_To_Index(uint32_t instance);

BACNET_STACK_EXPORT
int BitString_Value_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);

BACNET_STACK_EXPORT
bool BitString_Value_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);

/* optional API */
BACNET_STACK_EXPORT
bool BitString_Value_Object_Instance_Add(uint32_t instance);

BACNET_STACK_EXPORT
bool BitString_Value_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);
BACNET_STACK_EXPORT
bool BitString_Value_Name_Set(uint32_t object_instance, const char *new_name);
BACNET_STACK_EXPORT
const char *BitString_Value_Name_ASCII(uint32_t object_instance);

BACNET_STACK_EXPORT
bool BitString_Value_Present_Value(
    uint32_t object_instance, BACNET_BIT_STRING *value);
BACNET_STACK_EXPORT
bool BitString_Value_Present_Value_Set(
    uint32_t object_instance, const BACNET_BIT_STRING *value);

BACNET_STACK_EXPORT
const char *BitString_Value_Description(uint32_t instance);
BACNET_STACK_EXPORT
bool BitString_Value_Description_Set(
    uint32_t object_instance, const char *text_string);

BACNET_STACK_EXPORT
bool BitString_Value_Out_Of_Service(uint32_t object_instance);
BACNET_STACK_EXPORT
void BitString_Value_Out_Of_Service_Set(uint32_t object_instance, bool value);

BACNET_STACK_EXPORT
BACNET_RELIABILITY BitString_Value_Reliablity(uint32_t object_instance);
BACNET_STACK_EXPORT
bool BitString_Value_Reliablity_Set(
    uint32_t object_instance, BACNET_RELIABILITY value);

BACNET_STACK_EXPORT
bool BitString_Value_Change_Of_Value(uint32_t instance);
BACNET_STACK_EXPORT
void BitString_Value_Change_Of_Value_Clear(uint32_t instance);
BACNET_STACK_EXPORT
bool BitString_Value_Encode_Value_List(
    uint32_t object_instance, BACNET_PROPERTY_VALUE *value_list);

BACNET_STACK_EXPORT
bool BitString_Value_Write_Enabled(uint32_t instance);
BACNET_STACK_EXPORT
void BitString_Value_Write_Enable(uint32_t instance);
BACNET_STACK_EXPORT
void BitString_Value_Write_Disable(uint32_t instance);

BACNET_STACK_EXPORT
void *BitString_Value_Context_Get(uint32_t object_instance);
BACNET_STACK_EXPORT
void BitString_Value_Context_Set(uint32_t object_instance, void *context);

BACNET_STACK_EXPORT
uint32_t BitString_Value_Create(uint32_t object_instance);
BACNET_STACK_EXPORT
bool BitString_Value_Delete(uint32_t object_instance);
BACNET_STACK_EXPORT
void BitString_Value_Cleanup(void);

BACNET_STACK_EXPORT
void BitString_Value_Init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
