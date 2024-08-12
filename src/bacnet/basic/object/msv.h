/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2009
 * @brief Multi-State object is an object with a present-value that
 * uses an integer data type with a sequence of 1 to N values.
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_MULTI_STATE_VALUE_H
#define BACNET_BASIC_OBJECT_MULTI_STATE_VALUE_H
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
 * @param  old_value - multistate preset-value prior to write
 * @param  value - multistate preset-value of the write
 */
typedef void (*multistate_value_write_present_value_callback)(
    uint32_t object_instance, uint32_t old_value,
    uint32_t value);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    void Multistate_Value_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);

    BACNET_STACK_EXPORT
    bool Multistate_Value_Valid_Instance(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    unsigned Multistate_Value_Count(
        void);
    BACNET_STACK_EXPORT
    uint32_t Multistate_Value_Index_To_Instance(
        unsigned index);
    BACNET_STACK_EXPORT
    unsigned Multistate_Value_Instance_To_Index(
        uint32_t instance);

    BACNET_STACK_EXPORT
    int Multistate_Value_Read_Property(
        BACNET_READ_PROPERTY_DATA * rpdata);

    BACNET_STACK_EXPORT
    bool Multistate_Value_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data);

    /* optional API */
    bool Multistate_Value_Object_Instance_Add(
        uint32_t instance);

    BACNET_STACK_EXPORT
    bool Multistate_Value_Object_Name(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING * object_name);
    BACNET_STACK_EXPORT
    bool Multistate_Value_Name_Set(
        uint32_t object_instance,
        char *new_name);

    BACNET_STACK_EXPORT
    uint32_t Multistate_Value_Present_Value(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Multistate_Value_Present_Value_Set(
        uint32_t object_instance,
        uint32_t value);

    BACNET_STACK_EXPORT
    bool Multistate_Value_Change_Of_Value(
        uint32_t instance);
    BACNET_STACK_EXPORT
    void Multistate_Value_Change_Of_Value_Clear(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Multistate_Value_Encode_Value_List(
        uint32_t object_instance,
        BACNET_PROPERTY_VALUE * value_list);

    BACNET_STACK_EXPORT
    bool Multistate_Value_Out_Of_Service(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    void Multistate_Value_Out_Of_Service_Set(
        uint32_t object_instance,
        bool value);

    BACNET_STACK_EXPORT
    char *Multistate_Value_Description(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Multistate_Value_Description_Set(
        uint32_t object_instance,
        char *text_string);

    BACNET_STACK_EXPORT
    bool Multistate_Value_State_Text_List_Set(
        uint32_t object_instance,
        const char *state_text_list);
    BACNET_STACK_EXPORT
    bool Multistate_Value_State_Text_Set(
        uint32_t object_instance,
        uint32_t state_index,
        char *new_name);
    BACNET_STACK_EXPORT
    bool Multistate_Value_Max_States_Set(
        uint32_t instance,
        uint32_t max_states_requested);
    BACNET_STACK_EXPORT
    uint32_t Multistate_Value_Max_States(
        uint32_t instance);
    BACNET_STACK_EXPORT
    char *Multistate_Value_State_Text(
        uint32_t object_instance,
        uint32_t state_index);

    BACNET_STACK_EXPORT
    uint32_t Multistate_Value_Create(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Multistate_Value_Delete(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    void Multistate_Value_Cleanup(
        void);

    BACNET_STACK_EXPORT
    void Multistate_Value_Init(
        void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
