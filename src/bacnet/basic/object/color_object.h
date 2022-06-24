/**
 * @file
 * @author Steve Karg
 * @date 2022
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

void Color_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary);
bool Color_Valid_Instance(uint32_t object_instance);
unsigned Color_Count(void);
uint32_t Color_Index_To_Instance(unsigned index);
unsigned Color_Instance_To_Index(uint32_t object_instance);

bool Color_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);
bool Color_Name_Set(uint32_t object_instance, char *new_name);

int Color_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);

bool Color_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);

bool Color_Present_Value_Set(
    uint32_t object_instance, BACNET_XY_COLOR *value, uint8_t priority);
bool Color_Present_Value(uint32_t object_instance, BACNET_XY_COLOR *value);
void Color_Write_Present_Value_Callback_Set(
    color_write_present_value_callback cb);

bool Color_Tracking_Value_Set(
    uint32_t object_instance, BACNET_XY_COLOR *value);
bool Color_Tracking_Value(uint32_t object_instance, BACNET_XY_COLOR *value);

bool Color_Command(uint32_t object_instance, BACNET_COLOR_COMMAND *value);
bool Color_Command_Set(uint32_t object_instance, BACNET_COLOR_COMMAND *value);

bool Color_Default_Color_Set(
    uint32_t object_instance, BACNET_XY_COLOR *value);
bool Color_Default_Color(uint32_t object_instance, BACNET_XY_COLOR *value);

char *Color_Description(uint32_t instance);
bool Color_Description_Set(uint32_t instance, char *new_name);

bool Color_Write_Enabled(uint32_t instance);
void Color_Write_Enable(uint32_t instance);
void Color_Write_Disable(uint32_t instance);

bool Color_Create(uint32_t object_instance);
bool Color_Delete(uint32_t object_instance);
void Color_Cleanup(void);
void Color_Init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
