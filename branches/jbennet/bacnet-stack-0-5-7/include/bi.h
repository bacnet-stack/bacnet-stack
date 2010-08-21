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
#include "rp.h"
#include "wp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    void Binary_Input_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);

    bool Binary_Input_Valid_Instance(
        struct bacnet_session_object *sess,
        uint32_t object_instance);
    unsigned Binary_Input_Count(
        struct bacnet_session_object *sess);
    uint32_t Binary_Input_Index_To_Instance(
        struct bacnet_session_object *sess,
        unsigned index);
    unsigned Binary_Input_Instance_To_Index(
        struct bacnet_session_object *sess,
        uint32_t instance);
    bool Binary_Input_Object_Instance_Add(
        struct bacnet_session_object *sess,
        uint32_t instance);

    char *Binary_Input_Name(
        struct bacnet_session_object *sess,
        uint32_t object_instance);
    bool Binary_Input_Name_Set(
        struct bacnet_session_object *sess,
        uint32_t object_instance,
        char *new_name);

    char *Binary_Input_Description(
        struct bacnet_session_object *sess,
        uint32_t instance);
    bool Binary_Input_Description_Set(
        struct bacnet_session_object *sess,
        uint32_t instance,
        char *new_name);

    char *Binary_Input_Inactive_Text(
        struct bacnet_session_object *sess,
        uint32_t instance);
    bool Binary_Input_Inactive_Text_Set(
        struct bacnet_session_object *sess,
        uint32_t instance,
        char *new_name);
    char *Binary_Input_Active_Text(
        struct bacnet_session_object *sess,
        uint32_t instance);
    bool Binary_Input_Active_Text_Set(
        struct bacnet_session_object *sess,
        uint32_t instance,
        char *new_name);
    BACNET_POLARITY Binary_Input_Polarity(
        struct bacnet_session_object *sess,
        uint32_t object_instance);
    bool Binary_Input_Polarity_Set(
        struct bacnet_session_object *sess,
        uint32_t object_instance,
        BACNET_POLARITY polarity);
    bool Binary_Input_Out_Of_Service(
        struct bacnet_session_object *sess,
        uint32_t object_instance);

    bool Binary_Input_Change_Of_Value(
        struct bacnet_session_object *sess,
        uint32_t object_instance);
    void Binary_Input_Change_Of_Value_Clear(
        struct bacnet_session_object *sess,
        uint32_t object_instance);
    bool Binary_Input_Encode_Value_List(
        struct bacnet_session_object *sess,
        uint32_t object_instance,
        BACNET_PROPERTY_VALUE * value_list);

    int Binary_Input_Read_Property(
        struct bacnet_session_object *sess,
        BACNET_READ_PROPERTY_DATA * rpdata);

    bool Binary_Input_Write_Property(
        struct bacnet_session_object *sess,
        BACNET_WRITE_PROPERTY_DATA * wp_data);
    void Binary_Input_Init(
        struct bacnet_session_object *sess);
    BACNET_BINARY_PV Binary_Input_Present_Value(
        struct bacnet_session_object *sess,
        uint32_t object_instance);

    bool Binary_Input_Present_Value_Set(
        struct bacnet_session_object *sess,
        uint32_t object_instance,
        BACNET_BINARY_PV value);

#ifdef TEST
#include "ctest.h"
    void testBinaryInput(
        Test * pTest);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
