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
#ifndef ACCESS_ZONE_H
#define ACCESS_ZONE_H

#include <stdbool.h>
#include <stdint.h>
#include "bacdef.h"
#include "bacerror.h"
#include "bacdevobjpropref.h"
#include "rp.h"
#include "wp.h"


#ifndef MAX_ACCESS_ZONES
#define MAX_ACCESS_ZONES 4
#endif

#ifndef MAX_ACCESS_ZONE_ENTRY_POINTS
#define MAX_ACCESS_ZONE_ENTRY_POINTS 4
#endif

#ifndef MAX_ACCESS_ZONE_EXIT_POINTS
#define MAX_ACCESS_ZONE_EXIT_POINTS 4
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    typedef struct {
        uint32_t global_identifier;
        BACNET_ACCESS_ZONE_OCCUPANCY_STATE occupancy_state;
        BACNET_EVENT_STATE event_state;
        BACNET_RELIABILITY reliability;
        bool out_of_service;
        uint32_t entry_points_count, exit_points_count;
        BACNET_DEVICE_OBJECT_REFERENCE
            entry_points[MAX_ACCESS_ZONE_ENTRY_POINTS];
        BACNET_DEVICE_OBJECT_REFERENCE
            exit_points[MAX_ACCESS_ZONE_EXIT_POINTS];
    } ACCESS_ZONE_DESCR;

    void Access_Zone_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);
    bool Access_Zone_Valid_Instance(
        uint32_t object_instance);
    unsigned Access_Zone_Count(
        void);
    uint32_t Access_Zone_Index_To_Instance(
        unsigned index);
    unsigned Access_Zone_Instance_To_Index(
        uint32_t instance);
    bool Access_Zone_Object_Instance_Add(
        uint32_t instance);

    bool Access_Zone_Object_Name(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING * object_name);
    bool Access_Zone_Name_Set(
        uint32_t object_instance,
        char *new_name);

    bool Access_Zone_Out_Of_Service(
        uint32_t instance);
    void Access_Zone_Out_Of_Service_Set(
        uint32_t instance,
        bool oos_flag);

    int Access_Zone_Read_Property(
        BACNET_READ_PROPERTY_DATA * rpdata);
    bool Access_Zone_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data);

    bool Access_Zone_Create(
        uint32_t object_instance);
    bool Access_Zone_Delete(
        uint32_t object_instance);
    void Access_Zone_Cleanup(
        void);
    void Access_Zone_Init(
        void);

#ifdef TEST
#include "ctest.h"
    void testAccessZone(
        Test * pTest);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
