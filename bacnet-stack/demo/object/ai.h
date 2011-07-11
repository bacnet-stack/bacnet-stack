/**************************************************************************
*
* Copyright (C) 2005 Steve Karg <skarg@users.sourceforge.net>
* Copyright (C) 2011 Krzysztof Malorny <malornykrzysztof@gmail.com>
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
#ifndef AI_H
#define AI_H

#include <stdbool.h>
#include <stdint.h>
#include "bacdef.h"
#include "rp.h"
#include "wp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    typedef struct analog_input_descr {
        uint8_t  Event_State  : 3;
        float    Present_Value;
        BACNET_RELIABILITY Reliability;
        bool     Out_Of_Service;
        uint8_t  Units;
#if defined(INTRINSIC_REPORTING)
        uint32_t Time_Delay;
        uint32_t Notification_Class;
        float    High_Limit;
        float    Low_Limit;
        float    Deadband;
        uint8_t  Limit_Enable : 2;
        uint8_t  Event_Enable : 3;
        uint8_t  Acked_Transitions : 3;
        uint8_t  Notify_Type  : 1;
        BACNET_DATE_TIME Event_Time_Stamps[MAX_BACNET_EVENT_TRANSITION];
        /* time to generate event notification */
        uint32_t Remaining_Time_Delay;
#endif
    } ANALOG_INPUT_DESCR;




    void Analog_Input_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);

    bool Analog_Input_Valid_Instance(
        uint32_t object_instance);
    unsigned Analog_Input_Count(
        void);
    uint32_t Analog_Input_Index_To_Instance(
        unsigned index);
    unsigned Analog_Input_Instance_To_Index(
        uint32_t instance);
    bool Analog_Input_Object_Instance_Add(
        uint32_t instance);

    bool Analog_Input_Object_Name(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING *object_name);
    bool Analog_Input_Name_Set(
        uint32_t object_instance,
        char *new_name);

    char *Analog_Input_Description(
        uint32_t instance);
    bool Analog_Input_Description_Set(
        uint32_t instance,
        char *new_name);

    bool Analog_Input_Units_Set(
        uint32_t instance,
        uint16_t units);
    uint16_t Analog_Input_Units(
        uint32_t instance);

    int Analog_Input_Read_Property(
        BACNET_READ_PROPERTY_DATA * rpdata);
    bool Analog_Input_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data);

    float Analog_Input_Present_Value(
        uint32_t object_instance);
    void Analog_Input_Present_Value_Set(
        uint32_t object_instance,
        float value);

    void Analog_Input_Intrinsic_Reporting(uint32_t object_instance);

    void Analog_Input_Init(
        void);

#ifdef TEST
#include "ctest.h"
    void testAnalogInput(
        Test * pTest);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#define ANALOG_INPUT_OBJ_FUNCTIONS \
    OBJECT_ANALOG_INPUT, Analog_Input_Init, Analog_Input_Count, \
    Analog_Input_Index_To_Instance, Analog_Input_Valid_Instance, \
    Analog_Input_Object_Name, Analog_Input_Read_Property, NULL, \
    Analog_Input_Property_Lists, NULL, NULL, NULL
#endif
