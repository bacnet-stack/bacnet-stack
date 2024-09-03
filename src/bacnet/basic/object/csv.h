/**************************************************************************
*
* Copyright (C) 2012 Steve Karg <skarg@users.sourceforge.net>
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
#ifndef CSV_H
#define CSV_H

#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacerror.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    void CharacterString_Value_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);

    BACNET_STACK_EXPORT
    bool CharacterString_Value_Valid_Instance(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    unsigned CharacterString_Value_Count(
        void);
    BACNET_STACK_EXPORT
    uint32_t CharacterString_Value_Index_To_Instance(
        unsigned index);
    BACNET_STACK_EXPORT
    unsigned CharacterString_Value_Instance_To_Index(
        uint32_t instance);

    BACNET_STACK_EXPORT
    int CharacterString_Value_Read_Property(
        BACNET_READ_PROPERTY_DATA * rpdata);

    BACNET_STACK_EXPORT
    bool CharacterString_Value_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data);

    /* optional API */
    BACNET_STACK_EXPORT
    bool CharacterString_Value_Object_Instance_Add(
        uint32_t instance);

    BACNET_STACK_EXPORT
    bool CharacterString_Value_Object_Name(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING * object_name);
    BACNET_STACK_EXPORT
    bool CharacterString_Value_Name_Set(
        uint32_t object_instance,
        char *new_name);

    BACNET_STACK_EXPORT
    bool CharacterString_Value_Present_Value(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);
    BACNET_STACK_EXPORT
    bool CharacterString_Value_Present_Value_Set(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING * value);
    BACNET_STACK_EXPORT
    bool CharacterString_Value_Present_Value_Backup_Set(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING *present_value);
    BACNET_STACK_EXPORT
    bool CharacterString_Value_Description_Set(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING *text_string);
    BACNET_STACK_EXPORT
    bool CharacterString_Value_Out_Of_Service(
        uint32_t object_instance);

    BACNET_STACK_EXPORT
    bool CharacterString_Value_Change_Of_Value(
        uint32_t instance);
    BACNET_STACK_EXPORT
    void CharacterString_Value_Change_Of_Value_Clear(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool CharacterString_Value_Encode_Value_List(
        uint32_t object_instance,
        BACNET_PROPERTY_VALUE * value_list);

    BACNET_STACK_EXPORT
    void CharacterString_Value_Init(
        void);
    BACNET_STACK_EXPORT
    bool CharacterString_Value_Set(BACNET_OBJECT_LIST_INIT_T *pInit_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
