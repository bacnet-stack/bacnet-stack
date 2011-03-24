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
#ifndef BV_H
#define BV_H

#include <stdbool.h>
#include <stdint.h>
#include "bacdef.h"
#include "bacerror.h"
#include "rp.h"
#include "wp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    void Binary_Value_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);
    bool Binary_Value_Valid_Instance(
        uint32_t object_instance);
    unsigned Binary_Value_Count(
        void);
    uint32_t Binary_Value_Index_To_Instance(
        unsigned index);
    unsigned Binary_Value_Instance_To_Index(
        uint32_t object_instance);

    bool Binary_Value_Object_Name(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING *object_name);

    void Binary_Value_Init(
        void);

    int Binary_Value_Read_Property(
        BACNET_READ_PROPERTY_DATA * rpdata);

    bool Binary_Value_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data);

#ifdef TEST
#include "ctest.h"
    void testBinary_Value(
        Test * pTest);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#define BINARY_VALUE_OBJ_FUNCTIONS \
    OBJECT_BINARY_VALUE, Binary_Value_Init, Binary_Value_Count, \
    Binary_Value_Index_To_Instance, Binary_Value_Valid_Instance, \
    Binary_Value_Object_Name, Binary_Value_Read_Property, \
    Binary_Value_Write_Property, Binary_Value_Property_Lists, NULL, \
    NULL
#endif
