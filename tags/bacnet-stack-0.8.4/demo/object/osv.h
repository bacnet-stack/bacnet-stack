/**************************************************************************
*
* Copyright (C) 2015 Nikola Jelic <nikola.jelic@euroicc.com>
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
#ifndef OSV_H
#define OSV_H

#include <stdbool.h>
#include <stdint.h>
#include "bacdef.h"
#include "bacerror.h"
#include "wp.h"
#include "rp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    typedef struct octetstring_value_descr {
        unsigned Event_State:3;
        bool Out_Of_Service;
        BACNET_OCTET_STRING Present_Value;
    } OCTETSTRING_VALUE_DESCR;


    void OctetString_Value_Property_Lists(const int **pRequired,
        const int **pOptional,
        const int **pProprietary);
    bool OctetString_Value_Valid_Instance(uint32_t object_instance);
    unsigned OctetString_Value_Count(void);
    uint32_t OctetString_Value_Index_To_Instance(unsigned index);
    unsigned OctetString_Value_Instance_To_Index(uint32_t object_instance);

    bool OctetString_Value_Object_Name(uint32_t object_instance,
        BACNET_CHARACTER_STRING * object_name);

    int OctetString_Value_Read_Property(BACNET_READ_PROPERTY_DATA * rpdata);

    bool OctetString_Value_Write_Property(BACNET_WRITE_PROPERTY_DATA *
        wp_data);

    bool OctetString_Value_Present_Value_Set(uint32_t object_instance,
        BACNET_OCTET_STRING * value,
        uint8_t priority);
    BACNET_OCTET_STRING *OctetString_Value_Present_Value(uint32_t
        object_instance);

    bool OctetString_Value_Change_Of_Value(uint32_t instance);
    void OctetString_Value_Change_Of_Value_Clear(uint32_t instance);
    bool OctetString_Value_Encode_Value_List(uint32_t object_instance,
        BACNET_PROPERTY_VALUE * value_list);

    char *OctetString_Value_Description(uint32_t instance);
    bool OctetString_Value_Description_Set(uint32_t instance,
        char *new_name);

    bool OctetString_Value_Out_Of_Service(uint32_t instance);
    void OctetString_Value_Out_Of_Service_Set(uint32_t instance,
        bool oos_flag);

    /* note: header of Intrinsic_Reporting function is required
       even when INTRINSIC_REPORTING is not defined */
    void OctetString_Value_Intrinsic_Reporting(uint32_t object_instance);

    void OctetString_Value_Init(void);

#ifdef TEST
#include "ctest.h"
    void testOctetString_Value(Test * pTest);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
