/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date March 2024
 * @brief API for a basic BACnet Life Safety Zone object type implementation.
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_LIFE_SAFETY_ZONE_H
#define BACNET_BASIC_OBJECT_LIFE_SAFETY_ZONE_H
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
    void Life_Safety_Zone_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);
    BACNET_STACK_EXPORT
    bool Life_Safety_Zone_Valid_Instance(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    unsigned Life_Safety_Zone_Count(
        void);
    BACNET_STACK_EXPORT
    uint32_t Life_Safety_Zone_Index_To_Instance(
        unsigned index);
    BACNET_STACK_EXPORT
    unsigned Life_Safety_Zone_Instance_To_Index(
        uint32_t object_instance);

    BACNET_STACK_EXPORT
    bool Life_Safety_Zone_Object_Name(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING * object_name);
    BACNET_STACK_EXPORT
    bool Life_Safety_Zone_Name_Set(
        uint32_t object_instance,
        const char *new_name);
    BACNET_STACK_EXPORT
    const char *Life_Safety_Zone_Name_ASCII(
        uint32_t object_instance);

    BACNET_STACK_EXPORT
    BACNET_LIFE_SAFETY_STATE Life_Safety_Zone_Present_Value(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Life_Safety_Zone_Present_Value_Set(
        uint32_t object_instance,
        BACNET_LIFE_SAFETY_STATE present_value);

    BACNET_STACK_EXPORT
    BACNET_SILENCED_STATE Life_Safety_Zone_Silenced(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Life_Safety_Zone_Silenced_Set(
        uint32_t object_instance,
        BACNET_SILENCED_STATE value);
    BACNET_STACK_EXPORT
    BACNET_LIFE_SAFETY_MODE Life_Safety_Zone_Mode(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Life_Safety_Zone_Mode_Set(
        uint32_t object_instance,
        BACNET_LIFE_SAFETY_MODE value);
    BACNET_STACK_EXPORT
    BACNET_LIFE_SAFETY_OPERATION Life_Safety_Zone_Operation_Expected(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Life_Safety_Zone_Operation_Expected_Set(
        uint32_t object_instance,
        BACNET_LIFE_SAFETY_OPERATION value);

    BACNET_STACK_EXPORT
    bool Life_Safety_Zone_Out_Of_Service(
        uint32_t instance);
    BACNET_STACK_EXPORT
    void Life_Safety_Zone_Out_Of_Service_Set(
        uint32_t instance,
        bool oos_flag);

    BACNET_STACK_EXPORT
    BACNET_RELIABILITY Life_Safety_Zone_Reliability(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Life_Safety_Zone_Reliability_Set(
        uint32_t object_instance, BACNET_RELIABILITY value);

    BACNET_STACK_EXPORT
    bool Life_Safety_Zone_Members_Add(
        uint32_t object_instance,
        BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *data);
    BACNET_STACK_EXPORT
    void Life_Safety_Zone_Members_Clear(
        uint32_t object_instance);

    BACNET_STACK_EXPORT
    bool Life_Safety_Zone_Maintenance_Required(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    void Life_Safety_Zone_Maintenance_Required_Set(
        uint32_t object_instance,
        bool value);

    BACNET_STACK_EXPORT
    int Life_Safety_Zone_Read_Property(
        BACNET_READ_PROPERTY_DATA * rpdata);

    BACNET_STACK_EXPORT
    bool Life_Safety_Zone_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data);

    BACNET_STACK_EXPORT
    uint32_t Life_Safety_Zone_Create(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Life_Safety_Zone_Delete(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    void Life_Safety_Zone_Cleanup(
        void);
    BACNET_STACK_EXPORT
    void Life_Safety_Zone_Init(
        void);


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
