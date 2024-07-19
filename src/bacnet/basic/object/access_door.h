/**
 * @file
 * @brief API for basic BACnet Access Door Objects implementation.
 * @author Nikola Jelic <nikola.jelic@euroicc.com>
 * @date 2015
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_ACCESS_DOOR_H
#define BACNET_BASIC_OBJECT_ACCESS_DOOR_H

#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacerror.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"


#ifndef MAX_ACCESS_DOORS
#define MAX_ACCESS_DOORS 4
#endif


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    typedef struct {
        bool value_active[BACNET_MAX_PRIORITY];
        BACNET_DOOR_VALUE priority_array[BACNET_MAX_PRIORITY];
        BACNET_DOOR_VALUE relinquish_default;
        BACNET_EVENT_STATE event_state;
        BACNET_RELIABILITY reliability;
        bool out_of_service;
        BACNET_DOOR_STATUS door_status;
        BACNET_LOCK_STATUS lock_status;
        BACNET_DOOR_SECURED_STATUS secured_status;
        uint32_t door_pulse_time, door_extended_pulse_time,
            door_unlock_delay_time, door_open_too_long_time;
        BACNET_DOOR_ALARM_STATE door_alarm_state;
    } ACCESS_DOOR_DESCR;

    BACNET_STACK_EXPORT
    void Access_Door_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);
    BACNET_STACK_EXPORT
    bool Access_Door_Valid_Instance(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    unsigned Access_Door_Count(
        void);
    BACNET_STACK_EXPORT
    uint32_t Access_Door_Index_To_Instance(
        unsigned index);
    BACNET_STACK_EXPORT
    unsigned Access_Door_Instance_To_Index(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Access_Door_Object_Instance_Add(
        uint32_t instance);

    BACNET_STACK_EXPORT
    BACNET_DOOR_VALUE Access_Door_Present_Value(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    unsigned Access_Door_Present_Value_Priority(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Access_Door_Present_Value_Set(
        uint32_t object_instance,
        BACNET_DOOR_VALUE value,
        unsigned priority);
    BACNET_STACK_EXPORT
    bool Access_Door_Present_Value_Relinquish(
        uint32_t object_instance,
        unsigned priority);

    BACNET_STACK_EXPORT
    BACNET_DOOR_VALUE Access_Door_Relinquish_Default(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Access_Door_Relinquish_Default_Set(
        uint32_t object_instance,
        float value);

    BACNET_STACK_EXPORT
    bool Access_Door_Change_Of_Value(
        uint32_t instance);
    BACNET_STACK_EXPORT
    void Access_Door_Change_Of_Value_Clear(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Access_Door_Encode_Value_List(
        uint32_t object_instance,
        BACNET_PROPERTY_VALUE * value_list);

    BACNET_STACK_EXPORT
    bool Access_Door_Object_Name(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING * object_name);
    BACNET_STACK_EXPORT
    bool Access_Door_Name_Set(
        uint32_t object_instance,
        char *new_name);

    BACNET_STACK_EXPORT
    char *Access_Door_Description(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Access_Door_Description_Set(
        uint32_t instance,
        char *new_name);

    BACNET_STACK_EXPORT
    bool Access_Door_Out_Of_Service(
        uint32_t instance);
    BACNET_STACK_EXPORT
    void Access_Door_Out_Of_Service_Set(
        uint32_t instance,
        bool oos_flag);

    BACNET_STACK_EXPORT
    int Access_Door_Read_Property(
        BACNET_READ_PROPERTY_DATA * rpdata);
    BACNET_STACK_EXPORT
    bool Access_Door_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data);

    BACNET_STACK_EXPORT
    uint32_t Access_Door_Create(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Access_Door_Delete(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    void Access_Door_Cleanup(
        void);
    BACNET_STACK_EXPORT
    void Access_Door_Init(
        void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
