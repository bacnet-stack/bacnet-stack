/**
 * @file
 * @brief API for basic BACnet Access Point Objects implementation.
 * @author Nikola Jelic <nikola.jelic@euroicc.com>
 * @date 2015
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_ACCESS_POINT_H
#define BACNET_BASIC_OBJECT_ACCESS_POINT_H

#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
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
    uint32_t Access_Point_Create(
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

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
