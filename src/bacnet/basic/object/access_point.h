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
#ifndef ACCESS_POINT_H
#define ACCESS_POINT_H

#include <stdbool.h>
#include <stdint.h>
#include "bacnet/bacnet_stack_exports.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacerror.h"
#include "bacnet/timestamp.h"
#include "bacnet/bacdevobjpropref.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"


#ifndef MAX_ACCESS_POINTS
#define MAX_ACCESS_POINTS 4
#endif

#ifndef MAX_ACCESS_DOORS_COUNT
#define MAX_ACCESS_DOORS_COUNT 4
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    typedef struct {
        BACNET_EVENT_STATE event_state;
        BACNET_RELIABILITY reliability;
        bool out_of_service;
        BACNET_AUTHENTICATION_STATUS authentication_status;
        uint32_t active_authentication_policy,
            number_of_authentication_policies;
        BACNET_AUTHORIZATION_MODE authorization_mode;
        BACNET_ACCESS_EVENT access_event;
        uint32_t access_event_tag;
        BACNET_TIMESTAMP access_event_time;
        BACNET_DEVICE_OBJECT_REFERENCE access_event_credential;
        uint32_t num_doors;     /* helper value, not a property */
        BACNET_DEVICE_OBJECT_REFERENCE access_doors[MAX_ACCESS_DOORS_COUNT];
        uint8_t priority_for_writing;
    } ACCESS_POINT_DESCR;

    BACNET_STACK_EXPORT
    void Access_Point_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);
    BACNET_STACK_EXPORT
    bool Access_Point_Valid_Instance(
        uint32_t object_instance);
    unsigned Access_Point_Count(
        void);
    BACNET_STACK_EXPORT
    uint32_t Access_Point_Index_To_Instance(
        unsigned index);
    BACNET_STACK_EXPORT
    unsigned Access_Point_Instance_To_Index(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Access_Point_Object_Instance_Add(
        uint32_t instance);


    BACNET_STACK_EXPORT
    bool Access_Point_Object_Name(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING * object_name);
    BACNET_STACK_EXPORT
    bool Access_Point_Name_Set(
        uint32_t object_instance,
        char *new_name);


    BACNET_STACK_EXPORT
    bool Access_Point_Out_Of_Service(
        uint32_t instance);
    BACNET_STACK_EXPORT
    void Access_Point_Out_Of_Service_Set(
        uint32_t instance,
        bool oos_flag);

    BACNET_STACK_EXPORT
    int Access_Point_Read_Property(
        BACNET_READ_PROPERTY_DATA * rpdata);
    BACNET_STACK_EXPORT
    bool Access_Point_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data);

    BACNET_STACK_EXPORT
    bool Access_Point_Create(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Access_Point_Delete(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    void Access_Point_Cleanup(
        void);
    BACNET_STACK_EXPORT
    void Access_Point_Init(
        void);

#ifdef TEST
#include "ctest.h"
    BACNET_STACK_EXPORT
    void testAccessPoint(
        Test * pTest);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
