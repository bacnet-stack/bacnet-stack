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
#ifndef LSP_H
#define LSP_H

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
    void Life_Safety_Point_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);
    BACNET_STACK_EXPORT
    bool Life_Safety_Point_Valid_Instance(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    unsigned Life_Safety_Point_Count(
        void);
    BACNET_STACK_EXPORT
    uint32_t Life_Safety_Point_Index_To_Instance(
        unsigned index);
    BACNET_STACK_EXPORT
    unsigned Life_Safety_Point_Instance_To_Index(
        uint32_t object_instance);

    BACNET_STACK_EXPORT
    bool Life_Safety_Point_Object_Name(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING * object_name);
    BACNET_STACK_EXPORT
    bool Life_Safety_Point_Name_Set(
        uint32_t object_instance,
        char *new_name);


    BACNET_STACK_EXPORT
    BACNET_LIFE_SAFETY_STATE Life_Safety_Point_Present_Value(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Life_Safety_Point_Present_Value_Set(
        uint32_t object_instance, 
        BACNET_LIFE_SAFETY_STATE present_value);

    BACNET_STACK_EXPORT
    BACNET_SILENCED_STATE Life_Safety_Point_Silenced(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Life_Safety_Point_Silenced_Set(
        uint32_t object_instance, 
        BACNET_SILENCED_STATE value);
    BACNET_STACK_EXPORT
    BACNET_LIFE_SAFETY_MODE Life_Safety_Point_Mode(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Life_Safety_Point_Mode_Set(
        uint32_t object_instance, 
        BACNET_LIFE_SAFETY_MODE value);
    BACNET_STACK_EXPORT
    BACNET_LIFE_SAFETY_OPERATION Life_Safety_Point_Operation_Expected(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Life_Safety_Point_Operation_Expected_Set(
        uint32_t object_instance, 
        BACNET_LIFE_SAFETY_OPERATION value);

    BACNET_STACK_EXPORT
    bool Life_Safety_Point_Out_Of_Service(
        uint32_t instance);
    BACNET_STACK_EXPORT
    void Life_Safety_Point_Out_Of_Service_Set(
        uint32_t instance,
        bool oos_flag);

    BACNET_STACK_EXPORT
    BACNET_RELIABILITY Life_Safety_Point_Reliability(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Life_Safety_Point_Reliability_Set(
        uint32_t object_instance, BACNET_RELIABILITY value);

    BACNET_STACK_EXPORT
    int Life_Safety_Point_Read_Property(
        BACNET_READ_PROPERTY_DATA * rpdata);

    BACNET_STACK_EXPORT
    bool Life_Safety_Point_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data);

    BACNET_STACK_EXPORT
    uint32_t Life_Safety_Point_Create(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Life_Safety_Point_Delete(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    void Life_Safety_Point_Cleanup(
        void);
    BACNET_STACK_EXPORT
    void Life_Safety_Point_Init(
        void);


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
