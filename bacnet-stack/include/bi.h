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
#ifndef BI_H
#define BI_H

#include <stdbool.h>
#include <stdint.h>
#include "bacdef.h"
#include "cov.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    void Binary_Input_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);
    bool Binary_Input_Valid_Instance(
        uint32_t object_instance);
    unsigned Binary_Input_Count(
        void);
    uint32_t Binary_Input_Index_To_Instance(
        unsigned index);
    char *Binary_Input_Name(
        uint32_t object_instance);

    bool Binary_Input_Change_Of_Value(
        uint32_t object_instance);
    void Binary_Input_Change_Of_Value_Clear(
        uint32_t object_instance);
    bool Binary_Input_Encode_Value_List(
        uint32_t object_instance,
        BACNET_PROPERTY_VALUE *value_list);

    int Binary_Input_Encode_Property_APDU(
        uint8_t * apdu,
        uint32_t object_instance,
        BACNET_PROPERTY_ID property,
        int32_t array_index,
        BACNET_ERROR_CLASS * error_class,
        BACNET_ERROR_CODE * error_code);

    bool Binary_Input_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data,
        BACNET_ERROR_CLASS * error_class,
        BACNET_ERROR_CODE * error_code);

#ifdef TEST
#include "ctest.h"
    void testBinaryInput(
        Test * pTest);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
