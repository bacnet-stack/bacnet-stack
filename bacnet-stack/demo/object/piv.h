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
#ifndef PIV_H
#define PIV_H

#include <stdbool.h>
#include <stdint.h>
#include "bacdef.h"
#include "bacerror.h"
#include "wp.h"
#include "rp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    typedef struct positiveinteger_value_descr {
        bool Out_Of_Service:1;
        uint32_t Present_Value;
        uint16_t Units;
    } POSITIVEINTEGER_VALUE_DESCR;


    void PositiveInteger_Value_Property_Lists(const int **pRequired,
        const int **pOptional,
        const int **pProprietary);
    bool PositiveInteger_Value_Valid_Instance(uint32_t object_instance);
    unsigned PositiveInteger_Value_Count(void);
    uint32_t PositiveInteger_Value_Index_To_Instance(unsigned index);
    unsigned PositiveInteger_Value_Instance_To_Index(uint32_t object_instance);

    bool PositiveInteger_Value_Object_Name(uint32_t object_instance,
        BACNET_CHARACTER_STRING * object_name);

    int PositiveInteger_Value_Read_Property(BACNET_READ_PROPERTY_DATA *
        rpdata);

    bool PositiveInteger_Value_Write_Property(BACNET_WRITE_PROPERTY_DATA *
        wp_data);

    bool PositiveInteger_Value_Present_Value_Set(uint32_t object_instance,
        uint32_t value,
        uint8_t priority);
    uint32_t PositiveInteger_Value_Present_Value(uint32_t object_instance);

    bool PositiveInteger_Value_Change_Of_Value(uint32_t instance);
    void PositiveInteger_Value_Change_Of_Value_Clear(uint32_t instance);
    bool PositiveInteger_Value_Encode_Value_List(uint32_t object_instance,
        BACNET_PROPERTY_VALUE * value_list);

    char *PositiveInteger_Value_Description(uint32_t instance);
    bool PositiveInteger_Value_Description_Set(uint32_t instance,
        char *new_name);

    bool PositiveInteger_Value_Out_Of_Service(uint32_t instance);
    void PositiveInteger_Value_Out_Of_Service_Set(uint32_t instance,
        bool oos_flag);

    /* note: header of Intrinsic_Reporting function is required
       even when INTRINSIC_REPORTING is not defined */
    void PositiveInteger_Value_Intrinsic_Reporting(uint32_t object_instance);

    void PositiveInteger_Value_Init(void);

#ifdef TEST
#include "ctest.h"
    void testPositiveInteger_Value(Test * pTest);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
