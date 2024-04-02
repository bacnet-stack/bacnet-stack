/**
 * @file
 * @author Steve Karg
 * @date 2006
 * @brief Binary Value object is an input object with a present-value that
 * uses an enumerated two state active/inactive data type.
 * @section LICENSE
 * Copyright (C) 2006 Steve Karg <skarg@users.sourceforge.net>
 * SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BINARY_VALUE_OBJECT_H
#define BACNET_BINARY_VALUE_OBJECT_H
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacerror.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"

/**
 * @brief Callback for gateway write present value request
 * @param  object_instance - object-instance number of the object
 * @param  old_value - binary preset-value prior to write
 * @param  value - binary preset-value of the write
 */
typedef void (*binary_value_write_present_value_callback)(
    uint32_t object_instance, BACNET_BINARY_PV old_value,
    BACNET_BINARY_PV value);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    void Binary_Value_Init(
        void);

    BACNET_STACK_EXPORT
    void Binary_Value_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);

    BACNET_STACK_EXPORT
    bool Binary_Value_Valid_Instance(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    unsigned Binary_Value_Count(
        void);
    BACNET_STACK_EXPORT
    uint32_t Binary_Value_Index_To_Instance(
        unsigned index);
    BACNET_STACK_EXPORT
    unsigned Binary_Value_Instance_To_Index(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Binary_Value_Object_Instance_Add(
        uint32_t instance);

    BACNET_STACK_EXPORT
    bool Binary_Value_Object_Name(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING * object_name);
    BACNET_STACK_EXPORT
    bool Binary_Value_Name_Set(
        uint32_t object_instance,
        char *new_name);
    BACNET_STACK_EXPORT
    char *Binary_Value_Name(
        uint32_t object_instance);

    BACNET_STACK_EXPORT
    char *Binary_Value_Description(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Binary_Value_Description_Set(
        uint32_t instance,
        char *new_name);

    BACNET_STACK_EXPORT
    BACNET_RELIABILITY Binary_Value_Reliability(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Binary_Value_Reliability_Set(
        uint32_t object_instance,
        BACNET_RELIABILITY value);

    BACNET_STACK_EXPORT
    char *Binary_Value_Inactive_Text(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Binary_Value_Inactive_Text_Set(
        uint32_t instance,
        char *new_name);
    BACNET_STACK_EXPORT
    char *Binary_Value_Active_Text(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Binary_Value_Active_Text_Set(
        uint32_t instance,
        char *new_name);

    BACNET_STACK_EXPORT
    int Binary_Value_Read_Property(
        BACNET_READ_PROPERTY_DATA * rpdata);

    BACNET_STACK_EXPORT
    bool Binary_Value_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data);

    BACNET_STACK_EXPORT
    bool Binary_Value_Encode_Value_List(
        uint32_t object_instance,
        BACNET_PROPERTY_VALUE * value_list);
    BACNET_STACK_EXPORT
    bool Binary_Value_Change_Of_Value(
        uint32_t instance);
    BACNET_STACK_EXPORT
    void Binary_Value_Change_Of_Value_Clear(
        uint32_t instance);

    BACNET_STACK_EXPORT
    BACNET_BINARY_PV Binary_Value_Present_Value(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Binary_Value_Present_Value_Set(
        uint32_t instance,
        BACNET_BINARY_PV value);

    BACNET_STACK_EXPORT
    bool Binary_Value_Out_Of_Service(
        uint32_t instance);
    BACNET_STACK_EXPORT
    void Binary_Value_Out_Of_Service_Set(
        uint32_t instance,
        bool value);

    BACNET_STACK_EXPORT
    char *Binary_Value_Description(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Binary_Value_Description_Set(
        uint32_t object_instance,
        char *text_string);

    BACNET_STACK_EXPORT
    char *Binary_Value_Inactive_Text(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Binary_Value_Inactive_Text_Set(
        uint32_t instance,
        char *new_name);
    BACNET_STACK_EXPORT
    char *Binary_Value_Active_Text(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Binary_Value_Active_Text_Set(
        uint32_t instance,
        char *new_name);

    BACNET_STACK_EXPORT
    BACNET_POLARITY Binary_Value_Polarity(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Binary_Value_Polarity_Set(
        uint32_t object_instance,
        BACNET_POLARITY polarity);
    BACNET_STACK_EXPORT
    uint32_t Binary_Value_Create(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Binary_Value_Delete(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    void Binary_Value_Cleanup(
        void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
