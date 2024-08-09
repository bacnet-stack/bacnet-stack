/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2006
 * @brief A basic BACnet Binary Input Object implementation.
 * Binary Input objects are input objects with a present-value that
 * uses an enumerated two state active/inactive data type.
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_BINARY_INPUT_H
#define BACNET_BASIC_OBJECT_BINARY_INPUT_H
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/cov.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"

#if (INTRINSIC_REPORTING)
#include "bacnet/basic/object/nc.h"
#include "bacnet/getevent.h"
#include "bacnet/alarm_ack.h"
#include "bacnet/get_alarm_sum.h"
#endif

/**
 * @brief Callback for gateway write present value request
 * @param  object_instance - object-instance number of the object
 * @param  old_value - binary preset-value prior to write
 * @param  value - binary preset-value of the write
 */
typedef void (*binary_input_write_present_value_callback)(
    uint32_t object_instance, BACNET_BINARY_PV old_value,
    BACNET_BINARY_PV value);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    void Binary_Input_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);

    BACNET_STACK_EXPORT
    bool Binary_Input_Valid_Instance(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    unsigned Binary_Input_Count(
        void);
    BACNET_STACK_EXPORT
    uint32_t Binary_Input_Index_To_Instance(
        unsigned index);
    BACNET_STACK_EXPORT
    unsigned Binary_Input_Instance_To_Index(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Binary_Input_Object_Instance_Add(
        uint32_t instance);

    BACNET_STACK_EXPORT
    bool Binary_Input_Object_Name(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING * object_name);
    BACNET_STACK_EXPORT
    bool Binary_Input_Name_Set(
        uint32_t object_instance,
        const char *new_name);
    BACNET_STACK_EXPORT
    const char *Binary_Input_Name_ASCII(
        uint32_t object_instance);

    BACNET_STACK_EXPORT
    BACNET_BINARY_PV Binary_Input_Present_Value(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Binary_Input_Present_Value_Set(
        uint32_t object_instance,
        BACNET_BINARY_PV value);

    BACNET_STACK_EXPORT
    const char *Binary_Input_Description(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Binary_Input_Description_Set(
        uint32_t instance,
        const char *new_name);

    BACNET_STACK_EXPORT
    BACNET_RELIABILITY Binary_Input_Reliability(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Binary_Input_Reliability_Set(
        uint32_t object_instance,
        BACNET_RELIABILITY value);

    BACNET_STACK_EXPORT
    const char *Binary_Input_Inactive_Text(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Binary_Input_Inactive_Text_Set(
        uint32_t instance,
        const char *new_name);

    BACNET_STACK_EXPORT
    const char *Binary_Input_Active_Text(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Binary_Input_Active_Text_Set(
        uint32_t instance,
        const char *new_name);

    BACNET_STACK_EXPORT
    BACNET_POLARITY Binary_Input_Polarity(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Binary_Input_Polarity_Set(
        uint32_t object_instance,
        BACNET_POLARITY polarity);

    BACNET_STACK_EXPORT
    bool Binary_Input_Out_Of_Service(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    void Binary_Input_Out_Of_Service_Set(
        uint32_t object_instance,
        bool value);

    BACNET_STACK_EXPORT
    unsigned Binary_Input_Event_State(
        uint32_t object_instance);

    BACNET_STACK_EXPORT
    bool Binary_Input_Encode_Value_List(
        uint32_t object_instance,
        BACNET_PROPERTY_VALUE * value_list);
    BACNET_STACK_EXPORT
    bool Binary_Input_Change_Of_Value(
        uint32_t instance);
    BACNET_STACK_EXPORT
    void Binary_Input_Change_Of_Value_Clear(
        uint32_t instance);

    BACNET_STACK_EXPORT
    int Binary_Input_Read_Property(
        BACNET_READ_PROPERTY_DATA * rpdata);
    BACNET_STACK_EXPORT
    bool Binary_Input_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data);
    BACNET_STACK_EXPORT
    void Binary_Input_Write_Present_Value_Callback_Set(
        binary_input_write_present_value_callback cb);

    BACNET_STACK_EXPORT
    bool Binary_Input_Write_Enabled(uint32_t instance);
    BACNET_STACK_EXPORT
    void Binary_Input_Write_Enable(uint32_t instance);
    BACNET_STACK_EXPORT
    void Binary_Input_Write_Disable(uint32_t instance);

    BACNET_STACK_EXPORT
    uint32_t Binary_Input_Create(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Binary_Input_Delete(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    void Binary_Input_Cleanup(
        void);
    BACNET_STACK_EXPORT
    void Binary_Input_Init(
        void);

#if defined(INTRINSIC_REPORTING) && (BINARY_INPUT_INTRINSIC_REPORTING)
    BACNET_STACK_EXPORT
    bool Binary_Input_Event_Detection_Enable(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Binary_Input_Event_Detection_Enable_Set(
        uint32_t object_instance, bool value);

    BACNET_STACK_EXPORT
    int Binary_Input_Event_Information(
        unsigned index,
        BACNET_GET_EVENT_INFORMATION_DATA * getevent_data);

    BACNET_STACK_EXPORT
    int Binary_Input_Alarm_Ack(
        BACNET_ALARM_ACK_DATA * alarmack_data,
        BACNET_ERROR_CODE * error_code);

    BACNET_STACK_EXPORT
    int Binary_Input_Alarm_Summary(
        unsigned index,
        BACNET_GET_ALARM_SUMMARY_DATA * getalarm_data);

    BACNET_STACK_EXPORT
    uint32_t Binary_Input_Time_Delay(
            uint32_t object_instance);

    BACNET_STACK_EXPORT
    bool Binary_Input_Time_Delay_Set(
            uint32_t object_instance,
            uint32_t time_delay);

    BACNET_STACK_EXPORT
    uint32_t Binary_Input_Notification_Class(
            uint32_t object_instance);

    BACNET_STACK_EXPORT
    bool Binary_Input_Notification_Class_Set(
            uint32_t object_instance,
            uint32_t notification_class);

    BACNET_STACK_EXPORT
    BACNET_BINARY_PV Binary_Input_Alarm_Value(
            uint32_t object_instance);

    BACNET_STACK_EXPORT
    bool Binary_Input_Alarm_Value_Set(
        uint32_t object_instance, BACNET_BINARY_PV value);

    BACNET_STACK_EXPORT
    uint32_t Binary_Input_Event_Enable(
            uint32_t object_instance);

    BACNET_STACK_EXPORT
    bool Binary_Input_Event_Enable_Set(
            uint32_t object_instance,
            uint32_t event_enable);

    BACNET_STACK_EXPORT
    BACNET_NOTIFY_TYPE Binary_Input_Notify_Type(
            uint32_t object_instance);

    BACNET_STACK_EXPORT
    bool Binary_Input_Notify_Type_Set(
            uint32_t object_instance,
            BACNET_NOTIFY_TYPE notify_type);
#endif

    BACNET_STACK_EXPORT
    void Binary_Input_Intrinsic_Reporting(
        uint32_t object_instance);



#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
