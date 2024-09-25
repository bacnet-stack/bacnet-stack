/**************************************************************************
 *
 * Copyright (C) 2005 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *
 *********************************************************************/
#ifndef AI_H
#define AI_H

#include <stdbool.h>
#include <stdint.h>
#include "bacnet/bacdef.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
void Analog_Input_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary);

bool Analog_Input_Valid_Instance(uint32_t object_instance);
unsigned Analog_Input_Count(void);
uint32_t Analog_Input_Index_To_Instance(unsigned index);
unsigned Analog_Input_Instance_To_Index(uint32_t instance);
bool Analog_Input_Object_Instance_Add(uint32_t instance);

char *Analog_Input_Name(uint32_t object_instance);
bool Analog_Input_Name_Set(uint32_t object_instance, const char *new_name);

const char *Analog_Input_Description(uint32_t instance);
bool Analog_Input_Description_Set(uint32_t instance, const char *new_name);

bool Analog_Input_Units_Set(uint32_t instance, uint16_t units);
uint16_t Analog_Input_Units(uint32_t instance);

int Analog_Input_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);
bool Analog_Input_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);

float Analog_Input_Present_Value(uint32_t object_instance);
void Analog_Input_Present_Value_Set(uint32_t object_instance, float value);

void Analog_Input_Init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#define ANALOG_INPUT_OBJ_FUNCTIONS                                   \
    OBJECT_ANALOG_INPUT, Analog_Input_Init, Analog_Input_Count,      \
        Analog_Input_Index_To_Instance, Analog_Input_Valid_Instance, \
        Analog_Input_Name, Analog_Input_Read_Property, NULL,         \
        Analog_Input_Property_Lists, NULL, NULL
#endif
