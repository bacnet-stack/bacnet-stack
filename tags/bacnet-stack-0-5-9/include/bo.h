/**************************************************************************
*
* Copyright (C) 2005 Steve Karg <skarg@users.sourceforge.net>
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
#ifndef BO_H
#define BO_H

#include <stdbool.h>
#include <stdint.h>
#include "bacdef.h"
#include "bacerror.h"
#include "rp.h"
#include "wp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    void Binary_Output_Init(
        void);

    void Binary_Output_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);

    bool Binary_Output_Valid_Instance(
        uint32_t object_instance);
    unsigned Binary_Output_Count(
        void);
    uint32_t Binary_Output_Index_To_Instance(
        unsigned index);
    unsigned Binary_Output_Instance_To_Index(
        uint32_t instance);
    bool Binary_Output_Object_Instance_Add(
        uint32_t instance);

    char *Binary_Output_Name(
        uint32_t object_instance);
    bool Binary_Output_Name_Set(
        uint32_t object_instance,
        char *new_name);

    char *Binary_Output_Description(
        uint32_t instance);
    bool Binary_Output_Description_Set(
        uint32_t instance,
        char *new_name);

    char *Binary_Output_Inactive_Text(
        uint32_t instance);
    bool Binary_Output_Inactive_Text_Set(
        uint32_t instance,
        char *new_name);
    char *Binary_Output_Active_Text(
        uint32_t instance);
    bool Binary_Output_Active_Text_Set(
        uint32_t instance,
        char *new_name);

    int Binary_Output_Read_Property(
        BACNET_READ_PROPERTY_DATA * rpdata);

    bool Binary_Output_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data);

    BACNET_BINARY_PV Binary_Output_Present_Value(
        uint32_t instance);
    bool Binary_Output_Present_Value_Set(
        uint32_t instance,
        BACNET_BINARY_PV binary_value,
        unsigned priority);
    bool Binary_Output_Present_Value_Relinquish(
        uint32_t instance,
        unsigned priority);
    BACNET_POLARITY Binary_Output_Polarity(
        uint32_t instance);
    bool Binary_Output_Out_Of_Service(
        uint32_t instance);


#ifdef TEST
#include "ctest.h"
    void testBinaryOutput(
        Test * pTest);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#define BINARY_OUTPUT_OBJ_FUNCTIONS \
    OBJECT_BINARY_OUTPUT, Binary_Output_Init, Binary_Output_Count, \
    Binary_Output_Index_To_Instance, Binary_Output_Valid_Instance, \
    Binary_Output_Name, Binary_Output_Read_Property, \
    Binary_Output_Write_Property, Binary_Output_Property_Lists, \
    NULL, NULL
#endif
