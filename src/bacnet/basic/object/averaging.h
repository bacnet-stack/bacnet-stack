/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2026
 * @brief API for a basic BACnet Averaging object implementation.
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_AVERAGING_H
#define BACNET_BASIC_OBJECT_AVERAGING_H

#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacerror.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"
#include "bacnet/bacdevobjpropref.h"
#include "bacnet/basic/services.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void Averaging_Property_Lists(
    const int32_t **pRequired,
    const int32_t **pOptional,
    const int32_t **pProprietary);
BACNET_STACK_EXPORT
void Averaging_Writable_Property_List(
    uint32_t object_instance, const int32_t **properties);

BACNET_STACK_EXPORT
bool Averaging_Valid_Instance(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned Averaging_Count(void);
BACNET_STACK_EXPORT
uint32_t Averaging_Index_To_Instance(unsigned index);
BACNET_STACK_EXPORT
unsigned Averaging_Instance_To_Index(uint32_t object_instance);

BACNET_STACK_EXPORT
bool Averaging_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);
BACNET_STACK_EXPORT
bool Averaging_Name_Set(uint32_t object_instance, const char *new_name);
BACNET_STACK_EXPORT
const char *Averaging_Name_ASCII(uint32_t object_instance);

BACNET_STACK_EXPORT
int Averaging_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);
BACNET_STACK_EXPORT
bool Averaging_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);

BACNET_STACK_EXPORT
float Averaging_Minimum_Value(uint32_t object_instance);
BACNET_STACK_EXPORT
float Averaging_Average_Value(uint32_t object_instance);
BACNET_STACK_EXPORT
float Averaging_Variance_Value(uint32_t object_instance);
BACNET_STACK_EXPORT
float Averaging_Maximum_Value(uint32_t object_instance);

BACNET_STACK_EXPORT
uint32_t Averaging_Attempted_Samples(uint32_t object_instance);
BACNET_STACK_EXPORT
uint32_t Averaging_Valid_Samples(uint32_t object_instance);
BACNET_STACK_EXPORT
uint32_t Averaging_Window_Interval(uint32_t object_instance);
BACNET_STACK_EXPORT
uint32_t Averaging_Window_Samples(uint32_t object_instance);

BACNET_STACK_EXPORT
bool Averaging_Object_Property_Reference(
    uint32_t object_instance, BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *value);
BACNET_STACK_EXPORT
bool Averaging_Object_Property_Reference_Set(
    uint32_t object_instance,
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *value);

BACNET_STACK_EXPORT
bool Averaging_Sample_Add(uint32_t object_instance, float sample_value);
BACNET_STACK_EXPORT
bool Averaging_Sample_Miss(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Averaging_Reset(uint32_t object_instance);

BACNET_STACK_EXPORT
const char *Averaging_Description(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Averaging_Description_Set(uint32_t object_instance, const char *new_name);

BACNET_STACK_EXPORT
void *Averaging_Context_Get(uint32_t object_instance);
BACNET_STACK_EXPORT
void Averaging_Context_Set(uint32_t object_instance, void *context);

BACNET_STACK_EXPORT
void Averaging_Timer(uint32_t object_instance, uint16_t milliseconds);
BACNET_STACK_EXPORT
void Averaging_Read_Property_Internal_Callback_Set(read_property_function cb);

BACNET_STACK_EXPORT
uint32_t Averaging_Create(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Averaging_Delete(uint32_t object_instance);
BACNET_STACK_EXPORT
void Averaging_Cleanup(void);
BACNET_STACK_EXPORT
void Averaging_Init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
