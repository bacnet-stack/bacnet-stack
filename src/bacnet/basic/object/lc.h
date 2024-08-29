/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @brief The Load Control Objects from 135-2004-Addendum e
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_LOAD_CONTROL_H
#define BACNET_BASIC_OBJECT_LOAD_CONTROL_H
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacerror.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"

typedef struct shed_level_data {
    /* Represents the shed levels for the LEVEL choice of
    BACnetShedLevel that have meaning for this particular
    Load Control object. We use 'percent' of baseline. */
    float Value;
    const char *Description;
} BACNET_SHED_LEVEL_DATA;

/**
 * @brief Callback for manipulated object controlled value write
 * @param  object_type - object type of the manipulated object
 * @param  object_instance - object-instance number of the object
 * @param  property_id - property identifier of the manipulated object
 * @param  priority - priority of the write
 * @param  value - value of the write
 */
typedef void (*load_control_manipulated_object_write_callback)(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID property_id,
    uint8_t priority,
    float value);

/**
 * @brief Callback for manipulated object controlled value relinquish
 * @param  object_type - object type of the manipulated object
 * @param  object_instance - object-instance number of the object
 * @param  property_id - property identifier of the manipulated object
 * @param  priority - priority of the relinquish
 */
typedef void (*load_control_manipulated_object_relinquish_callback)(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID property_id,
    uint8_t priority);

/**
 * @brief Callback for manipulated object controlled value read
 * @param  object_type - object type of the manipulated object
 * @param  object_instance - object-instance number of the object
 * @param  property_id - property identifier of the manipulated object
 * @param  priority - present priority of the object property
 * @param  value - present value of the object property
 */
typedef void (*load_control_manipulated_object_read_callback)(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID property_id,
    uint8_t *priority,
    float *value);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void Load_Control_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary);

BACNET_STACK_DEPRECATED("Use Load_Control_Timer() instead")
BACNET_STACK_EXPORT
void Load_Control_State_Machine_Handler(void);

BACNET_STACK_EXPORT
bool Load_Control_Valid_Instance(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned Load_Control_Count(void);
BACNET_STACK_EXPORT
uint32_t Load_Control_Index_To_Instance(unsigned index);
BACNET_STACK_EXPORT
unsigned Load_Control_Instance_To_Index(uint32_t object_instance);

BACNET_STACK_EXPORT
bool Load_Control_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);
BACNET_STACK_EXPORT
bool Load_Control_Name_Set(uint32_t object_instance, const char *new_name);
BACNET_STACK_EXPORT
const char *Load_Control_Name_ASCII(uint32_t object_instance);

BACNET_STACK_EXPORT
BACNET_SHED_STATE Load_Control_Present_Value(uint32_t object_instance);

BACNET_STACK_EXPORT
bool Load_Control_Shed_Level_Array_Set(
    uint32_t object_instance,
    uint32_t array_index,
    const struct shed_level_data *value);
BACNET_STACK_EXPORT
bool Load_Control_Shed_Level_Array(
    uint32_t object_instance,
    uint32_t array_entry,
    struct shed_level_data *value);

BACNET_STACK_EXPORT
uint32_t Load_Control_Create(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Load_Control_Delete(uint32_t object_instance);
BACNET_STACK_EXPORT
void Load_Control_Cleanup(void);

BACNET_STACK_EXPORT
void Load_Control_Init(void);

BACNET_STACK_EXPORT
unsigned Load_Control_Priority_For_Writing(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Load_Control_Priority_For_Writing_Set(
    uint32_t object_instance, unsigned priority);

BACNET_STACK_EXPORT
bool Load_Control_Manipulated_Variable_Reference(
    uint32_t object_instance,
    BACNET_OBJECT_PROPERTY_REFERENCE *object_property_reference);
BACNET_STACK_EXPORT
bool Load_Control_Manipulated_Variable_Reference_Set(
    uint32_t object_instance,
    const BACNET_OBJECT_PROPERTY_REFERENCE *object_property_reference);

BACNET_STACK_EXPORT
void Load_Control_Manipulated_Object_Write_Callback_Set(
    uint32_t object_instance,
    load_control_manipulated_object_write_callback cb);
BACNET_STACK_EXPORT
void Load_Control_Manipulated_Object_Relinquish_Callback_Set(
    uint32_t object_instance,
    load_control_manipulated_object_relinquish_callback cb);
BACNET_STACK_EXPORT
void Load_Control_Manipulated_Object_Read_Callback_Set(
    uint32_t object_instance, load_control_manipulated_object_read_callback cb);

BACNET_STACK_EXPORT
void Load_Control_Timer(uint32_t object_instance, uint16_t milliseconds);

BACNET_STACK_EXPORT
int Load_Control_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);

BACNET_STACK_EXPORT
bool Load_Control_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);

/* functions used for unit testing */
BACNET_STACK_EXPORT
void Load_Control_State_Machine(int object_index, const BACNET_DATE_TIME *bdatetime);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
