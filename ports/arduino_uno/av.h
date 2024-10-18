/**************************************************************************
 *
 * Copyright (C) 2006 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *
 *********************************************************************/
#ifndef AV_H
#define AV_H

#include <stdbool.h>
#include <stdint.h>
#include "bacnet/bacdef.h"
#include "bacnet/bacerror.h"
#include "bacnet/wp.h"

#ifndef MAX_ANALOG_VALUES
#define MAX_ANALOG_VALUES 4
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
void Analog_Value_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary);
bool Analog_Value_Valid_Instance(uint32_t object_instance);
unsigned Analog_Value_Count(void);
uint32_t Analog_Value_Index_To_Instance(unsigned index);
char *Analog_Value_Name(uint32_t object_instance);

int Analog_Value_Encode_Property_APDU(
    uint8_t *apdu,
    uint32_t object_instance,
    BACNET_PROPERTY_ID property,
    uint32_t array_index,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code);

bool Analog_Value_Write_Property(
    BACNET_WRITE_PROPERTY_DATA *wp_data,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code);

bool Analog_Value_Present_Value_Set(
    uint32_t object_instance, float value, uint8_t priority);
float Analog_Value_Present_Value(uint32_t object_instance);

void Analog_Value_Init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
