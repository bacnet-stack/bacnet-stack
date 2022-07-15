/**
 * @file
 * @author Steve Karg
 * @date June 2022
 * @brief Color object, customize for your use
 *
 * @section DESCRIPTION
 *
 * The Color object is an object with a present-value that
 * uses an x,y color single precision floating point data type.
 *
 * @section LICENSE
 *
 * Copyright (C) 2022 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef BACNET_COLOR_OBJECT_H
#define BACNET_COLOR_OBJECT_H

#include <stdbool.h>
#include <stdint.h>
#include "bacnet/bacnet_stack_exports.h"
#include "bacnet/config.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacenum.h"
#include "bacnet/bacerror.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"

/**
 * @brief Callback for gateway write present value request
 * @param  object_instance - object-instance number of the object
 * @param  old_value - BACnetXYColor value prior to write
 * @param  value - BACnetXYColor value of the write
 */
typedef void (*color_write_present_value_callback)(uint32_t object_instance,
    BACNET_XY_COLOR *old_value,
    BACNET_XY_COLOR *value);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void Color_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary);
BACNET_STACK_EXPORT
bool Color_Valid_Instance(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned Color_Count(void);
BACNET_STACK_EXPORT
uint32_t Color_Index_To_Instance(unsigned index);
BACNET_STACK_EXPORT
unsigned Color_Instance_To_Index(uint32_t object_instance);

BACNET_STACK_EXPORT
bool Color_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);
BACNET_STACK_EXPORT
bool Color_Name_Set(uint32_t object_instance, char *new_name);

BACNET_STACK_EXPORT
int Color_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);

BACNET_STACK_EXPORT
bool Color_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);

BACNET_STACK_EXPORT
bool Color_Present_Value_Set(uint32_t object_instance, BACNET_XY_COLOR *value);
BACNET_STACK_EXPORT
bool Color_Present_Value(uint32_t object_instance, BACNET_XY_COLOR *value);
BACNET_STACK_EXPORT
void Color_Write_Present_Value_Callback_Set(
    color_write_present_value_callback cb);

BACNET_STACK_EXPORT
bool Color_Tracking_Value_Set(uint32_t object_instance, BACNET_XY_COLOR *value);
BACNET_STACK_EXPORT
bool Color_Tracking_Value(uint32_t object_instance, BACNET_XY_COLOR *value);

BACNET_STACK_EXPORT
bool Color_Command(uint32_t object_instance, BACNET_COLOR_COMMAND *value);
BACNET_STACK_EXPORT
bool Color_Command_Set(uint32_t object_instance, BACNET_COLOR_COMMAND *value);

BACNET_STACK_EXPORT
bool Color_Default_Color_Set(uint32_t object_instance, BACNET_XY_COLOR *value);
BACNET_STACK_EXPORT
bool Color_Default_Color(uint32_t object_instance, BACNET_XY_COLOR *value);

BACNET_STACK_EXPORT
uint32_t Color_Default_Fade_Time(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Color_Default_Fade_Time_Set(uint32_t object_instance, uint32_t value);

BACNET_STACK_EXPORT
BACNET_COLOR_OPERATION_IN_PROGRESS Color_In_Progress(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Color_In_Progress_Set(
    uint32_t object_instance, BACNET_COLOR_OPERATION_IN_PROGRESS value);

BACNET_STACK_EXPORT
BACNET_COLOR_TRANSITION Color_Transition(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Color_Transition_Set(
    uint32_t object_instance, BACNET_COLOR_TRANSITION value);

BACNET_STACK_EXPORT
char *Color_Description(uint32_t instance);
BACNET_STACK_EXPORT
bool Color_Description_Set(uint32_t instance, char *new_name);

BACNET_STACK_EXPORT
bool Color_Write_Enabled(uint32_t instance);
BACNET_STACK_EXPORT
void Color_Write_Enable(uint32_t instance);
BACNET_STACK_EXPORT
void Color_Write_Disable(uint32_t instance);

BACNET_STACK_EXPORT
bool Color_Create(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Color_Delete(uint32_t object_instance);
BACNET_STACK_EXPORT
void Color_Cleanup(void);
BACNET_STACK_EXPORT
void Color_Init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
