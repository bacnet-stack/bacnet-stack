/**************************************************************************
*
* Copyright (C) 2006 Steve Karg <skarg@users.sourceforge.net>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*********************************************************************/
#ifndef AV_H
#define AV_H

#include <stdbool.h>
#include <stdint.h>
#include "bacdef.h"
#include "bacerror.h"
#include "wp.h"
#include "rp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
    void Analog_Value_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);
    bool Analog_Value_Valid_Instance(
        uint32_t object_instance);
    unsigned Analog_Value_Count(
        void);
    uint32_t Analog_Value_Index_To_Instance(
        unsigned index);
    unsigned Analog_Value_Instance_To_Index(
        uint32_t object_instance);
    char *Analog_Value_Name(
        uint32_t object_instance);

    int Analog_Value_Read_Property(
        BACNET_READ_PROPERTY_DATA * rpdata);

    bool Analog_Value_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data);

    bool Analog_Value_Present_Value_Set(
        uint32_t object_instance,
        float value,
        uint8_t priority);
    float Analog_Value_Present_Value(
        uint32_t object_instance);

    void Analog_Value_Init(
        void);

#ifdef TEST
#include "ctest.h"
    void testAnalog_Value(
        Test * pTest);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#define ANALOG_VALUE_OBJ_FUNCTIONS \
    OBJECT_ANALOG_VALUE, Analog_Value_Init, Analog_Value_Count, \
    Analog_Value_Index_To_Instance, Analog_Value_Valid_Instance, \
    Analog_Value_Name, Analog_Value_Read_Property, \
    Analog_Value_Write_Property, Analog_Value_Property_Lists, NULL, \
    NULL
#endif
