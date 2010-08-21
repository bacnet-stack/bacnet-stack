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
#ifndef MS_INPUT_H
#define MS_INPUT_H

#include <stdbool.h>
#include <stdint.h>
#include "bacdef.h"
#include "bacerror.h"
#include "rp.h"
#include "wp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    void Multistate_Input_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);

    bool Multistate_Input_Valid_Instance(
        struct bacnet_session_object *sess,
        uint32_t object_instance);
    unsigned Multistate_Input_Count(
        struct bacnet_session_object *sess);
    uint32_t Multistate_Input_Index_To_Instance(
        struct bacnet_session_object *sess,
        unsigned index);
    unsigned Multistate_Input_Instance_To_Index(
        struct bacnet_session_object *sess,
        uint32_t instance);

    int Multistate_Input_Read_Property(
        struct bacnet_session_object *sess,
        BACNET_READ_PROPERTY_DATA * rpdata);

    bool Multistate_Input_Write_Property(
        struct bacnet_session_object *sess,
        BACNET_WRITE_PROPERTY_DATA * wp_data);

    /* optional API */
    bool Multistate_Input_Object_Instance_Add(
        struct bacnet_session_object *sess,
        uint32_t instance);
    char *Multistate_Input_Name(
        struct bacnet_session_object *sess,
        uint32_t object_instance);
    bool Multistate_Input_Name_Set(
        struct bacnet_session_object *sess,
        uint32_t object_instance,
        char *new_name);
    uint32_t Multistate_Input_Present_Value(
        struct bacnet_session_object *sess,
        uint32_t object_instance);
    bool Multistate_Input_Present_Value_Set(
        struct bacnet_session_object *sess,
        uint32_t object_instance,
        uint32_t value);
    bool Multistate_Input_Description_Set(
        struct bacnet_session_object *sess,
        uint32_t object_instance,
        char *text_string);
    bool Multistate_Input_State_Text_Set(
        struct bacnet_session_object *sess,
        uint32_t object_instance,
        uint32_t state_index,
        char *new_name);
    bool Multistate_Input_Max_States_Set(
        struct bacnet_session_object *sess,
        uint32_t instance,
        uint32_t max_states_requested);

    void Multistate_Input_Init(
        struct bacnet_session_object *sess);


#ifdef TEST
#include "ctest.h"
    void testMultistateOutput(
        Test * pTest);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
